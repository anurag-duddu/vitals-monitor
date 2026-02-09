################################################################################
#
# alarm-service
#
################################################################################

ALARM_SERVICE_VERSION = 1.0.0
ALARM_SERVICE_SITE = $(realpath $(BR2_EXTERNAL_VITALS_MONITOR_PATH)/../src)
ALARM_SERVICE_SITE_METHOD = local
ALARM_SERVICE_LICENSE = Proprietary
ALARM_SERVICE_LICENSE_FILES = ../LICENSE
ALARM_SERVICE_INSTALL_STAGING = NO
ALARM_SERVICE_INSTALL_TARGET = YES

ALARM_SERVICE_DEPENDENCIES = \
	sqlite \
	nanomsg \
	cjson

ALARM_SERVICE_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DVM_TARGET_HARDWARE=ON \
	-DVM_BUILD_COMPONENT=alarm-service \
	-DVM_INSTALL_PREFIX=/opt/vitals-monitor

define ALARM_SERVICE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/alarm-service \
		$(TARGET_DIR)/opt/vitals-monitor/bin/alarm-service
endef

$(eval $(cmake-package))
