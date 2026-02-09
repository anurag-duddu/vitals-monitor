################################################################################
#
# watchdog-monitor
#
################################################################################

WATCHDOG_MONITOR_VERSION = 1.0.0
WATCHDOG_MONITOR_SITE = $(realpath $(BR2_EXTERNAL_VITALS_MONITOR_PATH)/../src)
WATCHDOG_MONITOR_SITE_METHOD = local
WATCHDOG_MONITOR_LICENSE = Proprietary
WATCHDOG_MONITOR_LICENSE_FILES = ../LICENSE
WATCHDOG_MONITOR_INSTALL_STAGING = NO
WATCHDOG_MONITOR_INSTALL_TARGET = YES

WATCHDOG_MONITOR_DEPENDENCIES = \
	nanomsg

WATCHDOG_MONITOR_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DVM_TARGET_HARDWARE=ON \
	-DVM_BUILD_COMPONENT=watchdog-monitor \
	-DVM_INSTALL_PREFIX=/opt/vitals-monitor

define WATCHDOG_MONITOR_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/watchdog-monitor \
		$(TARGET_DIR)/opt/vitals-monitor/bin/watchdog-monitor
endef

$(eval $(cmake-package))
