// Header information for .osk file format (mesada mboot bootloader)
//
// Created by blackbox reverse engineering only. 100% unofficial, may be wrong, probably incomplete.
//
// Copyright (c) Angus Gratton 2011 Licensed under the new BSD License

#include <stdio.h>
#include <stdint.h>

#define MAGIC 0x62347833

// Each .osk file contains one or more "blocks" of data that are copied to the internal mmc
struct block_descriptor {
  uint32_t start;      // data start offset in .osk file, unit is 512-byte blocks
  uint32_t end;        // data end offset in .osk file, unit is 512-byte blocks
  uint32_t length;     // data length in .osk file, unit is bytes
  char flags[8];       // "flags" appear to be ASCII numbers, don't know why or what yet
  uint32_t zeroes[2];  // ???
};


// Each block has a label (determined by its position in the header)
//
// Blocks which aren't part of an .osk file (ie not to be reflashed) have their descriptors zeroed out
static const char* blocks[] = {
  "mboot", "configfile", "globaldata", "logo", "reserved1", "reserved2", "recovery", "kernel",
  "ramdisk", "system", "app", "sysconfig", "reserved3", "reserved4", "userdata"
};

#define NUM_BLOCKS sizeof(blocks)/sizeof(char *)


// Header at beginning of .osk file
struct header {
  uint32_t magic;       // set to MAGIC
  uint32_t length;      // total length of .osk file, in bytes
  uint32_t zeroes[4];   // ???
  uint32_t update_mask; // bit is set for each block that is to be updated
  uint32_t more_zero;   // ???
  struct block_descriptor desc[NUM_BLOCKS]; // block descriptors

  // After this, file is padded to 0x200 (start=1) and then normal data appears
};


// mboot uses the most naive checksum ever - sum every byte!
static uint32_t mboot_checksum(const uint8_t *data, uint32_t length) {
  uint32_t result = 0;
  for(uint32_t i = 0; i < length; i++) {
    result += data[i];
  }
  return result;
}
