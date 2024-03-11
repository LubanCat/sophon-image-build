inst_mod() {
  insmod /mnt/system/ko/configfs.ko
  insmod /mnt/system/ko/libcomposite.ko
  insmod /mnt/system/ko/u_serial.ko
  insmod /mnt/system/ko/usb_f_acm.ko
  insmod /mnt/system/ko/cvi_usb_f_cvg.ko
  insmod /mnt/system/ko/usb_f_uvc.ko
  insmod /mnt/system/ko/usb_f_fs.ko
  insmod /mnt/system/ko/u_audio.ko
  insmod /mnt/system/ko/usb_f_uac1.ko
  insmod /mnt/system/ko/usb_f_serial.ko
  insmod /mnt/system/ko/usb_f_mass_storage.ko
  insmod /mnt/system/ko/u_ether.ko
  insmod /mnt/system/ko/usb_f_ecm.ko
  insmod /mnt/system/ko/usb_f_eem.ko
  insmod /mnt/system/ko/usb_f_rndis.ko
}

case "$1" in
  host)
	insmod /mnt/system/ko/dwc2.ko
	;;
  device)
	inst_mod
	echo device > /proc/cviusb/otg_role
	;;
  *)
	echo "Usage: $0 host"
	echo "Usage: $0 device"
	exit 1
esac
exit $?
