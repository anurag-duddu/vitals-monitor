################################################################################
#
# sensor-service
#
################################################################################

SENSOR_SERVICE_VERSION = 1.0.0
SENSOR_SERVICE_SITE = $(realpath $(BR2_EXTERNAL_VITALS_MONITOR_PATH)/../src)
SENSOR_SERVICE_SITE_METHOD = local
SENSOR_SERVICE_LICENSE = Proprietary
SENSOR_SERVICE_LICENSE_FILES = ../LICENSE
SENSOR_SERVICE_INSTALL_STAGING = NO
SENSOR_SERVICE_INSTALL_TARGET = YES

SENSOR_SERVICE_DEPENDENCIES = \
	sqlite \
	nanomsg \
	cjson

SENSOR_SERVICE_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DVM_TARGET_HARDWARE=ON \
	-DVM_BUILD_COMPONENT=sensor-service \
	-DVM_INSTALL_PREFIX=/opt/vitals-monitor

define SENSOR_SERVICE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/sensor-service \
		$(TARGET_DIR)/opt/vitals-monitor/bin/sensor-service
endef

$(eval $(cmake-package))
