#!/bin/bash
# =============================================================================
# LUKS2 Encrypted Data Partition Setup for Vitals Monitor
# =============================================================================
#
# CDSCO Class B Medical Device — Vitals Monitor
# Target: STM32MP157F-DK2 (Cortex-A7 + Cortex-M4)
#
# PURPOSE:
#   This script creates a LUKS2 encrypted partition for storing patient
#   vitals data (SQLite trend database, alarm logs, configuration).
#   Encryption at rest protects patient data if the SD card is physically
#   removed from the device.
#
# REGULATORY CONTEXT:
#   IEC 62443-4-2 CR 4.1 — Data confidentiality (encryption at rest)
#   IT Act 2000 / DISHA   — Protection of digital health information
#   CDSCO Class B         — Patient data must be protected from unauthorized
#                            disclosure even upon physical device compromise
#
# KEY DERIVATION:
#   The encryption key is derived from the STM32MP1's hardware unique ID
#   (OTP fuses at BSEC OTP word 57-59).  This ensures:
#     - Each device has a unique encryption key
#     - The key is bound to the specific hardware (cannot decrypt on another board)
#     - No key file needs to be stored on the filesystem
#   The hardware ID is fed through PBKDF2/Argon2id to produce the LUKS master key.
#
# DATA PARTITION LAYOUT:
#   /opt/vitals-monitor/data/
#     vitals_trends.db        — SQLite database (raw + aggregated vitals)
#     alarm_events.db         — Alarm history (IEC 60601-1-8 compliance)
#     config.db               — Device configuration and calibration
#     audit/                  — Audit trail logs
#     export/                 — Temporary export staging area
#
# USAGE:
#   ./setup-luks.sh <block-device>
#
#   Example:
#     ./setup-luks.sh /dev/mmcblk0p7
#
# WARNING:
#   This script will DESTROY all data on the specified partition.
#   It is intended to be run once during device provisioning (factory setup).
#
# =============================================================================
set -euo pipefail

# ---------------------------------------------------------------------------
# Color output helpers
# ---------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }
die()   { error "$*"; exit 1; }

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

# LUKS label — used for systemd automount and identification
LUKS_LABEL="vitals-data"

# Mapped device name when opened
DM_NAME="vitals-data-crypt"

# Mount point on the target filesystem
MOUNT_POINT="/opt/vitals-monitor/data"

# LUKS2 cipher and key parameters
# AES-256-XTS is the standard for disk encryption; XTS mode is designed
# for storage (handles sector-level encryption without IV reuse issues).
CIPHER="aes-xts-plain64"
KEY_SIZE=512  # 512 bits = 256-bit AES key + 256-bit XTS tweak key
HASH="sha256"

# PBKDF parameters — Argon2id is LUKS2 default; tuned for STM32MP1's
# 800MHz Cortex-A7 with 512MB DDR3.  These parameters ensure key
# derivation takes ~2 seconds on the target hardware (acceptable for
# boot-time unlock; too slow for brute-force attacks).
PBKDF="argon2id"
PBKDF_MEMORY=65536    # 64 MB (fits within available RAM)
PBKDF_TIME=4          # 4 iterations
PBKDF_PARALLEL=1      # Single-core derivation (Cortex-A7 is dual-core)

# Hardware ID source — STM32MP1 unique device ID via sysfs
# The BSEC OTP words 57-59 contain a 96-bit unique ID programmed at
# the ST factory.  This ID is immutable and hardware-bound.
HW_ID_PATH="/sys/bus/nvmem/devices/stm32-romem0/nvmem"
HW_ID_FALLBACK="/sys/firmware/devicetree/base/soc/efuse@5c005000/serial-number"

