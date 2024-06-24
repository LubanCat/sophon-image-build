# cvi_mmf_sdk

# download source

```
git clone xxx/cvi_mmf_sdk --depth=1
cd cvi_mmf_sdk_lubancat
git clone https://github.com/sophgo/host-tools --depth=1
```

# build it

```
source build/cvisetup.sh
defconfig sg200x
# C906:
defconfig sg2000_lubancat_riscv_ubuntu_sd
# A53:
# defconfig sg2000_lubancat_arm_ubuntu_sd
build_all
```
