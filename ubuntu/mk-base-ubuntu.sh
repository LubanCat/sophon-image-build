#!/bin/bash -e

TARGET="lite"

TARGET_ROOTFS_DIR="binary"

if [ "$1" == "riscv64" ]; then
    ARCH=riscv64
elif [ "$1" == "armhf" ]; then
    ARCH=armhf
elif [ "$1" == "arm64" ]; then
    ARCH=arm64
else
    echo "Usage:"
    echo "	$0 <ARCH>"
    exit
fi

sudo rm -rf $TARGET_ROOTFS_DIR/

if [ ! -d $TARGET_ROOTFS_DIR ] ; then
    sudo mkdir -p $TARGET_ROOTFS_DIR

    if [ ! -e ubuntu-base-22.04.4-base-$ARCH.tar.gz ]; then
        echo "\033[36m wget ubuntu-base-22.04.4-base-"$ARCH".tar.gz \033[0m"
        wget -c http://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/ubuntu-base-22.04.4-base-$ARCH.tar.gz
    fi
    sudo tar -xzf ubuntu-base-22.04.4-base-$ARCH.tar.gz -C $TARGET_ROOTFS_DIR/
    sudo cp -b /etc/resolv.conf $TARGET_ROOTFS_DIR/etc/resolv.conf
    sudo cp sources.list $TARGET_ROOTFS_DIR/etc/apt/sources.list

    if [ $ARCH==riscv64 ];then
	    sudo cp -b /usr/bin/qemu-riscv64-static $TARGET_ROOTFS_DIR/usr/bin/
    elif [ $ARCH==armhf ];then
        sudo cp -b /usr/bin/qemu-arm-static $TARGET_ROOTFS_DIR/usr/bin/
    elif [ $ARCH==arm64 ];then
        sudo cp -b /usr/bin/qemu-aarch64-static $TARGET_ROOTFS_DIR/usr/bin/
    else
        echo "Unsupported framework"
        exit -1
    fi
fi

finish() {
    ./ch-mount.sh -u $TARGET_ROOTFS_DIR
    echo -e "error exit"
    exit -1
}
trap finish ERR

echo -e "\033[47;36m Change root.................... \033[0m"

./ch-mount.sh -m $TARGET_ROOTFS_DIR

cat <<EOF | sudo chroot $TARGET_ROOTFS_DIR/

export APT_INSTALL="apt-get install -fy --allow-downgrades"

export LC_ALL=C.UTF-8

apt-get -y update
apt-get -f -y upgrade

if [ "$TARGET" == "xfce" ]; then
    DEBIAN_FRONTEND=noninteractive apt install -y xubuntu-core onboard rsyslog sudo dialog apt-utils ntp evtest udev
    mv /var/lib/dpkg/info/ /var/lib/dpkg/info_old/
    mkdir /var/lib/dpkg/info/
    apt-get update
    DEBIAN_FRONTEND=noninteractive apt install -y xubuntu-core onboard rsyslog sudo dialog apt-utils ntp evtest udev
    mv /var/lib/dpkg/info_old/* /var/lib/dpkg/info/
elif [ "$TARGET" == "lite" ]; then
    DEBIAN_FRONTEND=noninteractive apt install -y rsyslog sudo dialog apt-utils ntp evtest acpid
fi

\${APT_INSTALL} net-tools openssh-server ifupdown alsa-utils ntp network-manager inetutils-ping libssl-dev \
    vsftpd tcpdump i2c-tools strace vim iperf3 ethtool netplan.io toilet htop pciutils usbutils curl \
    whiptail gnupg bc gdisk parted sox libsox-fmt-all gpiod libgpiod-dev \
    u-boot-tools bsdmainutils file fdisk bluez

HOST=lubancat

# Create User
useradd -G sudo -m -s /bin/bash cat
passwd cat <<IEOF
temppwd
temppwd
IEOF
gpasswd -a cat video
gpasswd -a cat audio
passwd root <<IEOF
root
root
IEOF

# allow root login
sed -i '/pam_securetty.so/s/^/# /g' /etc/pam.d/login

# hostname
echo lubancat > /etc/hostname

# set localtime
ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime

# group
groupadd -f --system cvi
groupadd -f --system gpio
adduser cat cvi
adduser cat gpio
adduser cat i2c

# workaround 90s delay
services=(NetworkManager systemd-networkd)
for service in ${services[@]}; do
  systemctl mask ${service}-wait-online.service
done

# disbale the wire/nl80211
systemctl mask wpa_supplicant-wired@
systemctl mask wpa_supplicant-nl80211@
systemctl mask wpa_supplicant@

# Make systemd less spammy

sed -i 's/#LogLevel=info/LogLevel=warning/' \
  /etc/systemd/system.conf

sed -i 's/#LogTarget=journal-or-kmsg/LogTarget=journal/' \
  /etc/systemd/system.conf

# check to make sure sudoers file has ref for the sudo group
SUDOEXISTS="$(awk '$1 == "%sudo" { print $1 }' /etc/sudoers)"
if [ -z "$SUDOEXISTS" ]; then
  # append sudo entry to sudoers
  echo "# Members of the sudo group may gain root privileges" >> /etc/sudoers
  echo "%sudo	ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
fi

# make sure that NOPASSWD is set for %sudo
# expecially in the case that we didn't add it to /etc/sudoers
# just blow the %sudo line away and force it to be NOPASSWD
sed -i -e '
/\%sudo/ c \
%sudo    ALL=(ALL) NOPASSWD: ALL
' /etc/sudoers

apt-get clean
rm -rf /var/lib/apt/lists/*

sync

EOF

./ch-mount.sh -u $TARGET_ROOTFS_DIR

DATE=$(date +%Y%m%d)
echo -e "\033[47;36m Run tar pack ubuntu-base-$TARGET-$ARCH-$DATE.tar.gz \033[0m"
sudo tar zcf ubuntu-base-$TARGET-$ARCH-$DATE.tar.gz $TARGET_ROOTFS_DIR

# sudo rm $TARGET_ROOTFS_DIR -r

echo -e "\033[47;36m normal exit \033[0m"
