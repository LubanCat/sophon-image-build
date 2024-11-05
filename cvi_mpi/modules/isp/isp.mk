ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
	include $(PARAM_FILE)
endif

isp_chip_dir := $(shell echo $(CVIARCH) | tr A-Z a-z)