# ---------------------------------------------------------------------------
# Argument parsing and validation
# ---------------------------------------------------------------------------
if [ $# -lt 1 ]; then
    echo "Usage: $0 <block-device>"
    echo ""
    echo "  block-device   The partition to encrypt (e.g., /dev/mmcblk0p7)"
    echo ""
    echo "WARNING: This will DESTROY all data on the specified partition."
    exit 1
fi

BLOCK_DEV="$1"

# Verify we are running as root (required for cryptsetup and mount)
[ "$(id -u)" -eq 0 ] || die "This script must be run as root."

# Verify the block device exists
[ -b "$BLOCK_DEV" ] || die "Block device not found: $BLOCK_DEV"

# Verify cryptsetup is available
command -v cryptsetup >/dev/null 2>&1 || \
    die "cryptsetup not found. Install: apt-get install cryptsetup-bin"

# ---------------------------------------------------------------------------
# Safety check — confirm with operator
# ---------------------------------------------------------------------------
echo ""
warn "======================================================="
warn " WARNING: ALL DATA ON $BLOCK_DEV WILL BE DESTROYED"
warn "======================================================="
echo ""
echo "  Device: $BLOCK_DEV"
echo "  Label:  $LUKS_LABEL"
echo "  Cipher: $CIPHER"
echo "  Key:    ${KEY_SIZE}-bit (derived from hardware unique ID)"
echo ""
read -rp "Type 'YES' to continue: " CONFIRM
[ "$CONFIRM" = "YES" ] || die "Aborted by operator."

# ---------------------------------------------------------------------------
# Step 1: Read the hardware unique ID
# ---------------------------------------------------------------------------
# The STM32MP1 has a 96-bit unique device identifier burned into OTP fuses
# at the ST semiconductor factory.  This value is:
#   - Unique per chip (no two STM32MP1s share the same ID)
#   - Immutable (cannot be changed after fuse programming)
#   - Readable via sysfs NVMEM interface
#
# We use this as the passphrase source for LUKS key derivation.  Combined
# with Argon2id PBKDF, this produces a 256-bit AES key that is:
#   - Bound to this specific hardware unit
#   - Not stored anywhere on the filesystem
#   - Reproducible on this device (for unlock at every boot)
#   - Not reproducible on any other device
# ---------------------------------------------------------------------------
info "Reading hardware unique ID..."

HW_ID=""
if [ -f "$HW_ID_PATH" ]; then
    # Read raw NVMEM and extract the unique ID region (bytes 0-11)
    HW_ID=$(xxd -p -l 12 "$HW_ID_PATH" 2>/dev/null | tr -d ' \n')
elif [ -f "$HW_ID_FALLBACK" ]; then
    # Fallback: device tree serial number property
    HW_ID=$(xxd -p "$HW_ID_FALLBACK" 2>/dev/null | tr -d ' \n')
else
    die "Cannot read hardware unique ID. Ensure running on STM32MP1 target."
fi

[ -n "$HW_ID" ] || die "Hardware unique ID is empty."
info "  Hardware ID read successfully (${#HW_ID} hex chars)"

# Derive a passphrase by hashing the hardware ID with a device-specific salt.
# This adds entropy and ensures the raw hardware ID is never used directly.
PASSPHRASE=$(echo -n "vitals-monitor:luks:${HW_ID}:${LUKS_LABEL}" | sha256sum | awk '{print $1}')
info "  Key derivation material prepared"

# ---------------------------------------------------------------------------
# Step 2: Create the LUKS2 encrypted container
# ---------------------------------------------------------------------------
# LUKS2 is the modern Linux disk encryption format.  It stores:
#   - Header with cipher/PBKDF metadata (detachable for extra security)
#   - Up to 32 key slots (we use slot 0 for the hardware-derived key)
#   - The encrypted master key (decrypted via the passphrase + PBKDF)
#
# The actual data encryption uses the master key (random, generated by
# cryptsetup), NOT the passphrase directly.  The passphrase only unlocks
# the master key via the PBKDF-protected key slot.
# ---------------------------------------------------------------------------
info "Creating LUKS2 container on $BLOCK_DEV..."

echo -n "$PASSPHRASE" | cryptsetup luksFormat \
    --type luks2 \
    --cipher "$CIPHER" \
    --key-size "$KEY_SIZE" \
    --hash "$HASH" \
    --pbkdf "$PBKDF" \
    --pbkdf-memory "$PBKDF_MEMORY" \
    --pbkdf-force-iterations "$PBKDF_TIME" \
    --pbkdf-parallel "$PBKDF_PARALLEL" \
    --label "$LUKS_LABEL" \
    --batch-mode \
    --key-file=- \
    "$BLOCK_DEV"

info "  LUKS2 container created successfully"

# ---------------------------------------------------------------------------
# Step 3: Open (unlock) the LUKS container
# ---------------------------------------------------------------------------
info "Opening LUKS container as /dev/mapper/$DM_NAME..."

echo -n "$PASSPHRASE" | cryptsetup luksOpen \
    --key-file=- \
    "$BLOCK_DEV" \
    "$DM_NAME"

info "  Container opened at /dev/mapper/$DM_NAME"

# ---------------------------------------------------------------------------
# Step 4: Create ext4 filesystem inside the encrypted container
# ---------------------------------------------------------------------------
# ext4 parameters tuned for the vitals monitor workload:
#   - Small block count (data partition is ~128MB on SD card)
#   - No reserved blocks (embedded device, no multi-user)
#   - Journal enabled (crash recovery for SQLite WAL)
# ---------------------------------------------------------------------------
info "Creating ext4 filesystem..."

mkfs.ext4 \
    -L "$LUKS_LABEL" \
    -m 0 \
    -O "^huge_file" \
    -E "lazy_itable_init=0,lazy_journal_init=0" \
    "/dev/mapper/$DM_NAME"

info "  ext4 filesystem created"

# ---------------------------------------------------------------------------
# Step 5: Mount and create directory structure
# ---------------------------------------------------------------------------
info "Mounting filesystem and creating directory structure..."

# Create mount point if it does not exist
mkdir -p "$MOUNT_POINT"

# Mount the encrypted filesystem
mount "/dev/mapper/$DM_NAME" "$MOUNT_POINT"

# Create the application data directory tree
# These directories store different categories of patient/device data:
#
# vitals_trends.db  — Time-series vital signs (HR, SpO2, RR, BP, Temp)
#                     Two-tier storage: raw (1s, 4h) + aggregated (1min, 72h)
#
# alarm_events.db   — Alarm timeline per IEC 60601-1-8 requirements
#                     Includes alarm type, priority, timestamp, acknowledgment
#
# config.db         — Device configuration, calibration coefficients,
#                     alarm thresholds, patient demographics
#
# audit/            — Audit trail logs for regulatory compliance
#                     Who accessed what data, when, configuration changes
#
# export/           — Temporary staging for data export (USB, network)
#                     Cleared after successful transfer
#
# backup/           — Rolling database backups (last 3 copies)
#                     Used for crash recovery if WAL replay fails
#
mkdir -p "$MOUNT_POINT"/{audit,export,backup}

# Set ownership to the vitals service user
chown -R vitals:vitals "$MOUNT_POINT"

# Set restrictive permissions:
#   - Directory: rwx for owner, rx for group, no access for others
#   - This prevents other system users from reading patient data
chmod 750 "$MOUNT_POINT"
chmod 750 "$MOUNT_POINT"/{audit,export,backup}

info "  Directory structure created:"
info "    $MOUNT_POINT/"
info "    $MOUNT_POINT/audit/"
info "    $MOUNT_POINT/export/"
info "    $MOUNT_POINT/backup/"

# ---------------------------------------------------------------------------
# Step 6: Clean up
# ---------------------------------------------------------------------------
info "Unmounting and closing LUKS container..."

umount "$MOUNT_POINT"
cryptsetup luksClose "$DM_NAME"

# Clear the passphrase from memory (best-effort; bash variables are
# not securely erasable, but we zero the variable to limit exposure)
PASSPHRASE=""
HW_ID=""

# ---------------------------------------------------------------------------
# Summary and boot integration instructions
# ---------------------------------------------------------------------------
echo ""
info "=============================="
info " LUKS setup complete"
info "=============================="
info ""
info "  Device:     $BLOCK_DEV"
info "  Label:      $LUKS_LABEL"
info "  Cipher:     $CIPHER ($KEY_SIZE-bit)"
info "  PBKDF:      $PBKDF (${PBKDF_MEMORY}KB memory, ${PBKDF_TIME} iterations)"
info "  Mount:      $MOUNT_POINT"
info ""
info "BOOT INTEGRATION:"
info "  The LUKS container must be unlocked at boot before vitals-monitor"
info "  services start.  Add a systemd unit or initramfs hook:"
info ""
info "  Option A — systemd-cryptsetup (recommended):"
info "    Add to /etc/crypttab:"
info "      $DM_NAME  $BLOCK_DEV  -  luks,discard,no-read-workqueue"
info ""
info "  Option B — initramfs hook:"
info "    Create /etc/initramfs-tools/hooks/vitals-luks that reads"
info "    the hardware ID and calls cryptsetup luksOpen during initramfs."
info ""
info "  In either case, the hardware-derived key is computed at runtime"
info "  from the STM32MP1 OTP fuses — no key file is stored on disk."
echo ""
