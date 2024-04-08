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

/etc/init.d/boot_init.sh

sleep 3s

# first boot configure
if [ ! -e "/usr/local/first_boot_flag" ] ;
then
    echo "It's the first time booting."
    echo "The rootfs will be configured."

    Mem_Size=$(free -m | grep Mem | awk '{print $2}')
    if [ '1500' -gt $Mem_Size  ] ;
    then
        echo 'Mem_Size =' $Mem_Size 'MB , make swap memory '
        swapoff -a
        dd if=/dev/zero of=/var/swapfile bs=1M count=200
        mkswap /var/swapfile
        swapon /var/swapfile
        echo "/var/swapfile swap swap defaults 0 0" >> /etc/fstab
    fi

    # Force rootfs synced
    mount -o remount,sync /

    #if [ -e "/dev/rfkill" ] ; then
    #   rm /dev/rfkill
    #fi

    rm -rf /*.deb
    rm -rf /*.tar
fi

# sync system time
hwclock --systohc
