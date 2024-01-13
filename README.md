

简体中文 | [English](./README-en.md)

<br>

# 目录

- [目录](#目录)
- [项目简介](#项目简介)
- [SDK目录结构](#sdk目录结构)
- [快速开始](#快速开始)
  - [准备编译环境](#准备编译环境)
  - [获取SDK](#获取sdk)
  - [准备编译工具](#准备编译工具)
  - [编译](#编译)


<br>

# 项目简介
- 本仓库提供[算能科技](https://www.sophgo.com/)端侧芯片`CV181x`和`CV180x`两个系列芯片的软件开发包(SDK)。
- 适用于官方EVB
- 适用于Lubancat

<br><br>

# SDK目录结构
```
.
├── build               // 编译目录，存放编译脚本以及各board差异化配置
├── buildroot-2021.05   // buildroot开源工具
├── freertos            // freertos系统
├── fsbl                // fsbl启动固件，prebuilt形式存在
├── install             // 执行一次完整编译后，各image的存放路径
├── isp_tuning          // 图像效果调试参数存放路径
├── linux_5.10          // 开源linux内核
├── middleware          // 自研多媒体框架，包含so与ko
├── opensbi             // 开源opensbi库
├── ramdisk             // 存放最小文件系统的prebuilt目录
└── u-boot-2021.10      // 开源uboot代码
```

<br><br>

# 快速开始

## 准备编译环境
- 在虚拟机上安装一个ubuntu系统，或者使用本地的ubuntu系统，推荐`Ubuntu 20.04 LTS`
- 安装串口工具： `mobarXterm` 或者 `xshell` 或者其他
- 安装编译依赖的工具:
```
sudo apt install pkg-config
sudo apt install build-essential
sudo apt install ninja-build
sudo apt install automake
sudo apt install autoconf
sudo apt install libtool
sudo apt install wget
sudo apt install curl
sudo apt install git
sudo apt install gcc
sudo apt install libssl-dev
sudo apt install bc
sudo apt install slib
sudo apt install squashfs-tools
sudo apt install android-sdk-libsparse-utils
sudo apt install android-sdk-ext4-utils
sudo apt install jq
sudo apt install cmake
sudo apt install python3-distutils
sudo apt install tclsh
sudo apt install scons
sudo apt install parallel
sudo apt install ssh-client
sudo apt install tree
sudo apt install python3-dev
sudo apt install python3-pip
sudo apt install device-tree-compiler
sudo apt install libssl-dev
sudo apt install ssh
sudo apt install cpio
sudo apt install squashfs-tools
sudo apt install fakeroot
sudo apt install libncurses5
sudo apt install flex
sudo apt install bison
```
*注意：cmake版本最低要求3.16.5*

- cmake要求3.16+：
```
cmake --version
```

- 更新cmake
```
sudo apt autoremove cmake
wget https://cmake.org/files/v3.16/cmake-3.16.5.tar.gz
tar zxvf cmake-3.16.5.tar.gz
cd cmake-3.16.5/
./configure (执行之前注意安装依赖包gcc、g++、build-essential、libssl-dev等)
make
sudo make install
```

## 获取SDK
```
git clone git@gitlab.ebf.local:sophgo/linux/cvi_mmf_sdk.git
```

## 准备编译工具

- 获取工具链
```
wget https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/07/16/host-tools.tar.gz
```
- 解压工具链并链接到SDK目录
```
tar xvf host-tools.tar.gz
cd cvi_mmf_sdk/
ln -s ../host-tools ./
```

## 编译
- 以 `cv1813h_wevb_0007a_emmc_lubancat`为例
```
cd cvi_mmf_sdk/
source build/cvisetup.sh
defconfig cv1813h_wevb_0007a_emmc_lubancat
build_all
```
- 编译成功后可以在`install`目录下看到生成的image