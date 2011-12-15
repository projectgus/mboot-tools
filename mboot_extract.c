#include "mboot.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// Extract a .osk file (mesada mboot bootloader) and create various .bin files
//
// Created by blackbox reverse engineering only. 100% unofficial, may be wrong, probably incomplete.
//
// Copyright (c) Angus Gratton 2011 Licensed under the new BSD License

#define handle_error(msg)                                       \
  do { perror(msg); return 2; } while (0)

int extract_osk(const char *filepath, int force) {
  int fd = open(filepath, O_RDONLY);
  if(fd<0)
    handle_error("open failed");

  struct stat sb;
  if (fstat(fd, &sb) == -1)
    handle_error("fstat failed");


  struct header *map = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if(!map) {
    handle_error("mmap failed");
  }

  if(map->magic != MAGIC) {
    fprintf(stderr, "Magic number 0x%x doesn't match expected 0x%x. Not an .osk file?\n", map->magic, MAGIC);
    if(!force) return 1;
  }
  if(map->length != sb.st_size) {
    fprintf(stderr, "Stored size 0x%x doesn't match expected size 0x%zx. Corrupt .osk file?\n",
            map->length, sb.st_size);
    if(!force) return 1;
  }

  for(int i = 0; i < NUM_BLOCKS; i++) {
    struct block_descriptor *desc = &map->desc[i];

    int valid_block = desc->start || desc->end || desc->length;
    int masked_in = map->update_mask & 1<<i;
    if(!masked_in && !valid_block) {
      printf("Skipping disused block %d: %s\n", i, blocks[i]);
      continue;
    }
    if((masked_in != 0) != (valid_block != 0)) {
      if(masked_in)
        printf("According to the update mask, there should be an update for block %d. But none is here.\n", i);
      else
        printf("According to the update mask, there is no update for block %d. However some offsets are defined here anyhow.\n", i);
      if(!force) return 1;
      continue;
    }


    printf("Extracting block %d to %s.bin ...\n", i, blocks[i]);

    uint8_t *start = &((uint8_t *)map)[desc->start*512];
    uint32_t stored_checksum = *((uint32_t *)&start[desc->length]);
    uint32_t calc_checksum = mboot_checksum(start, desc->length);
    if(stored_checksum != calc_checksum) {
      fprintf(stderr, "Calculated checksum 0x%x doesn't match stored checksum 0x%x. Invalid data?\n",
              calc_checksum, stored_checksum);
      if(!force) return 1;
    }

    char filename[100];
    snprintf(filename, 100, "%s.bin", blocks[i]);
    int out = open(filename, O_WRONLY|O_CREAT|(force ? O_TRUNC : 0), 00644);
    if(out < 0)
      handle_error("open output file");
    write(out, start, desc->length);
    close(out);
  }

  printf("Done.\n");
  return 0;
}



int main(int argc, char **argv) {

  if( (argc==3 && !strcmp(argv[1], "--force")) || argc > 3 || argc == 1) {
    fprintf(stderr, "Usage: %s [--force] <osk file>\n", argv[0]);
    return 1;
  }

  return extract_osk(argv[argc-1], argc==3);
}
