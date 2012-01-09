// Pack a supplied list of files together into an .osk format
// for the mesada mboot bootloader

// Assumption is that files will be equivalent to the ones product by mboot_extract,
// in particular that file names will start with the names of .osk "block" that
// they are to be copied to. ie mboot.bin, kernel.bin, etc. 
// (a full list of block names can be found in mboot.h.)
//
//
// Created by blackbox reverse engineering only. 100% unofficial, may be wrong, probably incomplete.
//
// ABSOLUTELY NO WARRANTY. FIRMWARE UPDATES CREATED BY THIS FILE MAY NOT WORK AND
// ARE QUITE LIKELY TO BRICK YOUR DEVICE. USE WITH ABSOLUTELY EXTREME CAUTION.
//
// Copyright (c) Angus Gratton 2011 Licensed under the new BSD License


#include "mboot.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>


#define handle_error(msg)                                       \
  do { perror(msg); return 2; } while (0)

static int get_block_index(const char *filename);

// I initially thought some of these values were flags, but now I think they're just
// version numbers.
//
// The version numbers appear to be ignored by mboot, here they are just hardcoded to
// likely sounding values.
static const char *versions[] = {
  "1100160\0", // 0 = mboot
  "11018180", // 1 = configfile
  "11018180", // 2 = globaldata (GUESS)
  "11018180", // 3 = logo (GUESS)
  "11018180", // 4 = reserved1 (GUESS)
  "11018180", // 5 = reserved2 (GUESS)
  "11018180", // 6 = recovery (GUESS)
  "11017070", // 7 = kernel
  "11018180", // 8 = ramdisk (initrd)
  "11018180", // 9 = system (ext2)
  "01018181", // 10 = app (ext2)
  "11018180", // 11 = sysconfig (GUESS)
  "11018080", // 12 = reserved3 (GUESS)
  "11018180", // 13 = reserved4 (GUESS)
  "11018180", // 14 = userdata  (GUESS)
  "11018180", // 15 = fat  (GUESS)
};


#define SD_MAGIC "update"
#define SD_HEADER_SIZE 512*1024

// files are arranged in block order, with NULL for any block not to be added
int pack_osk(const char *oskfile, const char **files, int for_sd) {
  int fd = open(oskfile, O_RDWR|O_CREAT|O_TRUNC, 00644);
  if(fd<0)
    handle_error("open output failed");

  if(for_sd) { // write SD card pseudo-header if needed
    write(fd, SD_MAGIC, sizeof(SD_MAGIC));
    uint8_t zero = 0;
    for(int p = sizeof(SD_MAGIC); p < SD_HEADER_SIZE; p++) {
      write(fd, &zero, 1);
    }
  }

  struct header header;
  memset(&header, 0, sizeof(struct header));
  header.magic = MAGIC;
  for(int i = 0; i < NUM_BLOCKS; i++) {
    if(files[i])
      header.update_mask |= 1<<i;
  }

  write(fd, &header, sizeof(header)); // Initial header mostly invalid, will rewrite after

  int start_block = 1;
  lseek(fd, 512 + (for_sd ? SD_HEADER_SIZE : 0), SEEK_SET);

  for(int i = 0; i < NUM_BLOCKS; i++) {
    if(!files[i])
      continue; // Block not in use

    printf("Loading %s...\n", files[i]);
    int input = open(files[i], O_RDONLY);
    if(input < 0) {
      handle_error("failed to open input file");
    }

    struct stat sb;
    if (fstat(input, &sb) == -1)
      handle_error("fstat on input failed");

    // Copy file into .osk output
    int end_block = start_block + (sb.st_size / 512) + 1;
    void *buf = malloc(sb.st_size);
    if(!buf) {
      handle_error("failed to allocate memory to read input file");
    }
    ssize_t r = read(input, buf, sb.st_size);
    if(r < sb.st_size) {
      handle_error("failed to read from input file");
    }
    write(fd, buf, sb.st_size);
    close(input);

    // Write checksum to .osk
    uint32_t checksum = mboot_checksum(buf, sb.st_size);
    free(buf);
    write(fd, &checksum, sizeof(checksum));

    // pad to block boundary
    uint8_t pad = 0xFF;
    for(int p = start_block*512 + sb.st_size + sizeof(checksum); p < end_block*512; p++)
      write(fd, &pad, 1);

    // update header
    struct block_descriptor *desc = &header.desc[i];
    desc->length = sb.st_size;
    desc->start = start_block;
    desc->end = end_block;
    memcpy(desc->version, versions[i], sizeof(desc->version));

    start_block = end_block;
  }

  header.length = start_block*512;

  // write out the full, updated, header again
  lseek(fd, for_sd ? SD_HEADER_SIZE : 0, SEEK_SET);
  write(fd, &header, sizeof(header));

  close(fd);
  printf("Success.\n");
  return 0;
}


// Lookup which block a filename matches
static int get_block_index(const char *filename)
{
  char *base = basename((char *)filename);
  for (int i = 0; i < NUM_BLOCKS; i++) {
    if(!strncmp(blocks[i], base, strlen(blocks[i])))
      return i;
  }
  return -1;
}


int main(int argc, const char **argv) {
  if(argc < 3) {
    fprintf(stderr, "Usage: %s [--for-sd] <osk output file> <input files>...\n\n", argv[0]);
    fprintf(stderr, "FIRMWARE UPDATES CREATED BY THIS PROGRAM MAY NOT WORK AND\n");
    fprintf(stderr, "ARE QUITE LIKELY TO BRICK YOUR DEVICE. USE ONLY WITH ABSOLUTELY EXTREME CAUTION.\n");
    return 1;
  }

  int for_sd = !strcmp(argv[1], "--for-sd");

  // Rearrange the input files into "block order"
  const char *files[NUM_BLOCKS] = { 0 };
  for(int i = for_sd ? 3 : 2; i < argc; i++) {
    int block_idx = get_block_index(argv[i]);
    if(block_idx < 0) {
      fprintf(stderr, "Failed to identify block type of file %s.\n", argv[i]);
      return 1;
    }
    files[block_idx] = argv[i];
  }

  return pack_osk(argv[for_sd ? 2 : 1], files, for_sd);
}
