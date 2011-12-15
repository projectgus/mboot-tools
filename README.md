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


Example Use
===========

./mboot_unpack my_firmware_file.osk

(creates numerous .bin files which you can edit/replace as appropriate)

sudo ./mboot_pack --for-sd /dev/sdc *.bin

(will write the .osk, plus an SD "update" header, directly to SD card)


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
* How the formatting of "partition 4" (aka '/data') or "partition 1" (aka vold /mnt/flash) is controlled.

About .osk files
================

.OSK files are an aggregate of images that will flash the device. Can contain any/all of these:

* Filesystem images (ie /system or /systema/app)
* Kernel image (zImage)
* initrd image (with uboot header for some reason!)
* mboot image (raw binary)
* ... etc

In these programs, each of these images is a "block". The .osk file
has a header which describes which "blocks" are present in the .osk
file, and what the .osk file layout is.

When an update is applied to the system, the .osk file is read and the
blocks in it are copied to the internal flash. The layout of the
internal flash is *not described* in the .osk file.



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

