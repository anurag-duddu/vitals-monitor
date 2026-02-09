#!/bin/bash
# =============================================================================
# dm-verity Setup Script for Vitals Monitor
# =============================================================================
#
# CDSCO Class B Medical Device — Vitals Monitor
# Target: STM32MP157F-DK2 (Cortex-A7 + Cortex-M4)
#
# PURPOSE:
#   dm-verity provides transparent integrity checking of block devices.
#   It creates a Merkle hash tree over the root filesystem image so that
#   every read from the block device is verified against a known-good hash.
#   If any block has been tampered with, the read returns an I/O error,
#   preventing execution of modified binaries.
#
# REGULATORY CONTEXT:
#   IEC 62443-4-2 CR 3.4 — Software integrity verification
#   IEC 62304 Section 5.8 — Software configuration management
#   CDSCO Class B — Requires demonstrated integrity of deployed software
#
#   dm-verity satisfies the requirement that the device firmware cannot be
#   silently modified after deployment.  The root hash is embedded in the
#   kernel command line (or signed FIT image), forming a chain of trust
#   from the bootloader's secure boot verification down to every file on
#   the rootfs.
#
# CHAIN OF TRUST:
#   1. TF-A (BL2) verifies U-Boot signature (STM32 secure boot fuses)
#   2. U-Boot verifies FIT image signature (kernel + DTB + root hash)
#   3. Kernel activates dm-verity using the embedded root hash
#   4. dm-verity verifies every block read from the rootfs partition
#
# USAGE:
#   ./setup-dm-verity.sh <rootfs.img> [output-dir]
#
#   Inputs:
#     rootfs.img   — The ext4 root filesystem image (read-only)
#     output-dir   — Directory for output files (default: ./verity-output)
#
#   Outputs:
#     <output-dir>/rootfs.img        — Original rootfs (unchanged)
#     <output-dir>/rootfs.hash       — Hash tree image (appended or separate)
#     <output-dir>/root-hash.txt     — Root hash for kernel cmdline
#     <output-dir>/verity-cmdline.txt — Complete dm-verity kernel cmdline args
#
# =============================================================================
set -euo pipefail

# ---------------------------------------------------------------------------
# Color output helpers
# ---------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }
die()   { error "$*"; exit 1; }

