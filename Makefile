CFLAGS=-std=gnu99 -Wall -Werror -D_GNU_SOURCE -g

all: mboot_extract mboot_pack

mboot_extract: mboot_extract.c mboot.h
	gcc $(CFLAGS) -o mboot_extract mboot_extract.c

mboot_pack: mboot_pack.c mboot.h
	gcc $(CFLAGS) -o mboot_pack mboot_pack.c

