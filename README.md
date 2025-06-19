# LubanCat-sg200x sdk

# download source

```
git clone --depth=1 -b sg200x https://github.com/LubanCat/sophon-image-build.git

cd sophon-image-build

git clone https://github.com/sophgo/host-tools --depth=1
```

# build

```
source build/cvisetup.sh

# build riscv ubuntu emmc
defconfig sg2000_lubancat_riscv_ubuntu_emmc
build_all


# clean_all
# rm ubuntu/ubuntu-rootfs.ext4


# build arm64 ubuntu sd
defconfig sg2000_lubancat_arm64_ubuntu_sd
build_all
........

```


# update

|                  |   URL                                  | Branch         |    Commit      |
|:----------------:|:---------------------------------------|:---------------|:---------------|
| linux_5.10       | https://github.com/sophgo/linux_5.10          | sg200x-dev     | e1b5c29  |
| cvi_mpi          | https://github.com/sophgo/cvi_mpi.git         | sg200x-dev     | df2364e |
| SensorSupportList| https://github.com/sophgo/SensorSupportList.git | sg200x-dev   | 3666db3 |
| cvi_rtsp	   | https://github.com/sophgo/cvi_rtsp.git	   | sg200x-dev     | 9f66540   |
| tdl_sdk          | https://github.com/sophgo/tdl_sdk             | v1             | 30301dd   |
| osdrv            | https://github.com/sophgo/osdrv.git           | sg200x-dev     | c7aeabf   |
| rmadisk          | https://github.com/sophgo/ramdisk.git         | sg200x-dev     | 8bf2a74   |
| cvibuilder       | https://github.com/sophgo/cvibuilder.git      | sg200x-dev     | 4309f2a |
| cvikernel        | https://github.com/sophgo/cvikernel.git       | sg200x-dev     | 9f1f57a  |
| cviruntime       | https://github.com/sophgo/cviruntime.git      | sg200x-dev     | 3f49386  |
| cvimath          | https://github.com/sophgo/cvimath.git         | sg200x-dev     | ce8705f  |
| cnpy             | https://github.com/sophgo/cnpy.git	           | tpu	        | 2f56f4c  |
| flatbuffers      | https://github.com/sophgo/flatbuffers.git     | master         | 6da1cf7  |
| fsbl             | https://github.com/sophgo/fsbl     | sg200x-dev  | 69853ff |
| opensbi          | https://github.com/sophgo/opensbi  | sg200x-dev  | 3b045ae |
| u-boot-2021.10   | https://github.com/sophgo/u-boot-2021.10   | sg200x-dev | 4a21b6b |
| oss              | https://github.com/sophgo/oss.git          | master        | 53c237e   |


