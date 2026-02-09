#!/bin/bash
# =============================================================================
# Vitals Monitor - Post-build script
# Called by Buildroot after building all packages, before creating rootfs image.
# Installs systemd units, configs, and creates required directory structure.
#
# Arguments:
#   $1 = TARGET_DIR (the target rootfs directory)
#
# Environment:
#   BR2_EXTERNAL_VITALS_MONITOR_PATH - path to our external tree
#   TARGET_DIR                       - alias for $1
# =============================================================================

set -euo pipefail

TARGET_DIR="${1}"
BOARD_DIR="${BR2_EXTERNAL_VITALS_MONITOR_PATH}/board/vitals-monitor"

echo ">>> Vitals Monitor post-build: configuring target filesystem"

# =============================================================================
# Create application directory structure
# =============================================================================
mkdir -p "${TARGET_DIR}/opt/vitals-monitor/bin"
mkdir -p "${TARGET_DIR}/opt/vitals-monitor/lib"
mkdir -p "${TARGET_DIR}/opt/vitals-monitor/etc"
mkdir -p "${TARGET_DIR}/opt/vitals-monitor/data/db"
mkdir -p "${TARGET_DIR}/opt/vitals-monitor/data/logs"
mkdir -p "${TARGET_DIR}/opt/vitals-monitor/data/audit"
mkdir -p "${TARGET_DIR}/opt/vitals-monitor/data/updates"

# =============================================================================
# Install systemd service units
# =============================================================================
SYSTEMD_UNIT_DIR="${TARGET_DIR}/usr/lib/systemd/system"
mkdir -p "${SYSTEMD_UNIT_DIR}"

# --- vitals-ui.service ---
cat > "${SYSTEMD_UNIT_DIR}/vitals-ui.service" << 'UNIT_EOF'
[Unit]
Description=Vitals Monitor UI (LVGL/DRM)
After=multi-user.target sensor-service.service alarm-service.service
Requires=sensor-service.service alarm-service.service
StartLimitIntervalSec=60
StartLimitBurst=5

[Service]
Type=simple
ExecStart=/opt/vitals-monitor/bin/vitals-ui
WorkingDirectory=/opt/vitals-monitor
Environment=XDG_RUNTIME_DIR=/run/user/0
Environment=VM_DATA_DIR=/opt/vitals-monitor/data
Restart=on-failure
RestartSec=3
WatchdogSec=30
MemoryMax=128M
CPUWeight=200
StandardOutput=journal
StandardError=journal
SyslogIdentifier=vitals-ui

[Install]
WantedBy=multi-user.target
UNIT_EOF

# --- sensor-service.service ---
cat > "${SYSTEMD_UNIT_DIR}/sensor-service.service" << 'UNIT_EOF'
[Unit]
Description=Vitals Monitor Sensor Acquisition Service
After=local-fs.target
StartLimitIntervalSec=60
StartLimitBurst=5

[Service]
Type=notify
ExecStart=/opt/vitals-monitor/bin/sensor-service
WorkingDirectory=/opt/vitals-monitor
Environment=VM_DATA_DIR=/opt/vitals-monitor/data
Restart=on-failure
RestartSec=2
WatchdogSec=10
MemoryMax=64M
CPUWeight=300
Nice=-10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=sensor-service

[Install]
WantedBy=multi-user.target
UNIT_EOF

# --- alarm-service.service ---
cat > "${SYSTEMD_UNIT_DIR}/alarm-service.service" << 'UNIT_EOF'
[Unit]
Description=Vitals Monitor Alarm Engine Service
After=sensor-service.service
Requires=sensor-service.service
StartLimitIntervalSec=60
StartLimitBurst=5

[Service]
Type=notify
ExecStart=/opt/vitals-monitor/bin/alarm-service
WorkingDirectory=/opt/vitals-monitor
Environment=VM_DATA_DIR=/opt/vitals-monitor/data
Restart=on-failure
RestartSec=2
WatchdogSec=10
MemoryMax=32M
CPUWeight=250
Nice=-5
StandardOutput=journal
StandardError=journal
SyslogIdentifier=alarm-service

[Install]
WantedBy=multi-user.target
UNIT_EOF

# --- network-service.service ---
cat > "${SYSTEMD_UNIT_DIR}/network-service.service" << 'UNIT_EOF'
[Unit]
Description=Vitals Monitor Network & Sync Service
After=network-online.target sensor-service.service
Wants=network-online.target
Requires=sensor-service.service
StartLimitIntervalSec=120
StartLimitBurst=3

[Service]
Type=notify
ExecStart=/opt/vitals-monitor/bin/network-service
WorkingDirectory=/opt/vitals-monitor
Environment=VM_DATA_DIR=/opt/vitals-monitor/data
Restart=on-failure
RestartSec=10
WatchdogSec=60
MemoryMax=64M
StandardOutput=journal
StandardError=journal
SyslogIdentifier=network-service

[Install]
WantedBy=multi-user.target
UNIT_EOF

