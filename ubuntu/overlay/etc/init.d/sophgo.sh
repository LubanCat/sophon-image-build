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
	ROOT_PART=$(cat /proc/cmdline | sed 's/ /\n/g' | grep root= | awk -F 'root=' '{print $2}'| awk -F '/' '{print $3}')

	ROOT_DEV=${ROOT_PART%p*}

  	PART_NUM=${ROOT_PART#${ROOT_DEV}p}

  	LAST_PART_NUM=$(parted /dev/${ROOT_DEV} -ms unit s p | tail -n 1 | cut -f 1 -d:)
	  
  if [ $LAST_PART_NUM -ne $PART_NUM ]; then
    echo "$ROOT_PART is not the last partition. Don't know how to expand"
  fi

  parted  /dev/${ROOT_DEV} <<EOF
unit GB print
resizepart ${LAST_PART_NUM}
-1
yes
quit
EOF

    resize2fs /dev/$ROOT_PART

    touch /boot/boot_dilatation_init
fi

if ! grep -q "mmcblk" /etc/fstab ; then

    echo "/dev/${ROOT_DEV}p1  /boot  auto  defaults  0 2" >> /etc/fstab
    reboot
fi

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

    #rndis
    /etc/run_usb.sh probe rndis
    echo "root" | su -c '/etc/run_usb.sh start' root
    ifconfig usb0 up

    #adb
    #/etc/run_usb.sh probe adb
    #echo "root" | su -c '/etc/run_usb.sh start' root
fi

# sync system time
hwclock --systohc
