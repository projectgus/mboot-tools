#include "mboot.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

// Pack a supplied list of files together into an .osk

// Assumption is that files will be equivalent to the ones product by mboot_extract,
// in particular that file names will start with the names of .osk "blocks" ie mboot.bin, kernel.bin, etc.

#define handle_error(msg)                                       \
  do { perror(msg); return 2; } while (0)

static int get_block_index(const char *filename);

// Flags on each block are poorly understood at this point...
static const char *flags[] = {
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
};


#define SD_HEADER_SIZE 512*1024

// files are arranged in block order, with NULL for any block not to be added
int pack_osk(const char *oskfile, const char **files, int for_sd) {
  int fd = open(oskfile, O_RDWR|O_CREAT, 00644);
  if(fd<0)
    handle_error("open output failed");

  size_t old_size = sizeof(struct header);
  struct header *map = mmap(NULL, old_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if(map == MAP_FAILED) {
    handle_error("mmap failed");
  }

  memset(map, 0, sizeof(struct header));
  map->magic = MAGIC;
  map->magic2 = MAGIC2;

  int start_block = 1;

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

    int end_block = start_block + (sb.st_size / 512) + 1;
    map = mremap(map, old_size, end_block*512, MREMAP_MAYMOVE);
    old_size = end_block*512;
    if(input < 0) {
      handle_error("failed to remap file to new length");
    }

    struct block_descriptor *desc = &map->desc[i];
    uint8_t *bytes = (uint8_t *)map;
    uint32_t start_offs = start_block * 512;

    memset(&bytes[start_offs], 0xFF, (end_block-start_block)*512);
    ssize_t r = read(input, &bytes[start_offs], sb.st_size);
    if(r < sb.st_size) {
      handle_error("failed to read from input file");
    }
    close(input);

    desc->length = sb.st_size;
    desc->start = start_block;
    desc->end = end_block;
    memcpy(desc->flags, flags[i], sizeof(desc->flags));

    *(uint32_t *)&bytes[start_offs + sb.st_size] = mboot_checksum(&bytes[start_offs], sb.st_size);
    start_block = end_block;
  }

  map->length = old_size;

  // If we need to prepend a 512k "for SD card" header, we do it here
  // this is probably a silly way to do it, given we're file backed and all, but what the hey
  printf("Prepending SD card header...\n");
  map = mremap(map, old_size, old_size+SD_HEADER_SIZE, MREMAP_MAYMOVE);
  memmove(&((uint8_t*)map)[SD_HEADER_SIZE], map, old_size);
  memset(map, 0, SD_HEADER_SIZE);
  strcpy((char *)map, "update");

  old_size += SD_HEADER_SIZE;
  munmap(map, old_size);
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
    fprintf(stderr, "Usage: %s [--for-sd] <osk output file> <input files>...\n", argv[0]);
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
