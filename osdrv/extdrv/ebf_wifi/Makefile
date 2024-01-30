#
# Makefile for the Linux Wireless network device drivers.
#
-include $(BUILD_PATH)/.config

.PHONY : all clean

CONFIG_WIFI_RTL8821CS=y
CONFIG_WIFI_RTL8723DS=n

all:
ifeq ($(CONFIG_WIFI_RTL8821CS),y)
	$(warning "CONFIG_WIFI_RTL8821CS ----")
	@if [ -d sdio_rtl8821cs ];then cd sdio_rtl8821cs; make;fi
endif

ifeq ($(CONFIG_WIFI_RTL8723DS),y)
	$(warning "CONFIG_WIFI_RTL8723DS ----")
	@if [ -d sdio_rtl8723ds ];then cd sdio_rtl8723ds; make;fi
endif

clean:
	@if [ -d sdio_rtl8821cs ];then cd sdio_rtl8821cs;  make clean;fi
	@if [ -d sdio_rtl8723ds ];then cd sdio_rtl8723ds;  make clean;fi