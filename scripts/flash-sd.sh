#!/bin/bash
# =============================================================================
# SD Card Flash Script for Vitals Monitor
# =============================================================================
#
# CDSCO Class B Medical Device — Vitals Monitor
# Target: STM32MP157F-DK2 (Cortex-A7 + Cortex-M4)
#
# PURPOSE:
#   Flash the built firmware image onto an SD card for the STM32MP157F-DK2
#   development kit.  Includes safety checks to prevent accidentally
#   overwriting the wrong disk and post-write verification via checksums.
#
# REGULATORY CONTEXT:
#   IEC 62304 Section 5.8   — Software release procedures
#   IEC 62304 Section 5.8.7 — Verification of delivered software
#   CDSCO Class B           — Documented, verified deployment process
#
# USAGE:
#   ./scripts/flash-sd.sh [image-file] [device]
#
#   If no arguments given, the script auto-detects the SD card and uses
#   the default image path (output/images/sdcard.img).
#
# SUPPORTED PLATFORMS:
#   - macOS (uses diskutil for detection and unmounting)
#   - Linux (uses lsblk for detection, udisksctl or umount for unmounting)
#
# =============================================================================
set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEFAULT_IMAGE="$PROJECT_ROOT/output/images/sdcard.img"

# ---------------------------------------------------------------------------
# Color output helpers
# ---------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${GREEN}[FLASH]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }
step()  { echo -e "${CYAN}[STEP]${NC}  $*"; }
die()   { error "$*"; exit 1; }

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
detect_platform() {
    case "$(uname -s)" in
        Darwin) echo "macos" ;;
        Linux)  echo "linux" ;;
        *)      die "Unsupported platform: $(uname -s)" ;;
    esac
}

PLATFORM=$(detect_platform)

# ---------------------------------------------------------------------------
# SD card detection
# ---------------------------------------------------------------------------

