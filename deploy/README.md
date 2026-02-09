# Deployment Guide -- Vitals Monitor on STM32MP157F-DK2

This document covers building, flashing, and configuring the vitals monitor
for the STM32MP157F-DK2 target hardware. It addresses Phases 8 (Buildroot),
10 (Security), and 11 (OTA) from the project roadmap.

---

## Table of Contents

1. [Directory Structure on Target](#directory-structure-on-target)
2. [systemd Service Architecture](#systemd-service-architecture)
3. [Building for Target (Buildroot)](#building-for-target-buildroot)
4. [SD Card Flashing Procedure](#sd-card-flashing-procedure)
5. [First Boot Checklist](#first-boot-checklist)
6. [WiFi Configuration](#wifi-configuration)
7. [Secure Boot Setup (Phase 10)](#secure-boot-setup-phase-10)
8. [OTA Update Plan (Phase 11)](#ota-update-plan-phase-11)

---

## Directory Structure on Target

```
/opt/vitals-monitor/              # Application root
  bin/
    vitals-ui                     # UI application (LVGL + fbdev)
    sensor-service                # Sensor data acquisition daemon
    alarm-service                 # Alarm evaluation daemon
    network-service               # EHR/ABDM sync daemon
    watchdog-monitor              # System watchdog and health monitor
  lib/
    liblvgl.so                    # LVGL shared library
    libnanomsg.so                 # nanomsg IPC library
    libsqlite3.so                 # SQLite library
    libmbedtls.so                 # mbedTLS library
  data/
    vitals_trends.db              # SQLite database (vitals, trends, alarms)
    config.db                     # Device configuration database
    audit.db                      # Audit log database
  etc/
    vitals-monitor.conf           # Master configuration file
    alarm-limits.conf             # Alarm threshold defaults
    network.conf                  # Network / WiFi settings
  share/
    fonts/                        # UI fonts
    keys/                         # OTA signature verification keys (public only)
  logs/                           # Rotating log files (if journald not used)

/tmp/vitals-monitor/              # Runtime IPC sockets (tmpfs)
  vitals.ipc                      # nanomsg: vitals data (1 Hz)
  waveforms.ipc                   # nanomsg: waveform packets (10-20 Hz)
  alarms.ipc                      # nanomsg: alarm events

/etc/systemd/system/              # Service unit files
  watchdog.service
  sensor-service.service
  alarm-service.service
  vitals-ui.service
  network-service.service
```

---

## systemd Service Architecture

The application is split into 5 services managed by systemd. This provides
process isolation, automatic restart on crash, resource limits, and security
hardening.

### Service Dependency Graph

```
                    watchdog.service
                    (starts first)
                         |
                         v
                sensor-service.service
                   /            \
                  v              v
       alarm-service.service   network-service.service
                  \
                   v
            vitals-ui.service
```

### Service Descriptions

| Service            | Binary             | User    | Memory Limit | Purpose                              |
|--------------------|--------------------|---------|--------------|--------------------------------------|
| watchdog           | watchdog-monitor   | root    | 16 MB        | HW watchdog, service health monitor  |
| sensor-service     | sensor-service     | vitals  | 64 MB        | Read sensors, publish via IPC        |
| alarm-service      | alarm-service      | vitals  | 32 MB        | Evaluate alarm thresholds            |
| vitals-ui          | vitals-ui          | vitals  | 128 MB       | LVGL UI, framebuffer rendering       |
| network-service    | network-service    | vitals  | 64 MB        | WiFi, EHR sync, OTA checks          |

### Service Files

The systemd unit files are in `deploy/systemd/`. They are installed to
`/etc/systemd/system/` during the Buildroot image build.

Key features of each unit file:
- `Type=notify` -- service signals readiness via `sd_notify()`
- `WatchdogSec` -- systemd kills the service if it stops heartbeating
- `Restart=always` + `RestartSec=3` -- automatic restart on crash
- `ProtectSystem=strict` -- read-only access to system directories
- `NoNewPrivileges=yes` -- no privilege escalation
- `SupplementaryGroups` -- minimal hardware access (i2c, spi, gpio, video)

### Enabling Services

```bash
systemctl enable watchdog sensor-service alarm-service vitals-ui network-service
systemctl start watchdog
# Other services start in dependency order
```

### Checking Status

```bash
systemctl status sensor-service
journalctl -u vitals-ui -f     # Follow UI logs
journalctl -u sensor-service --since "5 min ago"
```

---

## Building for Target (Buildroot)

The target image is built using Buildroot with an external tree that adds
the vitals-monitor packages and board configuration.

### Prerequisites

- Docker (for reproducible builds)
- At least 30 GB free disk space
- At least 8 GB RAM

### Build Steps

```bash
# Clone Buildroot (use version matching our external tree)
git clone https://github.com/buildroot/buildroot.git -b 2024.02.x
cd buildroot

# Configure with our external tree and defconfig
BR2_EXTERNAL=/path/to/vitals-monitor/buildroot-external \
  make vitals_monitor_stm32mp157f_dk2_defconfig

# Build (takes 30-60 minutes on first run)
make -j$(nproc)

# Output image
ls output/images/sdcard.img
```

### Using Docker (Recommended)

```bash
cd /path/to/vitals-monitor
./scripts/docker-build.sh all
# Output: buildroot/output/images/sdcard.img
```

### Buildroot External Tree Structure

```
buildroot-external/
  external.desc                       # BR2_EXTERNAL descriptor
  external.mk                        # Top-level package includes
  Config.in                           # Top-level Kconfig
  configs/
    vitals_monitor_stm32mp157f_dk2_defconfig
  board/
    vitals-monitor/
      linux.config                    # Kernel config fragment
      uboot.config                    # U-Boot config fragment
      genimage.cfg                    # Partition layout for sdcard.img
      post-build.sh                   # Filesystem overlay install script
      post-image.sh                   # Image generation script
      overlay/                        # Files copied into rootfs
        etc/systemd/system/           # Service unit files
        opt/vitals-monitor/etc/       # Default configuration
  package/
    vitals-ui/
      vitals-ui.mk
      Config.in
    sensor-service/
      sensor-service.mk
      Config.in
    alarm-service/
      alarm-service.mk
      Config.in
    network-service/
      network-service.mk
      Config.in
    watchdog-monitor/
      watchdog-monitor.mk
      Config.in
```

### Key Buildroot Configuration Choices

| Setting                        | Value          | Reason                           |
|--------------------------------|----------------|----------------------------------|
| `BR2_arm`                      | y              | ARM Cortex-A7 target             |
| `BR2_TOOLCHAIN_EXTERNAL`       | Linaro 7.x     | Stable cross-compiler            |
| `BR2_TARGET_GENERIC_INIT_SYSTEMD` | y           | systemd for service management   |
| `BR2_PACKAGE_SQLITE`           | y              | Trend and config storage         |
| `BR2_PACKAGE_NANOMSG`          | y              | IPC between services             |
| `BR2_PACKAGE_MBEDTLS`          | y              | TLS and encryption               |
| `BR2_PACKAGE_WPASUPP`          | y              | WiFi support                     |
| `BR2_PACKAGE_SWUPDATE`         | y              | OTA updates (Phase 11)           |
| `BR2_ROOTFS_OVERLAY`           | board/overlay  | Install config and services      |

---

## SD Card Flashing Procedure

### From macOS

```bash
# Identify the SD card device
diskutil list   # Look for the SD card (e.g., /dev/disk4)

# Unmount (do NOT eject)
diskutil unmountDisk /dev/disk4

# Flash (use rdisk for faster raw writes)
sudo dd if=output/images/sdcard.img of=/dev/rdisk4 bs=1m status=progress

# Eject safely
diskutil eject /dev/disk4
```

### From Linux

```bash
# Identify device
lsblk

# Flash
sudo dd if=output/images/sdcard.img of=/dev/sdX bs=1M status=progress
sync
```

### Partition Layout (genimage.cfg)

| Partition | Label     | Size   | Filesystem | Contents                       |
|-----------|-----------|--------|------------|--------------------------------|
| 1         | fsbl1     | 256 KB | raw        | FSBL (TF-A BL2)               |
| 2         | fsbl2     | 256 KB | raw        | FSBL backup                    |
| 3         | ssbl      | 2 MB   | raw        | U-Boot                         |
| 4         | rootfsA   | 256 MB | ext4       | Active root filesystem         |
| 5         | rootfsB   | 256 MB | ext4       | Standby root filesystem (OTA)  |
| 6         | data      | rest   | ext4       | /opt/vitals-monitor/data (LUKS)|

---

## First Boot Checklist

After flashing and inserting the SD card into the STM32MP157F-DK2:

### Hardware Setup

1. Connect 5V/3A power supply to USB-C (PWR)
2. Connect USB-C (ST-LINK) to host for serial console
3. Insert SD card
4. Optionally connect USB WiFi dongle (if not using onboard)

### Serial Console

```bash
# macOS
screen /dev/tty.usbmodem* 115200

# Linux
minicom -D /dev/ttyACM0 -b 115200
```

### Boot Verification Checklist

```
[ ] U-Boot prompt appears (hold key to interrupt)
[ ] Kernel boots (penguin logo or console output)
[ ] systemd reaches multi-user.target
[ ] watchdog.service is active
[ ] sensor-service.service is active
[ ] alarm-service.service is active
[ ] vitals-ui.service is active
[ ] Display shows UI on the LCD (or HDMI)
[ ] Touch input responds
[ ] journalctl shows no critical errors
```

### Diagnostic Commands

```bash
# Check all vitals-monitor services
systemctl status watchdog sensor-service alarm-service vitals-ui network-service

# Check IPC sockets
ls -la /tmp/vitals-monitor/

# Check sensor access
ls -la /dev/i2c-* /dev/spidev*

# Check display
cat /sys/class/graphics/fb0/virtual_size

# Check memory usage
free -m

# Check CPU temperature
cat /sys/class/thermal/thermal_zone0/temp
```

---

## WiFi Configuration

### Method 1: Configuration File (Pre-boot)

Edit the network configuration before first boot by mounting the data
partition and creating/editing the WiFi config:

```bash
# Mount data partition from SD card on host
sudo mount /dev/sdX6 /mnt

# Edit WiFi configuration
cat > /mnt/opt/vitals-monitor/etc/network.conf << 'EOF'
WIFI_SSID=HospitalWiFi
WIFI_PSK=your_password_here
WIFI_COUNTRY=IN
WIFI_SECURITY=WPA2
EOF

sudo umount /mnt
```

### Method 2: UI Configuration (Runtime)

Navigate to Settings > Network on the vitals monitor touchscreen.
The network-service will scan for available networks and allow selection.

### Method 3: Command Line (Runtime)

```bash
# Scan for networks
wpa_cli scan
wpa_cli scan_results

# Connect
wpa_cli add_network
wpa_cli set_network 0 ssid '"HospitalWiFi"'
wpa_cli set_network 0 psk '"password"'
wpa_cli enable_network 0
wpa_cli save_config

# Verify
ip addr show wlan0
ping -c 3 8.8.8.8
```

---

## Secure Boot Setup (Phase 10)

Phase 10 hardens the device for production deployment. This section provides
an overview; detailed implementation instructions will be added when the
phase is active.

### Secure Boot Chain

```
ROM bootloader (STM32MP1 built-in)
    |  verifies signature of...
    v
TF-A BL2 (FSBL) -- signed with OEM key
    |  verifies signature of...
    v
U-Boot (SSBL) -- signed with OEM key
    |  verifies signature of...
    v
Linux kernel + device tree -- signed with OEM key
    |  verifies integrity of...
    v
Root filesystem -- dm-verity hash tree
```

### Key Components

| Component       | Technology      | Purpose                                    |
|-----------------|-----------------|---------------------------------------------|
| Secure boot     | STM32MP1 HAB    | Verify boot chain signatures                |
| Rootfs integrity| dm-verity       | Detect tampering of read-only rootfs         |
| Data encryption | LUKS2           | Encrypt patient data partition at rest       |
| MAC             | AppArmor        | Confine each service to minimal permissions  |
| Debug lockdown  | Fuse bits       | Disable JTAG/UART debug in production        |
| Kiosk mode      | systemd target  | No shell, no login prompt, UI only           |

### OEM Key Management

- **Signing keys** are kept offline in an HSM (never on the device)
- **Verification keys** (public) are embedded in the FSBL and U-Boot
- **LUKS key** is derived from device-unique hardware ID (OTP fuses)
- Key rotation is handled through OTA updates

### dm-verity Setup

The read-only root filesystem uses dm-verity to detect any modification:

```bash
# During image build (post-image.sh):
veritysetup format rootfs.ext4 rootfs.verity > verity.txt
# Hash is embedded in the kernel command line

# At boot, kernel verifies every block read from rootfs
# Any corruption causes an I/O error -> system reboots to known-good state
```

### AppArmor Profiles

Each service has an AppArmor profile restricting:
- File system access (only /opt/vitals-monitor/data, /tmp/vitals-monitor)
- Network access (only network-service can open sockets)
- Device access (only sensor-service can access /dev/i2c-*, /dev/spidev*)
- Capability restrictions (no CAP_SYS_ADMIN, etc.)

---

## OTA Update Plan (Phase 11)

### Overview

The device uses an **A/B partition scheme** with SWUpdate for atomic,
rollback-safe firmware updates. Updates can be delivered via USB drive
or over WiFi from an update server.

### A/B Partition Scheme

```
SD Card Layout:
  [FSBL-A][FSBL-B][U-Boot][rootfsA][rootfsB][data]
                            ^^^^^^   ^^^^^^
                           active   standby
```

- The active partition runs the current firmware
- Updates are written to the standby partition
- On success, the bootloader swaps active/standby labels
- On failure (3 consecutive failed boots), the bootloader reverts

### Update Package Format

Updates are packaged as `.swu` files (SWUpdate format):

```
update-v1.2.0.swu
  sw-description          # Manifest (versions, checksums, scripts)
  sw-description.sig      # RSA-2048 signature of manifest
  rootfs.ext4.gz          # Compressed root filesystem image
  kernel.itb              # FIT image (kernel + DTB)
  post-install.sh         # Optional post-install script
```

### Update Flow

```
1. Check for update
   network-service polls update server, or user inserts USB
        |
2. Download / copy .swu file
   Written to /tmp (tmpfs) or streamed directly
        |
3. Verify signature
   SWUpdate checks RSA-2048 signature against embedded public key
   REJECT if signature invalid
        |
4. Install to standby partition
   SWUpdate writes rootfs to standby, updates kernel
   Progress reported via ota_get_status()
        |
5. Mark standby as "pending boot"
   U-Boot environment updated: boot_partition = standby
        |
6. Reboot
   U-Boot boots from new partition with boot_count = 1
        |
7. Health check
   watchdog.service verifies all services start within 60s
   On success: mark partition as "confirmed" (boot_count = 0)
   On failure: reboot -> U-Boot reverts to previous partition
```

### Software Interface

The `ota_update.h` API provides a C interface for the UI and network-service:

```c
#include "ota_update.h"

/* Check for updates */
if (ota_check_for_update("https://updates.example.com/vitals/manifest.json")) {
    /* Start the update */
    ota_start_update("https://updates.example.com/vitals/update-v1.2.0.swu");

    /* Monitor progress */
    const ota_status_t *status = ota_get_status();
    printf("State: %d, Progress: %d%%\n", status->state, status->progress_pct);
}

/* Emergency rollback */
ota_rollback();

/* Version info */
printf("Running: %s on partition %s\n",
       ota_get_current_version(),
       ota_get_active_partition());
```

### Rollback Triggers

Automatic rollback occurs if:
- Boot fails 3 consecutive times (U-Boot boot_count mechanism)
- watchdog.service detects critical service failure within 60s of boot
- Kernel panic or filesystem corruption detected

Manual rollback:
- User triggers via Settings > System > Rollback in the UI
- Or via command line: `ota_rollback()` / `swupdate -r`

### Update Server Requirements

The update server is a simple HTTPS file server hosting:

```
/vitals/
  manifest.json           # Current version info + .swu URL
  update-v1.0.0.swu      # Previous release
  update-v1.1.0.swu      # Current release
  update-v1.2.0.swu      # Next release
```

The `manifest.json` format:

```json
{
  "latest_version": "1.2.0",
  "min_version": "1.0.0",
  "url": "https://updates.example.com/vitals/update-v1.2.0.swu",
  "sha256": "abcdef1234567890...",
  "release_notes": "Bug fixes and performance improvements",
  "mandatory": false
}
```

---

## Troubleshooting

### Display Not Working

```bash
# Check framebuffer
cat /sys/class/graphics/fb0/name
cat /sys/class/graphics/fb0/virtual_size
# Expected: 800,480

# Check if UI is running
systemctl status vitals-ui
journalctl -u vitals-ui --no-pager | tail -20
```

### Sensors Not Detected

```bash
# Check I2C devices
i2cdetect -y 1
# Expected: 0x48 (TMP117), 0x57 (MAX30102)

# Check SPI
ls /dev/spidev*
# Expected: /dev/spidev0.0 (ADS1292R), /dev/spidev1.0 (NIBP)

# Check sensor-service logs
journalctl -u sensor-service --no-pager | tail -30
```

### Service Keeps Restarting

```bash
# Check why it failed
systemctl status <service-name>
journalctl -u <service-name> --since "10 min ago"

# Check resource limits
systemctl show <service-name> | grep -E 'Memory|CPU'
```

### OTA Update Fails

```bash
# Check SWUpdate logs
journalctl -u swupdate --no-pager | tail -50

# Check available disk space
df -h

# Verify .swu signature manually
swupdate -c -i /path/to/update.swu

# Check boot partition status
fw_printenv boot_partition
fw_printenv boot_count
```
