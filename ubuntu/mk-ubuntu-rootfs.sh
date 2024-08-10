#!/bin/bash -e

# Directory contains the target rootfs
TARGET_ROOTFS_DIR="binary"

TARGET="lite"

if [ "$1" == "riscv64" ]; then
    ARCH=riscv64
elif [ "$1" == "armhf" ]; then
    ARCH=armhf
else
    echo "Usage:"
    echo "	$0 <ARCH>"
    exit
fi

#MIRROR="carp-sg200x"

echo -e "\033[47;36m Building for $ARCH \033[0m"

if [ ! $VERSION ]; then
    VERSION="release"
fi

finish() {
    sudo umount $TARGET_ROOTFS_DIR/dev
    exit -1
}
trap finish ERR

echo -e "\033[47;36m Extract image \033[0m"
sudo rm -rf $TARGET_ROOTFS_DIR
sudo tar -xpf ubuntu-base-$TARGET-$ARCH-*.tar.gz

#linux kernel deb
if [ -e ../linux-headers* ]; then
    Image_Deb=$(basename ../linux-headers*)
    sudo mkdir -p $TARGET_ROOTFS_DIR/boot/kerneldeb
    sudo touch $TARGET_ROOTFS_DIR/boot/build-host
    sudo cp -vrpf ../${Image_Deb} $TARGET_ROOTFS_DIR/boot/kerneldeb
    sudo cp -vrpf ../${Image_Deb/headers/image} $TARGET_ROOTFS_DIR/boot/kerneldeb
fi

