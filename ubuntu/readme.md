## 简介

A set of shell scripts that will build GNU/Linux distribution rootfs image
for rockchip platform.

## 适用板卡

- 使用sg200x处理器的LubanCat板卡(不支持桌面)

## 安装依赖

推荐使用Ubuntu20.04及以上版本主机构建根文件系统

* ubuntu 22.04 (Jammy-X11)

```
sudo apt-get install binfmt-support qemu-user-static
sudo dpkg -i ubuntu-build-service/packages/*
sudo apt-get install -f
```

## 构建 Ubuntu22.04镜像（仅支持64bit）

- lite：控制台版，无桌面

#### step1.构建基础 Ubuntu 系统。

```
# 运行以下脚本，根据提示选择要构建的版本
./mk-base-ubuntu.sh
```
#### step2.添加 overlay 层,并打包ubuntu-rootfs镜像

```
# 运行以下脚本，根据提示选择要构建处理器版本和ubuntu的版本
./mk-ubuntu-rootfs.sh
```