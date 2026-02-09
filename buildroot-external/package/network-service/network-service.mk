################################################################################
#
# network-service
#
################################################################################

NETWORK_SERVICE_VERSION = 1.0.0
NETWORK_SERVICE_SITE = $(realpath $(BR2_EXTERNAL_VITALS_MONITOR_PATH)/../src)
NETWORK_SERVICE_SITE_METHOD = local
NETWORK_SERVICE_LICENSE = Proprietary
NETWORK_SERVICE_LICENSE_FILES = ../LICENSE
NETWORK_SERVICE_INSTALL_STAGING = NO
NETWORK_SERVICE_INSTALL_TARGET = YES

NETWORK_SERVICE_DEPENDENCIES = \
	sqlite \
	nanomsg \
	mbedtls \
	cjson

NETWORK_SERVICE_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DVM_TARGET_HARDWARE=ON \
	-DVM_BUILD_COMPONENT=network-service \
	-DVM_INSTALL_PREFIX=/opt/vitals-monitor

define NETWORK_SERVICE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/network-service \
		$(TARGET_DIR)/opt/vitals-monitor/bin/network-service
endef

$(eval $(cmake-package))
