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
# Install systemd service units (BUILD-2.5)
#
# Canonical hardened unit files live in deploy/systemd/.  We install them
# here rather than generating inline copies, to eliminate duplication and
# ensure the security directives (User=, ProtectSystem=, NoNewPrivileges=,
# etc.) are always applied consistently.
# =============================================================================
SYSTEMD_UNIT_DIR="${TARGET_DIR}/usr/lib/systemd/system"
DEPLOY_SYSTEMD_DIR="$(dirname "${BR2_EXTERNAL_VITALS_MONITOR_PATH}")/deploy/systemd"
mkdir -p "${SYSTEMD_UNIT_DIR}"

install -m 644 "${DEPLOY_SYSTEMD_DIR}/vitals-ui.service"        "${SYSTEMD_UNIT_DIR}/vitals-ui.service"
install -m 644 "${DEPLOY_SYSTEMD_DIR}/sensor-service.service"   "${SYSTEMD_UNIT_DIR}/sensor-service.service"
install -m 644 "${DEPLOY_SYSTEMD_DIR}/alarm-service.service"    "${SYSTEMD_UNIT_DIR}/alarm-service.service"
install -m 644 "${DEPLOY_SYSTEMD_DIR}/network-service.service"  "${SYSTEMD_UNIT_DIR}/network-service.service"
install -m 644 "${DEPLOY_SYSTEMD_DIR}/watchdog.service"         "${SYSTEMD_UNIT_DIR}/watchdog-monitor.service"

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

# =============================================================================
# Install AppArmor profiles (BUILD-2.6)
#
# Profiles from deploy/security/apparmor/ enforce mandatory access control
# on each vitals-monitor service.  They restrict filesystem, network, and
# device access per IEC 62443 defense-in-depth requirements.
# =============================================================================
DEPLOY_APPARMOR_DIR="$(dirname "${BR2_EXTERNAL_VITALS_MONITOR_PATH}")/deploy/security/apparmor"
APPARMOR_PROFILE_DIR="${TARGET_DIR}/etc/apparmor.d"
mkdir -p "${APPARMOR_PROFILE_DIR}"

install -m 644 "${DEPLOY_APPARMOR_DIR}/vitals-ui"          "${APPARMOR_PROFILE_DIR}/opt.vitals-monitor.bin.vitals-ui"
install -m 644 "${DEPLOY_APPARMOR_DIR}/sensor-service"     "${APPARMOR_PROFILE_DIR}/opt.vitals-monitor.bin.sensor-service"
install -m 644 "${DEPLOY_APPARMOR_DIR}/alarm-service"      "${APPARMOR_PROFILE_DIR}/opt.vitals-monitor.bin.alarm-service"
install -m 644 "${DEPLOY_APPARMOR_DIR}/network-service"    "${APPARMOR_PROFILE_DIR}/opt.vitals-monitor.bin.network-service"
install -m 644 "${DEPLOY_APPARMOR_DIR}/watchdog-monitor"   "${APPARMOR_PROFILE_DIR}/opt.vitals-monitor.bin.watchdog-monitor"

# =============================================================================
# Install iptables firewall rules (BUILD-2.7)
#
# Default-DROP INPUT policy.  Only established/related connections and
# localhost traffic are allowed.  OUTPUT is permitted for FHIR/NTP/OTA.
# =============================================================================
mkdir -p "${TARGET_DIR}/etc/iptables"
cat > "${TARGET_DIR}/etc/iptables/rules.v4" << 'FW_EOF'
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]

# Allow loopback
-A INPUT -i lo -j ACCEPT
-A OUTPUT -o lo -j ACCEPT

# Allow established and related connections
-A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT

# Drop invalid packets
-A INPUT -m conntrack --ctstate INVALID -j DROP

# Allow ICMP ping (rate-limited) for network diagnostics
-A INPUT -p icmp --icmp-type echo-request -m limit --limit 1/s --limit-burst 4 -j ACCEPT

COMMIT
FW_EOF

# Install systemd service to restore iptables rules at boot
cat > "${SYSTEMD_UNIT_DIR}/iptables-restore.service" << 'IPTABLES_EOF'
[Unit]
Description=Restore iptables firewall rules
DefaultDependencies=no
Before=network-pre.target
Wants=network-pre.target

[Service]
Type=oneshot
ExecStart=/usr/sbin/iptables-restore /etc/iptables/rules.v4
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
IPTABLES_EOF

mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
ln -sf /usr/lib/systemd/system/iptables-restore.service \
       "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/iptables-restore.service"

echo ">>> Vitals Monitor post-build: complete"
