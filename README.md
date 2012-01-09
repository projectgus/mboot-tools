What Is This
============

Some utility programs allowing programmers to work with the firmware
file format for the "Mesada mboot" bootloader. Tested with Linux.

These programs should allow you to unpack, modify & then repack
firmware update files for this proprietary ARM bootloader.

As used in the "Flexiview FV-1" device, S5PV210 based, manufactured by
Mesada, and sold under various brand names (including generic
'AndroidTV' labels and as the Kogan Agora TV in UK & Australia.)


Important Disclaimer
====================

These programs were created by blackbox reverse engineering only.

100% unofficial, may be wrong, probably incomplete.

Don't blame me if you build an update and brick your device. I make
absolutely no guarantees that this will work at all, in any way.

However, I haven't managed to brick anything yet so that's a good sign at least. :)

Example Use
===========

## Unpacking a firmare .osk file

    ./mboot_extract my_firmware_file.osk

(creates numerous .bin files which you can edit/replace as appropriate)

## Recreating a firmware .osk file

    ./mboot_pack my_awesome_firmware.osk kernel.bin system.bin ramdisk.bin app.bin

(picks up some of the .bin files and makes them back into a firmware image.)

## Creating an SD card image

    ./mboot_pack --for-sd /dev/sdc kernel.bin system.bin ramdisk.bin app.bin

(will write the .osk, plus an SD "update" header, directly to SD card at /dev/sdc. Same as if you used the Windows-based "SDTool" to make one. Anything already on the SD card will be erased.)


## Reflashing a new kernel

Once you have your kernel configured to build:

    sb2 make zImage # <-- replace with whatever make args you need for ARM kernels, I'm using [sb2](http://www.plugcomputer.org/plugwiki/index.php/Scratchbox2_based_cross_compiling)
    ln -s arch/arm/boot/zImage kernel.bin
    mboot_pack --for-sd /dev/sdc kernel.bin

(The kernel.bin symlink is so mboot_pack knows this file is the kernel. The last line will create an SD card image that just flashes a new kernel and leaves everything else alone, except for the data partition which for some reason is always formatted.)


About .osk files
================

.OSK files are an aggregate of images that will flash the device. Can contain any/all of these:

* Filesystem images (ie system.bin is an image of /system and app.bin is an image of /system/app)
* Kernel image (kernel.bin is a zImage)
* initrd image (ramdisk.bin is an initrd with a uboot header affixed!)
* mboot image (mboot.bin is the mboot bootloader binary. Reflash at own risk!)

Each part of the .osk is called a "block" in mboot-tools. The .osk file
has a header which describes which "blocks" are present in the .osk
file, and what the .osk file layout is.

When an update is applied to the system, the .osk file is read and the
blocks in it are copied to the internal flash. The layout of the
internal flash is *not described* in the .osk header.

The above types of blocks are all the components I've seen, although
lots of others appear to be defined in mboot. It would be risky to create .osk
files containing these other types, though.

Important: when using mboot_pack, it is not recommended that you make
.osk files including mboot.bin. Reflashing bootloaders is dangerous
and silly, and just because the "stock" updates do it doesn't mean you
should!



Status
======

Basic unpacking & repacking of FV-1 firmwares works.

It is possible to make a firmware update containing only one partition
from the original firmware image (for instance, to avoid the dangerous
re-flashing of mboot which currently occurs on *every single update*
for the firmware files I've seen.)

The following things remain unknown:

* How per-block flags & version numbers work
* How to change the internal partition table of the device, from the point of view of the bootloader.
* How to enable any interactive booting mode in the bootloader (currently it prompts on serial with timeout=0).
* How to boot from external SD slot instead of internal eMMC chip.
* How to modify 'configfile' or any of the other "block types" marked with "(GUESS)" in mboot.c.
* How the formatting of "partition 4" (aka '/data') or "partition 1" (aka vold /mnt/flash) is controlled. At the moment any update via the SD card causes /mnt/flash to be reformatted fresh.


LICENSE
=======

Copyright 2011 Angus Gratton. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY ANGUS GRATTON ''AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ANGUS GRATTON OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Angus Gratton.

