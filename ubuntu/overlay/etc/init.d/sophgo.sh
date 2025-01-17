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

if [ ! -e "/boot/boot_init" ] ; then
	ROOT_PART=$(cat /proc/cmdline | sed 's/ /\n/g' | grep root= | awk -F 'root=' '{print $2}'| awk -F '/' '{print $3}')

	ROOT_DEV=${ROOT_PART%p*}

  	PART_NUM=${ROOT_PART#${ROOT_DEV}p}

  	LAST_PART_NUM=$(parted /dev/${ROOT_DEV} -ms unit s p | tail -n 1 | cut -f 1 -d:)
	  
  if [ $LAST_PART_NUM -ne $PART_NUM ]; then
    echo "$ROOT_PART is not the last partition. Don't know how to expand"
  fi

    #/*******************sd启动boot分区设置自动挂载*******************/
    if blkid | grep -q "/dev/${ROOT_DEV}p1"; then
        if ! grep -q "${ROOT_DEV}p1" /etc/fstab ; then
            echo "/dev/${ROOT_DEV}p1  /boot  auto  defaults  0 2" >> /etc/fstab
        fi
        mkdir -p /boot
        mount /dev/${ROOT_DEV}p1 /boot
    else    
        #/*******************emmc启动usr data分区先进行格式化再设置自动挂载*******************/
        if ! grep -q "${ROOT_DEV}p7" /etc/fstab ; then
            mkfs.ext4 "/dev/${ROOT_DEV}p7"
            echo "/dev/${ROOT_DEV}p7  /mnt/data  auto  defaults  0 2" >> /etc/fstab
            mkdir -p /mnt/data
            mount /dev/${ROOT_DEV}p7 /mnt/data
        fi
    fi

    #/*******************扩容*******************/
    if [ ! -e "/boot/boot_dilatation_init" ] ; then
        if blkid | grep -q "boot"; then
            parted  /dev/${ROOT_DEV} <<EOF
unit GB print
resizepart ${LAST_PART_NUM}
-1
yes
quit
EOF
        fi
    resize2fs /dev/$ROOT_PART
    touch /boot/boot_dilatation_init
    fi

    touch /boot/boot_init
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

    #host
    #echo host > /proc/cviusb/otg_role

    #rndis
    /etc/run_usb.sh probe rndis
    echo "root" | su -c '/etc/run_usb.sh start' root
    ifconfig usb0 up

    #adb
    #/etc/run_usb.sh probe adb
    #echo "root" | su -c '/etc/run_usb.sh start' root
fi

if [ ! -e /boot/boot_mac_eth0 ]; then
	mac_address_eth0=$(ifconfig eth0 | grep ether | awk '{ print $2 }')
	echo "$mac_address_eth0" > /boot/boot_mac_eth0
else
	sudo ifconfig eth0 down
	mac_address_eth0=$(cat /boot/boot_mac_eth0)
	sudo ifconfig eth0 hw ether $mac_address_eth0
	sudo ifconfig eth0 up
fi

if ifconfig usb0 > /dev/null 2>&1; then
	if [ ! -e /boot/boot_mac_usb0 ]
	then
		mac_address_rndis=$(ifconfig usb0 | grep ether | awk '{ print $2 }')
		echo "$mac_address_rndis" > /boot/boot_mac_usb0
	else
		sudo ifconfig usb0 down
		mac_address_rndis=$(cat /boot/boot_mac_usb0)
		sudo ifconfig usb0 hw ether $mac_address_rndis
	fi
    sudo ifconfig usb0 192.168.137.10 up
    sudo route add default gw 192.168.137.1 metric 800
fi

systemctl restart NetworkManager

# set the system time from the RTC
#sudo /bin/busybox hwclock -s 
