#!/usr/bin/env python
#
# Script utilities for manipulating Mesada Technology mboot firmware files (.osk)
#
# Not official, nor endorsed, nor based on any documentation. Probably won't work.
#
# VERY HIGH CHANCE OF BRICKING YOUR DEVICE. NOT MY FAULT IF YOU DO.

from ctypes import *
import mmap, os, sys, binascii, zlib, math

MAGIC = (0x33, 0x78, 0x34, 0x62)
UNKNOWN = (0x81, 0x07, 0x00, 0x00)

class OskPrefix(Structure):
    """
    The prefix is the very first thing in a (non-SDCardified) .osk file.
    """
    _fields_ = [("magic", c_ubyte*4),       # ascii 3x4b, magic number?
                ("total_length", c_int32), # length of entire .osk file, in bytes
                ("_zeroes", c_byte*16),     # mystery
                ("unknown", c_ubyte*4),     # 0x81 0x07 0x00 0x00 <-- maybe a device id?
                ("_zeroes2", c_byte*4),
                ]

    def __init__(self, **kw):
        defaults = { "magic" :  MAGIC,
                    "unknown" : UNKNOWN,
                    }
        defaults.update(kw)
        print defaults
        Structure.__init__(self, **defaults)


# These are the sequence of possible "blocks" described in the file. unused blocks (ie not known to the device)
# are left in but kept empty (all zeroes.)
#
# As yet I do not know how to flash blocks which are not known to the system - you can probably make a lot of
# mess that way!
osk_block_types = [ "mboot", "configfile", "globaldata", "logo", "reserved1", "reserved2", "recovery", "kernel",
                    "ramdisk", "system", "app", "sysconfig", "reserved3", "reserved4", "userdata" ]

def get_block_flags(block_type):
    # NO IDEA what these flags all mean as yet

    if not block_type in osk_block_types:
        raise Error("Unknown block type %s" % block_type)
    suffix_flags = {
        "app" : "8181",  # app is the last partition before EXT3, so trailing "1" could mean "format next"?
        "kernel" : "7070",
        "mboot" : "160" + chr(0),
        }
    flags = "0" if block_type == "app" else "1"
    flags += "10"
    flags += "0" if block_type == "mboot" else "1"
    flags += suffix_flags.get(block_type, "8180")
    print flags,"for",block_type
    return flags

class OskBlockDescriptor(Structure):
    """
    The prefix is followed by block descriptors for each block type. Unused blocks are still included but
    are left blank
    """
    _fields_ = [("start", c_int32),    # data start offset (in osk file not on mmc, in 512-byte block units)
                ("end",   c_int32),    # next block data offset (in osk file not on mmc, in 512-byte block units)
                ("length", c_int32),   # length of actual data in bytes - must be less than (end-start)*512
                ("flags", c_char*8),   # ASCII(?) flags, not sure what they do
                ("zero",  c_int32*2),
                ]

    def __init__(self, block_type, **kw):
        if not "flags" in kw:
            kw["flags"] = get_block_flags(block_type)
        Structure.__init__(self, kw)


class OskHeader(Structure):
    """
    The full header structure looks like this. Padded to 512 which is where the first data block (start=1)
    starts from
    """
    _anonymous_ = [ "prefix" ]
    _fields_ = [("prefix", OskPrefix),
                ("descriptors", OskBlockDescriptor * len(osk_block_types)),
                ("padding", c_byte * (512-sizeof(OskPrefix)-sizeof(OskBlockDescriptor)*len(osk_block_types))),
                ]


def mboot_crc(data):
    """
    Currently wrong, needs to become mboot's crc algo
    """
    return binascii.crc32(data) & 0xffffffff

def extract_osk(osk_file, force=False):
    def try_error(msg):
        print "%s%s" % (msg, " (continuing anyway)" if force else "")
        if not force:
            sys.exit(1) # bit harsh, but still!

    with open(osk_file, "r") as f:
        m = mmap.mmap(f.fileno(), 0, mmap.MAP_SHARED, mmap.PROT_READ)

        header = OskHeader.from_buffer_copy(m, 0)

        if tuple(header.magic) != MAGIC:
            try_error("OSK magic number doesn't match. Got %s" % str(tuple(header.unknown)))
        if tuple(header.unknown) != UNKNOWN:
            try_error("OSK \"unknown magic number\" doesn't match. Got %s" % str(tuple(header.unknown)))
        if header.total_length != m.size():
            try_error("File length (%d) doesn't match length in header (%d)" % (m.size(), header.total_length))

        idx=-1
        for d in header.descriptors:
            idx += 1
            filename = "%s.bin" % osk_block_types[idx]
            if d.start == 0 or d.end == 0 or d.length == 0:
                print "No block defined for %s" % filename
                continue
            print "Extracting %s" % filename
            if os.path.exists(filename):
                try_error("%s already exists" % filename)
            with open(filename, "w") as w:
                start = d.start*512
                data = m[start:start+d.length]
                checksum = mboot_crc(data)
                file_checksum = c_uint32.from_buffer_copy(m, start+d.length).value
                if checksum != file_checksum:
                    try_error("Stored OSK checksum (0x%08x) does not match calculated checksum (0x%08x)" % (file_checksum, checksum))
                w.write(data)

        m.close()

    print "Done"


def find_osk_index(source_file):
    source_file = os.path.basename(source_file)
    for t in osk_block_types:
        if source_file.startswith(t):
            return osk_block_types.index(t)
    raise ValueError("Filename %s does not match any of the known mboot block types: %s" % (source_file, osk_block_types))


def pack_osk(output_file, sources):

    sources = sorted(sources, key=lambda x: find_osk_index(x)) # sort by key order
    try:
        print sources
        fd = os.open(output_file, os.O_CREAT | os.O_TRUNC | os.O_RDWR)
        os.write(fd, "xyz") # file needs some data in it before we can mmap() it at all
        m = mmap.mmap(fd, 0)

        m.resize(sizeof(OskHeader))
        header = OskHeader.from_buffer(m)
        header.magic = MAGIC
        header.unknown = UNKNOWN

        next_block = 1
        for source in sources:
          idx = find_osk_index(source)
          print source,idx
          with open(source, "r") as i:
              data = i.read()
          block_len = len(data)/512 + 1
          end_block = next_block + block_len
          end_offs = end_block*512
          start_offs = next_block*512
          print "Writing %s at offset 0x%08x (block 0x%04x-0x%04x)" % (source, start_offs, next_block, end_block)
          header.descriptors[idx].start = next_block
          header.descriptors[idx].end = end_block
          header.descriptors[idx].length = len(data)
          header.descriptors[idx].flags = get_block_flags(osk_block_types[idx])
          m.resize(end_block*512)
          header = OskHeader.from_buffer(m) # resize can move the mmap pointer, so move this as well

          m[start_offs:start_offs+len(data)] = data

          checksum = c_uint32.from_buffer(m, start_offs+len(data))
          checksum.value = mboot_crc(data)


          for offs in range(start_offs+len(data)+4, end_offs):
              m[offs] = chr(0xFF)
          next_block = end_block

        header.total_length = next_block * 512
        print "Done writing, total file length was %d bytes" % header.total_length
    finally:
        if m:
            m.close()
        os.close(fd)