# Detect removable SD card devices.
# Returns the raw device path (e.g., /dev/rdisk4 on macOS, /dev/sdb on Linux).
# Filters out internal drives, USB hubs, and other non-SD devices.
detect_sd_card() {
    local devices=()

    if [ "$PLATFORM" = "macos" ]; then
        # macOS: use diskutil to find external, physical disks
        # diskutil list shows all disks; we filter for external/removable
        while IFS= read -r line; do
            local disk_id
            disk_id=$(echo "$line" | awk '{print $1}')
            # Check if disk is external and removable
            local disk_info
            disk_info=$(diskutil info "$disk_id" 2>/dev/null || true)
            if echo "$disk_info" | grep -q "Removable Media:.*Removable" || \
               echo "$disk_info" | grep -q "Protocol:.*USB" || \
               echo "$disk_info" | grep -q "Location:.*External"; then
                local size
                size=$(echo "$disk_info" | grep "Disk Size:" | head -1 | \
                       sed 's/.*(\([0-9]*\) Bytes).*/\1/' 2>/dev/null || echo "0")
                # Filter: SD cards are typically 2GB-128GB
                if [ "$size" -gt 1000000000 ] 2>/dev/null && \
                   [ "$size" -lt 137438953472 ] 2>/dev/null; then
                    local size_gb
                    size_gb=$(echo "scale=1; $size / 1073741824" | bc 2>/dev/null || echo "?")
                    devices+=("$disk_id (${size_gb} GB)")
                fi
            fi
        done < <(diskutil list 2>/dev/null | grep "^/dev/disk" | awk '{print $1}')

    elif [ "$PLATFORM" = "linux" ]; then
        # Linux: use lsblk to find removable block devices
        while IFS= read -r line; do
            local name size rm type
            name=$(echo "$line" | awk '{print $1}')
            size=$(echo "$line" | awk '{print $2}')
            rm=$(echo "$line" | awk '{print $3}')
            type=$(echo "$line" | awk '{print $4}')

            # Only show removable disks (not partitions)
            if [ "$rm" = "1" ] && [ "$type" = "disk" ]; then
                devices+=("/dev/$name ($size)")
            fi
        done < <(lsblk -dno NAME,SIZE,RM,TYPE 2>/dev/null)
    fi

    if [ ${#devices[@]} -eq 0 ]; then
        return 1
    fi

    # Print detected devices
    for dev in "${devices[@]}"; do
        echo "$dev"
    done
}

# ---------------------------------------------------------------------------
# Unmount all partitions on a device
# ---------------------------------------------------------------------------
unmount_device() {
    local device="$1"

    info "Unmounting all partitions on $device..."

    if [ "$PLATFORM" = "macos" ]; then
        # macOS: diskutil unmountDisk unmounts all volumes on the disk
        diskutil unmountDisk "$device" 2>/dev/null || true
    elif [ "$PLATFORM" = "linux" ]; then
        # Linux: unmount each partition individually
        local partitions
        partitions=$(lsblk -lno NAME "$device" 2>/dev/null | tail -n +2)
        for part in $partitions; do
            umount "/dev/$part" 2>/dev/null || true
        done
        # Also try udisksctl if available
        if command -v udisksctl >/dev/null 2>&1; then
            for part in $partitions; do
                udisksctl unmount -b "/dev/$part" 2>/dev/null || true
            done
        fi
    fi
}

# ---------------------------------------------------------------------------
# Get the raw device path (for faster dd writes)
# ---------------------------------------------------------------------------
raw_device_path() {
    local device="$1"
    if [ "$PLATFORM" = "macos" ]; then
        # macOS: /dev/rdiskN is the raw (unbuffered) device — much faster for dd
        echo "$device" | sed 's|/dev/disk|/dev/rdisk|'
    else
        echo "$device"
    fi
}

# ---------------------------------------------------------------------------
# Eject the device safely
# ---------------------------------------------------------------------------
eject_device() {
    local device="$1"

    info "Ejecting $device..."

    if [ "$PLATFORM" = "macos" ]; then
        diskutil eject "$device" 2>/dev/null || true
    elif [ "$PLATFORM" = "linux" ]; then
        sync
        if command -v eject >/dev/null 2>&1; then
            eject "$device" 2>/dev/null || true
        elif command -v udisksctl >/dev/null 2>&1; then
            udisksctl power-off -b "$device" 2>/dev/null || true
        fi
    fi
}

# ---------------------------------------------------------------------------
# Main flash procedure
# ---------------------------------------------------------------------------

# Parse arguments
IMAGE="${1:-$DEFAULT_IMAGE}"
DEVICE="${2:-}"

# Verify root/sudo
if [ "$(id -u)" -ne 0 ]; then
    warn "This script requires root privileges for raw disk access."
    warn "Re-running with sudo..."
    exec sudo "$0" "$@"
fi

# Verify image file exists
[ -f "$IMAGE" ] || die "Image file not found: $IMAGE"

IMAGE_SIZE=$(stat -f%z "$IMAGE" 2>/dev/null || stat --printf="%s" "$IMAGE" 2>/dev/null)
IMAGE_SIZE_MB=$(( IMAGE_SIZE / 1024 / 1024 ))
info "Image: $IMAGE (${IMAGE_SIZE_MB} MB)"

# Compute source checksum before write
step "Computing source image checksum..."
if command -v sha256sum >/dev/null 2>&1; then
    SRC_CHECKSUM=$(sha256sum "$IMAGE" | awk '{print $1}')
elif command -v shasum >/dev/null 2>&1; then
    SRC_CHECKSUM=$(shasum -a 256 "$IMAGE" | awk '{print $1}')
else
    die "No SHA-256 tool found (need sha256sum or shasum)"
fi
info "  SHA-256: ${SRC_CHECKSUM:0:16}...${SRC_CHECKSUM: -16}"

# Auto-detect SD card if no device specified
if [ -z "$DEVICE" ]; then
    step "Detecting SD card..."
    DETECTED=$(detect_sd_card) || die "No SD card detected. Insert card and retry."

    DEVICE_COUNT=$(echo "$DETECTED" | wc -l | tr -d ' ')

    if [ "$DEVICE_COUNT" -eq 1 ]; then
        DEVICE=$(echo "$DETECTED" | awk '{print $1}')
        info "  Detected: $DETECTED"
    else
        echo ""
        info "Multiple removable devices detected:"
        echo ""
        local idx=1
        while IFS= read -r line; do
            echo "  [$idx] $line"
            idx=$((idx + 1))
        done <<< "$DETECTED"
        echo ""
        read -rp "Select device number [1-$DEVICE_COUNT]: " SELECTION
        DEVICE=$(echo "$DETECTED" | sed -n "${SELECTION}p" | awk '{print $1}')
        [ -n "$DEVICE" ] || die "Invalid selection."
    fi
fi

# Verify the device exists and is a block device
[ -e "$DEVICE" ] || die "Device not found: $DEVICE"

# ---------------------------------------------------------------------------
# Safety confirmation
# ---------------------------------------------------------------------------
# This is the most critical safety check.  Writing to the wrong device
# could destroy the host operating system or user data.
echo ""
warn "======================================================="
warn " WARNING: ALL DATA ON $DEVICE WILL BE DESTROYED"
warn "======================================================="
echo ""
echo "  Source image: $IMAGE (${IMAGE_SIZE_MB} MB)"
echo "  Target device: $DEVICE"
echo "  Platform: $PLATFORM"
echo ""

# Show current partition table of the target device
if [ "$PLATFORM" = "macos" ]; then
    diskutil list "$DEVICE" 2>/dev/null || true
elif [ "$PLATFORM" = "linux" ]; then
    lsblk "$DEVICE" 2>/dev/null || true
fi

echo ""
read -rp "Type 'FLASH' to proceed: " CONFIRM
[ "$CONFIRM" = "FLASH" ] || die "Aborted by user."

# ---------------------------------------------------------------------------
# Step 1: Unmount all partitions
# ---------------------------------------------------------------------------
step "Unmounting partitions..."
unmount_device "$DEVICE"

# ---------------------------------------------------------------------------
# Step 2: Write the image
# ---------------------------------------------------------------------------
RAW_DEVICE=$(raw_device_path "$DEVICE")
step "Writing image to $RAW_DEVICE..."
info "  This will take several minutes. Do not remove the SD card."
echo ""

# Use dd with:
#   bs=4m   — 4 MB block size for optimal throughput
#   conv=sync — pad final block with zeros
#   status=progress — show write progress (GNU dd; falls back silently)
if dd if="$IMAGE" of="$RAW_DEVICE" bs=4m conv=sync status=progress 2>/dev/null; then
    true
else
    # Retry without status=progress (macOS dd may not support it)
    dd if="$IMAGE" of="$RAW_DEVICE" bs=4m conv=sync 2>&1
fi

# Flush filesystem buffers to ensure all data is written to the card
sync

info "Write complete."

# ---------------------------------------------------------------------------
# Step 3: Verify the write
# ---------------------------------------------------------------------------
step "Verifying written data (reading back and comparing checksums)..."

# Read back exactly the same number of bytes as the source image
# and compute a checksum.  This catches write errors, bad blocks,
# and partial writes.
VERIFY_CHECKSUM=$(dd if="$RAW_DEVICE" bs=4m count=$(( (IMAGE_SIZE + 4194303) / 4194304 )) 2>/dev/null | \
    head -c "$IMAGE_SIZE" | \
    { sha256sum 2>/dev/null || shasum -a 256; } | \
    awk '{print $1}')

if [ "$SRC_CHECKSUM" = "$VERIFY_CHECKSUM" ]; then
    info "Verification PASSED"
    info "  Source:   ${SRC_CHECKSUM:0:16}...${SRC_CHECKSUM: -16}"
    info "  Written:  ${VERIFY_CHECKSUM:0:16}...${VERIFY_CHECKSUM: -16}"
else
    error "Verification FAILED!"
    error "  Source:   $SRC_CHECKSUM"
    error "  Written:  $VERIFY_CHECKSUM"
    error ""
    error "The SD card may be defective. Try a different card."
    die "Flash verification failed."
fi

# ---------------------------------------------------------------------------
# Step 4: Eject safely
# ---------------------------------------------------------------------------
step "Ejecting SD card..."
eject_device "$DEVICE"

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
info "=============================="
info " Flash complete!"
info "=============================="
info ""
info "  Image:    $IMAGE"
info "  Device:   $DEVICE"
info "  Size:     ${IMAGE_SIZE_MB} MB"
info "  Checksum: VERIFIED"
info ""
info "NEXT STEPS:"
info "  1. Remove the SD card from this computer"
info "  2. Insert into the STM32MP157F-DK2 SD card slot"
info "  3. Set boot switches: BOOT0=OFF, BOOT2=ON (SD card boot)"
info "  4. Power on the board"
info "  5. Serial console: /dev/ttyACM0 at 115200 baud"
echo ""