# ---------------------------------------------------------------------------
# Argument parsing and validation
# ---------------------------------------------------------------------------
if [ $# -lt 1 ]; then
    echo "Usage: $0 <rootfs.img> [output-dir]"
    echo ""
    echo "  rootfs.img   Path to the ext4 root filesystem image"
    echo "  output-dir   Output directory (default: ./verity-output)"
    exit 1
fi

ROOTFS_IMG="$1"
OUTPUT_DIR="${2:-./verity-output}"

# Verify the rootfs image exists and is a regular file
[ -f "$ROOTFS_IMG" ] || die "Root filesystem image not found: $ROOTFS_IMG"

# Verify veritysetup is available (part of cryptsetup package)
command -v veritysetup >/dev/null 2>&1 || \
    die "veritysetup not found. Install cryptsetup: apt-get install cryptsetup-bin"

# ---------------------------------------------------------------------------
# Prepare output directory
# ---------------------------------------------------------------------------
mkdir -p "$OUTPUT_DIR"
info "Output directory: $OUTPUT_DIR"

# ---------------------------------------------------------------------------
# Step 1: Calculate filesystem image metadata
# ---------------------------------------------------------------------------
info "Analyzing rootfs image..."

# Get the image size — dm-verity operates on fixed-size block devices.
# The data size must be a multiple of the data block size (4096 bytes).
IMG_SIZE=$(stat -f%z "$ROOTFS_IMG" 2>/dev/null || stat --printf="%s" "$ROOTFS_IMG" 2>/dev/null)
IMG_SIZE_MB=$(( IMG_SIZE / 1024 / 1024 ))
info "  Image size: ${IMG_SIZE_MB} MB (${IMG_SIZE} bytes)"

# Verify alignment to 4096-byte blocks
if [ $(( IMG_SIZE % 4096 )) -ne 0 ]; then
    warn "Image size is not aligned to 4096-byte blocks."
    warn "Padding to next block boundary..."
    PADDED_SIZE=$(( ((IMG_SIZE + 4095) / 4096) * 4096 ))
    truncate -s "$PADDED_SIZE" "$ROOTFS_IMG"
    info "  Padded to ${PADDED_SIZE} bytes"
fi

# ---------------------------------------------------------------------------
# Step 2: Generate the dm-verity hash tree
# ---------------------------------------------------------------------------
# veritysetup format creates a Merkle tree of SHA-256 hashes over the
# data device.  Each leaf hash covers one 4096-byte data block.  Interior
# nodes hash groups of child hashes.  The single root hash at the top
# can verify the integrity of the entire filesystem.
#
# Parameters:
#   --data-block-size=4096  — matches ext4 block size
#   --hash-block-size=4096  — one page per hash block (optimal for ARM)
#   --hash=sha256           — SHA-256 is the standard for dm-verity
#   --salt=random           — random salt prevents precomputation attacks
#
# The hash tree is written to a separate image file.  On the target device
# it will be stored on a dedicated partition or appended to the rootfs
# partition (the kernel needs both the data region and hash region).
# ---------------------------------------------------------------------------
info "Generating dm-verity hash tree (this may take a moment)..."

HASH_IMG="$OUTPUT_DIR/rootfs.hash"
ROOTHASH_FILE="$OUTPUT_DIR/root-hash.txt"

# Run veritysetup and capture the root hash from stdout.
# The format command outputs several lines; the root hash is on the
# line starting with "Root hash:".
VERITY_OUTPUT=$(veritysetup format \
    --data-block-size=4096 \
    --hash-block-size=4096 \
    --hash=sha256 \
    "$ROOTFS_IMG" \
    "$HASH_IMG" 2>&1)

echo "$VERITY_OUTPUT"

# Extract the root hash value
ROOT_HASH=$(echo "$VERITY_OUTPUT" | grep "Root hash:" | awk '{print $NF}')
SALT=$(echo "$VERITY_OUTPUT" | grep "Salt:" | awk '{print $NF}')
DATA_BLOCKS=$(echo "$VERITY_OUTPUT" | grep "Data blocks:" | awk '{print $NF}')
HASH_ALGO=$(echo "$VERITY_OUTPUT" | grep "Hash algorithm:" | awk '{print $NF}')
UUID=$(echo "$VERITY_OUTPUT" | grep "UUID:" | awk '{print $NF}')

if [ -z "$ROOT_HASH" ]; then
    die "Failed to extract root hash from veritysetup output"
fi

info "  Root hash:     $ROOT_HASH"
info "  Salt:          $SALT"
info "  Data blocks:   $DATA_BLOCKS"
info "  Hash algo:     $HASH_ALGO"

# Save the root hash to a file for build system integration
echo "$ROOT_HASH" > "$ROOTHASH_FILE"

# ---------------------------------------------------------------------------
# Step 3: Generate kernel command line arguments
# ---------------------------------------------------------------------------
# The dm-verity target is activated by the kernel's device-mapper init code.
# The following cmdline arguments tell the kernel:
#   - Which block device contains the data (rootfs partition)
#   - Which block device contains the hash tree
#   - The root hash to verify against
#   - Block sizes and hash algorithm
#
# On the STM32MP157F-DK2:
#   /dev/mmcblk0p5 = rootfs partition (ext4, read-only)
#   /dev/mmcblk0p6 = verity hash partition
#
# These partition numbers must match the SD card layout defined in
# buildroot-external/board/stm32mp157f-dk2/genimage.cfg
# ---------------------------------------------------------------------------
CMDLINE_FILE="$OUTPUT_DIR/verity-cmdline.txt"

# dm-verity kernel cmdline format:
#   dm-mod.create="<name>,<uuid>,<minor>,ro,0 <data_sectors> verity <version>
#       <data_dev> <hash_dev> <data_block_size> <hash_block_size>
#       <num_data_blocks> <hash_start_block> <algorithm> <root_hash> <salt>"
#
# For simplicity, we use the dm= format which is more readable:
DATA_SECTORS=$(( DATA_BLOCKS * 8 ))  # 4096-byte blocks -> 512-byte sectors

cat > "$CMDLINE_FILE" << CMDLINE
# dm-verity kernel command line arguments
# Add these to the U-Boot bootargs or FIT image configuration
#
# DATA_DEV and HASH_DEV must be updated to match your partition layout.
# Default assumes:
#   /dev/mmcblk0p5 = rootfs (data)
#   /dev/mmcblk0p6 = verity hash tree

root=/dev/dm-0 ro
dm-mod.create="verity-rootfs,,$UUID,ro,0 ${DATA_SECTORS} verity 1 /dev/mmcblk0p5 /dev/mmcblk0p6 4096 4096 ${DATA_BLOCKS} 0 ${HASH_ALGO} ${ROOT_HASH} ${SALT}"
CMDLINE

info "Kernel cmdline arguments written to: $CMDLINE_FILE"

# ---------------------------------------------------------------------------
# Step 4: Verify the setup by running a verification pass
# ---------------------------------------------------------------------------
info "Verifying hash tree integrity..."

veritysetup verify \
    "$ROOTFS_IMG" \
    "$HASH_IMG" \
    "$ROOT_HASH" 2>&1 && \
    info "Verification PASSED — hash tree is consistent." || \
    die "Verification FAILED — hash tree is inconsistent!"

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
HASH_SIZE=$(stat -f%z "$HASH_IMG" 2>/dev/null || stat --printf="%s" "$HASH_IMG" 2>/dev/null)
HASH_SIZE_MB=$(( HASH_SIZE / 1024 / 1024 ))

echo ""
info "=============================="
info " dm-verity setup complete"
info "=============================="
info ""
info "  Rootfs image:   $ROOTFS_IMG (${IMG_SIZE_MB} MB)"
info "  Hash tree:      $HASH_IMG (${HASH_SIZE_MB} MB)"
info "  Root hash:      $ROOT_HASH"
info "  Root hash file: $ROOTHASH_FILE"
info "  Cmdline args:   $CMDLINE_FILE"
info ""
info "NEXT STEPS:"
info "  1. Flash rootfs.img to the rootfs partition (/dev/mmcblk0p5)"
info "  2. Flash rootfs.hash to the verity partition (/dev/mmcblk0p6)"
info "  3. Add the root hash to the kernel cmdline (see $CMDLINE_FILE)"
info "  4. Sign the FIT image containing the kernel + DTB + cmdline"
info "  5. Enable STM32 secure boot fuses to lock the chain of trust"
echo ""
