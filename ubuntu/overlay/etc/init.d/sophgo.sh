#!/bin/bash -e
### BEGIN INIT INFO
# Provides:          rockchip
# Required-Start:
# Required-Stop:
# Default-Start:
# Default-Stop:
# Short-Description:
# Description:       Setup rockchip platform environment
### END INIT INFO

PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

#/etc/init.d/boot_init.sh

if [ ! -e "/boot/boot_dilatation_init" ] ; then
    parted /dev/mmcblk0p4 << EOF
resizepart 1
-1

yes
q
EOF
    resize2fs /dev/mmcblk0p4

    touch /boot/boot_dilatation_init
fi

sleep 3s

if [ -e "/mnt/system/ko/loadsystemko.sh" ] ; then
    chmod 777 /mnt/system/ko/loadsystemko.sh
    /mnt/system/ko/loadsystemko.sh
fi


if [ -e "/etc/uhubon.sh" ] ; then
    /etc/uhubon.sh device
fi

if [ -e "/etc/run_usb.sh" ] ; then

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/system/lib
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64v0p7_xthead/lp64d

    #/etc/run_usb.sh probe adb

    /etc/run_usb.sh probe rndis
    /etc/run_usb.sh start

    ifconfig usb0 up
fi


# sync system time
hwclock --systohc