# --- watchdog-monitor.service ---
cat > "${SYSTEMD_UNIT_DIR}/watchdog-monitor.service" << 'UNIT_EOF'
[Unit]
Description=Vitals Monitor Hardware Watchdog Service
DefaultDependencies=no
After=local-fs.target
Before=multi-user.target

[Service]
Type=simple
ExecStart=/opt/vitals-monitor/bin/watchdog-monitor
Restart=always
RestartSec=1
WatchdogSec=5
MemoryMax=8M
CPUWeight=400
Nice=-20
StandardOutput=journal
StandardError=journal
SyslogIdentifier=watchdog-monitor

[Install]
WantedBy=sysinit.target
UNIT_EOF

# =============================================================================
# Enable services via symlinks
# =============================================================================
WANTS_DIR="${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
SYSINIT_WANTS="${TARGET_DIR}/etc/systemd/system/sysinit.target.wants"
mkdir -p "${WANTS_DIR}"
mkdir -p "${SYSINIT_WANTS}"

ln -sf /usr/lib/systemd/system/vitals-ui.service      "${WANTS_DIR}/vitals-ui.service"
ln -sf /usr/lib/systemd/system/sensor-service.service  "${WANTS_DIR}/sensor-service.service"
ln -sf /usr/lib/systemd/system/alarm-service.service   "${WANTS_DIR}/alarm-service.service"
ln -sf /usr/lib/systemd/system/network-service.service "${WANTS_DIR}/network-service.service"
ln -sf /usr/lib/systemd/system/watchdog-monitor.service "${SYSINIT_WANTS}/watchdog-monitor.service"

# =============================================================================
# Install application configuration
# =============================================================================
cat > "${TARGET_DIR}/opt/vitals-monitor/etc/vitals-monitor.conf" << 'CONF_EOF'
# Vitals Monitor Configuration
# This file is read by all services at startup.

[general]
data_dir = /opt/vitals-monitor/data
log_level = info

[display]
# DRM device for LVGL framebuffer output
drm_device = /dev/dri/card0
# Touchscreen input device
input_device = /dev/input/event0
# Target resolution
width = 800
height = 480
# Frame rate target (Hz)
fps = 30

[database]
path = /opt/vitals-monitor/data/db/vitals_trends.db
wal_mode = true
# Raw data retention (seconds) - default 4 hours
raw_retention_s = 14400
# Aggregated data retention (seconds) - default 72 hours
agg_retention_s = 259200

[ipc]
# Nanomsg IPC endpoints
sensor_pub = ipc:///run/vitals-monitor/sensor.ipc
alarm_pub = ipc:///run/vitals-monitor/alarm.ipc
command_req = ipc:///run/vitals-monitor/command.ipc

[network]
# FHIR server endpoint (empty = disabled)
fhir_endpoint =
# TLS certificate paths
ca_cert = /opt/vitals-monitor/etc/ca-bundle.crt
client_cert =
client_key =

[watchdog]
# Hardware watchdog device
device = /dev/watchdog0
# Watchdog timeout (seconds)
timeout_s = 15
# Kick interval (seconds) - must be < timeout_s
kick_interval_s = 5
CONF_EOF

# =============================================================================
# Configure data partition mount
# =============================================================================
cat > "${TARGET_DIR}/usr/lib/systemd/system/opt-vitals\\x2dmonitor-data.mount" << 'MOUNT_EOF'
[Unit]
Description=Vitals Monitor Data Partition
DefaultDependencies=no
Before=local-fs.target

[Mount]
What=/dev/disk/by-partlabel/data
Where=/opt/vitals-monitor/data
Type=ext4
Options=defaults,noatime,commit=60

[Install]
WantedBy=local-fs.target
MOUNT_EOF

mkdir -p "${TARGET_DIR}/etc/systemd/system/local-fs.target.wants"
ln -sf "/usr/lib/systemd/system/opt-vitals\\x2dmonitor-data.mount" \
       "${TARGET_DIR}/etc/systemd/system/local-fs.target.wants/opt-vitals\\x2dmonitor-data.mount"

# =============================================================================
# Create tmpfiles.d entry for runtime directory
# =============================================================================
mkdir -p "${TARGET_DIR}/usr/lib/tmpfiles.d"
cat > "${TARGET_DIR}/usr/lib/tmpfiles.d/vitals-monitor.conf" << 'TMPFILES_EOF'
# Create runtime directory for IPC sockets
d /run/vitals-monitor 0755 root root -
TMPFILES_EOF

# =============================================================================
# Sysctl: increase inotify watches, RT priority limits
# =============================================================================
mkdir -p "${TARGET_DIR}/etc/sysctl.d"
cat > "${TARGET_DIR}/etc/sysctl.d/99-vitals-monitor.conf" << 'SYSCTL_EOF'
# Allow real-time scheduling for vitals services
kernel.sched_rt_runtime_us = 950000
# Increase inotify watchers
fs.inotify.max_user_watches = 8192
SYSCTL_EOF

# =============================================================================
# Set hostname
# =============================================================================
echo "vitals-monitor" > "${TARGET_DIR}/etc/hostname"

echo ">>> Vitals Monitor post-build: complete"
