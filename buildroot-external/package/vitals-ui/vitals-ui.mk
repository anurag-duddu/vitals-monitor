################################################################################
#
# vitals-ui
#
################################################################################

VITALS_UI_VERSION = 1.0.0
VITALS_UI_SITE = $(realpath $(BR2_EXTERNAL_VITALS_MONITOR_PATH)/../src)
VITALS_UI_SITE_METHOD = local
VITALS_UI_LICENSE = Proprietary
VITALS_UI_LICENSE_FILES = ../LICENSE
VITALS_UI_INSTALL_STAGING = NO
VITALS_UI_INSTALL_TARGET = YES

VITALS_UI_DEPENDENCIES = \
	sqlite \
	nanomsg \
	lvgl \
	lv_drivers \
	libdrm \
	libinput \
	tslib \
	cjson

VITALS_UI_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DVM_TARGET_HARDWARE=ON \
	-DVM_DISPLAY_BACKEND=DRM \
	-DVM_INSTALL_PREFIX=/opt/vitals-monitor \
	-DVM_SCREEN_WIDTH=800 \
	-DVM_SCREEN_HEIGHT=480

define VITALS_UI_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/vitals-ui \
		$(TARGET_DIR)/opt/vitals-monitor/bin/vitals-ui
endef

$(eval $(cmake-package))