# overlay folder
sudo cp -rpf overlay/* $TARGET_ROOTFS_DIR/

# overlay-firmware folder
#sudo cp -rpf overlay-firmware/* $TARGET_ROOTFS_DIR/

if [ $ARCH == "riscv64" ]; then
    # overlay-sophgo-riscv64 folder
    sudo cp -rpf overlay-sophgo-riscv64/bin/* $TARGET_ROOTFS_DIR/bin/

    sudo cp -rpf overlay-sophgo-riscv64/sbin/* $TARGET_ROOTFS_DIR/sbin/

    sudo cp -rpf overlay-sophgo-riscv64/etc/* $TARGET_ROOTFS_DIR/etc/

    sudo cp -rpf overlay-sophgo-riscv64/mnt/* $TARGET_ROOTFS_DIR/mnt/

    #sudo cp -rfd overlay-sophgo-riscv64/usr/lib/* $TARGET_ROOTFS_DIR/usr/lib/
    sudo cp -rfd overlay-sophgo-riscv64/usr/lib/firmware $TARGET_ROOTFS_DIR/usr/lib/  

    sudo cp -rfd overlay-sophgo-riscv64/lib64 $TARGET_ROOTFS_DIR/

    sudo cp -rf overlay-sophgo-riscv64/usr/bin/* $TARGET_ROOTFS_DIR/usr/bin/
    sudo cp -rf overlay-sophgo-riscv64/usr/sbin/* $TARGET_ROOTFS_DIR/usr/sbin/
    #sudo cp -rfd overlay-sophgo-riscv64/usr/lib64v_xthead $TARGET_ROOTFS_DIR/usr/
    sudo cp -rfd overlay-sophgo-riscv64/usr/lib64v0p7_xthead $TARGET_ROOTFS_DIR/usr/
    sudo cp -rfd overlay-sophgo-riscv64/lib64v0p7_xthead $TARGET_ROOTFS_DIR/
    
elif [ $ARCH == "armhf" ]; then
    # overlay-sophgo-arm folder
    sudo cp -rpf overlay-sophgo-arm/bin/* $TARGET_ROOTFS_DIR/bin/

    sudo cp -rpf overlay-sophgo-arm/sbin/* $TARGET_ROOTFS_DIR/sbin/

    sudo cp -rpf overlay-sophgo-arm/etc/* $TARGET_ROOTFS_DIR/etc/

    sudo cp -rpf overlay-sophgo-arm/mnt/* $TARGET_ROOTFS_DIR/mnt/

    #sudo cp -rfd overlay-sophgo-arm/usr/lib/* $TARGET_ROOTFS_DIR/usr/lib/  //error
    sudo cp -rfd overlay-sophgo-arm/usr/lib/firmware $TARGET_ROOTFS_DIR/usr/lib/  

    sudo cp -rfd overlay-sophgo-arm/lib64 $TARGET_ROOTFS_DIR/

    sudo cp -rf overlay-sophgo-arm/usr/bin/* $TARGET_ROOTFS_DIR/usr/bin/
    sudo cp -rf overlay-sophgo-arm/usr/sbin/* $TARGET_ROOTFS_DIR/usr/sbin/
    #sudo cp -rfd overlay-sophgo-arm/usr/lib64v_xthead $TARGET_ROOTFS_DIR/usr/
    #sudo cp -rfd overlay-sophgo-arm/usr/lib64v0p7_xthead $TARGET_ROOTFS_DIR/usr/
fi

if [ "$ARCH" == "riscv64" ]; then
    sudo cp -b /usr/bin/qemu-riscv64-static "$TARGET_ROOTFS_DIR/usr/bin/"
elif [ "$ARCH" == "armhf" ]; then
    sudo cp -b /usr/bin/qemu-arm-static "$TARGET_ROOTFS_DIR/usr/bin/"
else
    echo "Unsupported framework"
    exit -1
fi

echo -e "\033[47;36m Change root.....................\033[0m"

sudo mount -o bind /dev $TARGET_ROOTFS_DIR/dev

ID=$(stat --format %u $TARGET_ROOTFS_DIR)

cat << EOF | sudo chroot $TARGET_ROOTFS_DIR

# Fixup owners
#if [ "$ID" -ne 0 ]; then
#    find / -user $ID -exec chown -h 0:0 {} \;
#fi

for u in \$(ls /home/); do
    chown -h -R \$u:\$u /home/\$u
done

mount -t proc proc /proc
mount -t sysfs sys /sys

if [ -e "/usr/lib64v0p7_xthead/lp64d/libc.so" ] ; then
    ln -sf /usr/lib64v0p7_xthead/lp64d/libc.so ld-musl-riscv64v0p7_xthead.so.1
    ln -sf /usr/lib64v0p7_xthead/lp64d/libc.so /lib/ld-musl-riscv64v_xthead.so.1
fi

if [ $MIRROR ]; then
	mkdir -p /etc/apt/keyrings
	curl -fsSL https://Embedfire.github.io/keyfile | gpg --dearmor -o /etc/apt/keyrings/embedfire.gpg
	chmod a+r /etc/apt/keyrings/embedfire.gpg
	echo "deb [arch=riscv64 signed-by=/etc/apt/keyrings/embedfire.gpg] https://cloud.embedfire.com/mirrors/ebf-debian $MIRROR main" | tee /etc/apt/sources.list.d/embedfire-$MIRROR.list > /dev/null
fi

echo 'export LD_LIBRARY_PATH="/usr/lib:/usr/local/lib:/mnt/system/lib:/usr/lib64v0p7_xthead/lp64d/"' >> /etc/profile
echo 'export PATH="/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin:/mnt/system/usr/bin"' >> /etc/profile

export LC_ALL=C.UTF-8

apt-get update
apt-get upgrade -y

chmod o+x /usr/lib/dbus-1.0/dbus-daemon-launch-helper
chmod +x /etc/rc.local

export APT_INSTALL="apt-get install -fy --allow-downgrades"

echo -e "\033[47;36m ---------- LubanCat -------- \033[0m"
\${APT_INSTALL} u-boot-tools logrotate


apt install -fy --allow-downgrades /boot/kerneldeb/* || true

echo -e "\033[47;36m ------- Custom Script ------- \033[0m"
systemctl mask systemd-networkd-wait-online.service
systemctl mask NetworkManager-wait-online.service
systemctl disable hostapd
rm /lib/systemd/system/wpa_supplicant@.service

systemctl enable sophgo.service

systemctl disable apt-daily-upgrade.timer
systemctl disable apt-daily.timer
systemctl disable apt-daily.service
systemctl disable apt-daily-upgrade.service

systemctl disable networkd-dispatcher.service

sudo systemctl mask alsa-restore.service
sudo systemctl mask modprobe@drm.service
sudo systemctl mask modprobe@configfs.service
sudo systemctl mask modprobe@fuse.service
sudo systemctl mask modprobe@efi_pstore.service

sed -i 's/^ProtectHostname=yes/# ProtectHostname=yes/' /usr/lib/systemd/system/systemd-udevd.service

echo -e "\033[47;36m  ---------- Clean ----------- \033[0m"
if [ -e "/usr/lib/arm-linux-gnueabihf/dri" ] ;
then
        cd /usr/lib/arm-linux-gnueabihf/dri/
        cp kms_swrast_dri.so swrast_dri.so /
        rm /usr/lib/arm-linux-gnueabihf/dri/*.so
        mv /*.so /usr/lib/arm-linux-gnueabihf/dri/
elif [ -e "/usr/lib/aarch64-linux-gnu/dri" ];
then
        cd /usr/lib/aarch64-linux-gnu/dri/
        cp kms_swrast_dri.so swrast_dri.so /
        rm /usr/lib/aarch64-linux-gnu/dri/*.so
        mv /*.so /usr/lib/aarch64-linux-gnu/dri/
        rm /etc/profile.d/qt.sh
fi
rm -rf /home/$(whoami)
rm -rf /var/lib/apt/lists/*
rm -rf /var/cache/
rm -rf /packages/
rm -rf /boot/*

umount /proc
umount /sys

EOF

sudo umount $TARGET_ROOTFS_DIR/dev

IMAGE_VERSION=$TARGET ./mk-image.sh 
