#
# Makefile for the Linux Wireless network device drivers.
#
-include $(BUILD_PATH)/.config

.PHONY : all clean

CONFIG_WIFI_RTL8821CS=y

all:
ifeq ($(CONFIG_WIFI_RTL8821CS),y)
	$(warning "CONFIG_WIFI_RTL8821CS ----")
	@if [ -d sdio_rtl8821cs ];then cd sdio_rtl8821cs; make;fi
endif

clean:
	@if [ -d sdio_rtl8821cs ];then cd sdio_rtl8821cs;  make clean;fi
