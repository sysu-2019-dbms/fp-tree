
# Prerequisite

## Simulating NVM on Linux

*(Note: tested on Ubuntu 18.04 or above)*

### Check your DRAM status by executing 

```bash
dmesg | grep BIOS-e820
```

You should see something like the following output (output may vary):

```
[    0.000000] BIOS-e820: [mem 0x0000000000000000-0x000000000009e7ff] usable
[    0.000000] BIOS-e820: [mem 0x000000000009e800-0x000000000009ffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000000dc000-0x00000000000fffff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000000100000-0x00000000bfecffff] usable
[    0.000000] BIOS-e820: [mem 0x00000000bfed0000-0x00000000bfefefff] ACPI data
[    0.000000] BIOS-e820: [mem 0x00000000bfeff000-0x00000000bfefffff] ACPI NVS
[    0.000000] BIOS-e820: [mem 0x00000000bff00000-0x00000000bfffffff] usable
[    0.000000] BIOS-e820: [mem 0x00000000f0000000-0x00000000f7ffffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000fec00000-0x00000000fec0ffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000fee00000-0x00000000fee00fff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000fffe0000-0x00000000ffffffff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000100000000-0x000000023fffffff] usable
```

The system has 8G RAM and 4G~8G space is available for simulation. 

### Make some DRAM persistent

We make this DRAM space persistent by editing the booting parameters of the system. Open `/etc/default/grub` as root user in any editor you like:

```bash
sudo vim /etc/default/grub
```

Find `GRUB_CMDLINE_LINUX_DEFAULT` and add `memmap=xxG!yyG` at the end, which makes the DRAM from xxG to xx+yyG persistent. For example, the following makes the 4G~8G of the DRAM persistent:

```
GRUB_CMDLINE_LINUX_DEFAULT="quiet memmap=4G!4G"
```

Save the file and make it take effect by:

```bash
sudo update-grub
sudo reboot
```

### Check NVM simulation

After rebooting, check if you have successfully simulated the NVM by:

```bash
dmesg | grep user
```

You should see something like this (output may vary):

```
[    0.000000] user-defined physical RAM map:
[    0.000000] user: [mem 0x0000000000000000-0x000000000009e7ff] usable
[    0.000000] user: [mem 0x000000000009e800-0x000000000009ffff] reserved
[    0.000000] user: [mem 0x00000000000dc000-0x00000000000fffff] reserved
[    0.000000] user: [mem 0x0000000000100000-0x00000000bfecffff] usable
[    0.000000] user: [mem 0x00000000bfed0000-0x00000000bfefefff] ACPI data
[    0.000000] user: [mem 0x00000000bfeff000-0x00000000bfefffff] ACPI NVS
[    0.000000] user: [mem 0x00000000bff00000-0x00000000bfffffff] usable
[    0.000000] user: [mem 0x00000000f0000000-0x00000000f7ffffff] reserved
[    0.000000] user: [mem 0x00000000fec00000-0x00000000fec0ffff] reserved
[    0.000000] user: [mem 0x00000000fee00000-0x00000000fee00fff] reserved
[    0.000000] user: [mem 0x00000000fffe0000-0x00000000ffffffff] reserved
[    0.000000] user: [mem 0x0000000100000000-0x00000001ffffffff] persistent (type 12)
[    0.000000] user: [mem 0x0000000200000000-0x000000023fffffff] usable

```

Note that the 4G~8G space is persistent with type 12, which means it is simulated as NVM.

The system has also recognized the `pmem0` persistent device. We can check this by executing:

```bash
sudo fdisk -l /dev/pmem0
```

You should see something like this (output may vary):

```
Disk /dev/pmem0: 4 GiB, 4294967296 bytes, 8388608 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 4096 bytes
I/O size (minimum/optimal): 4096 bytes / 4096 bytes
```

### Mounting the device

Firstly we format the NVM device to EXT4 FS.

```bash
sudo mkfs.ext4 /dev/pmem0
```

Then we create a folder for mounting:

```bash
sudo mkdir /pmem-fs
sudo chown `whoami` /pmem-fs
```

Finally we mount the device:

```bash
sudo mount -o dax /dev/pmem0 /pmem-fs
```

### Install libpmem library

```bash
sudo apt install libpmem1 libpmem-dev
```

### Run the example program to check your configuration

```bash
make pmem_test
```

After execution, a string `hello, persistent memory` is written into the NVM.

You should see the following output: 

```
hello, persistent memory
```

By now, you have successfully configured the simulation environment of NVM on Linux.
