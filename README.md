# Artifact repository for HybridTier (ASPLOS 25)

HybridTier: An Adaptive and Lightweight CXL-Memory Tiering System

> Kevin Song, Jiacheng Yang, Zixuan Wang, Jishen Zhao, Sihang Liu, Gennady Pekhimenko
> International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS), 2025


## Kernel setup
This section assumes you have a two NUMA node system to emulate fast-tier and slow-tier CXL memory. 
If you have a different hardware platform, e.g. one with real CXL memory or persistent memory, please see [this section](#porting-hybridtier-to-other-hardware-platforms). 

While the HybridTier runtime does not require a specific kernel, emulating CXL memory using remote NUMA node requires a few minor modifications to the Linux kernel.
To setup HybridTier, there are two options:
1. Use the kernel provided (v6.2), with modifications already in-place. 
2. Use your own kernel, and make modifications yourself.

### Approach 1
The kernel source code is provided here: https://github.com/kevins981/linux/commits/v6.2-hybridtier/ (make sure to use the v6.2-hybridtier branch). This is the kernel we used for all evaluations in the paper. 

```bash
git clone https://github.com/kevins981/linux.git linux-hybridtier
cd linux-hybridtier
git checkout v6.2-hybridtier

# build kernel
make menuconfig # exit without changing anything
make -j16
sudo make modules_install -j
sudo make install -j
```
On our setup, the resulting kernel image size is much larger than the default system kernel. You can check the image size via `ls -l /boot/initrd.img-6.2.0-hybridtier+`. If its > 300MB, the kernel may not load properly. 
To fix this, do the following (source: https://unix.stackexchange.com/questions/270390/how-to-reduce-the-size-of-the-initrd-when-compiling-your-kernel)

```bash
cd /lib/modules/6.2.0-hybridtier+
sudo find . -name *.ko -exec strip --strip-unneeded {} +
sudo update-initramfs -c -k 6.2.0-hybridtier+
```

After these steps, `/boot/initrd.img-6.2.0-hybridtier+` is 57MB on our system. 

To load the HybridTier kernel, edit the grub file. See https://unix.stackexchange.com/a/327686 on how to find the value for `GRUB_DEFAULT`. 

```bash
sudo vi /etc/default/grub
# the GRUB_DEFAULT for our machine looks like this
GRUB_DEFAULT="gnulinux-advanced-(hidden)>gnulinux-6.2.0-hybridtier+-advanced-(hidden)"
```

To modify the fast-tier memory capacity, we use the `memmap` kernel parameter to disable parts of the memory address range. See https://docs.pmem.io/persistent-memory/getting-started-guide/creating-development-environments/linux-environments/linux-memmap#quick-start. On our machine, to limit size of fast-tier memory to 64GB, we use `memmap=440G!4G`, which reserves 440GB of memory starting at 4GB. This is because we have 512GB of memory on NUMA node 0. We also observe that upon boot, roughly 8GB of memory is used by default. Therefore, to leave 64GB of memory for applications, we disable 512-8-64=440GB. 

To find the proper sstarting address, see https://docs.pmem.io/persistent-memory/getting-started-guide/creating-development-environments/linux-environments/linux-memmap#how-to-choose-the-correct-memmap-option-for-your-system.
```bash
sudo vi /etc/default/grub
GRUB_CMDLINE_LINUX="net.ifnames=0 biosdevname=0 memmap=439G!4G" # modify value depending on desired fast-tier memory size

sudo update-grub2
sudo reboot
```

### Approach 2

## Porting HybridTier to other hardware platforms
The provided source code is implemented for two NUMA node system to emulate fast-tier and slow-tier CXL memory. It will not work out of the box on a system with e.g. real CXL memory or persistent memory
