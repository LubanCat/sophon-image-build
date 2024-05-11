#!/bin/bash -e

# Directory contains the target rootfs
TARGET_ROOTFS_DIR="binary"

TARGET="lite"

ARCH="riscv64"

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

# overlay-sophgo folder
sudo cp -rpf overlay-sophgo/bin/* $TARGET_ROOTFS_DIR/bin/

sudo cp -rpf overlay-sophgo/sbin/* $TARGET_ROOTFS_DIR/sbin/

sudo cp -rpf overlay-sophgo/etc/* $TARGET_ROOTFS_DIR/etc/

sudo cp -rpf overlay-sophgo/mnt/* $TARGET_ROOTFS_DIR/mnt/

sudo cp -rfd overlay-sophgo/usr/lib/* $TARGET_ROOTFS_DIR/usr/lib/

sudo cp -rfd overlay-sophgo/lib64 $TARGET_ROOTFS_DIR/

sudo cp -rf overlay-sophgo/usr/bin/* $TARGET_ROOTFS_DIR/usr/bin/
sudo cp -rf overlay-sophgo/usr/sbin/* $TARGET_ROOTFS_DIR/usr/sbin/
#sudo cp -rfd overlay-sophgo/usr/lib64v_xthead $TARGET_ROOTFS_DIR/usr/
sudo cp -rfd overlay-sophgo/usr/lib64v0p7_xthead $TARGET_ROOTFS_DIR/usr/

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

EOF

sudo umount $TARGET_ROOTFS_DIR/dev

IMAGE_VERSION=$TARGET ./mk-image.sh 
