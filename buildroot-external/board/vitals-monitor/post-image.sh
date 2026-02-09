#!/bin/bash
# =============================================================================
# Vitals Monitor - Post-image script
# Called by Buildroot after rootfs images are created.
# Generates the final SD card image using genimage.
#
# Arguments: none (uses Buildroot environment variables)
#
# Environment:
#   BINARIES_DIR - directory containing built images
#   BR2_EXTERNAL_VITALS_MONITOR_PATH - path to our external tree
# =============================================================================

set -euo pipefail

BOARD_DIR="${BR2_EXTERNAL_VITALS_MONITOR_PATH}/board/vitals-monitor"
GENIMAGE_CFG="${BOARD_DIR}/genimage.cfg"
GENIMAGE_TMP="${BUILD_DIR}/genimage.tmp"

echo ">>> Vitals Monitor post-image: generating SD card image"

# Clean up any previous genimage temp directory
rm -rf "${GENIMAGE_TMP}"

# Run genimage to create the final sdcard.img
genimage \
    --rootpath "${TARGET_DIR}" \
    --tmppath "${GENIMAGE_TMP}" \
    --inputpath "${BINARIES_DIR}" \
    --outputpath "${BINARIES_DIR}" \
    --config "${GENIMAGE_CFG}"

echo ">>> Vitals Monitor post-image: SD card image created at ${BINARIES_DIR}/sdcard.img"
echo ">>> Flash with: dd if=sdcard.img of=/dev/sdX bs=1M status=progress"
