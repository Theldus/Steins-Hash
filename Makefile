# A Steins;Gate timestamp finder tool
# This is free and unencumbered software released into the public domain.

CC     ?= cc
CC_JS   = emcc
CFLAGS += -Wall -Wextra -O3
CFLAGS_JS += $(CFLAGS)
CFLAGS_JS += -s EXPORTED_FUNCTIONS='["_find_episode", "_malloc", "_free"]'
CFLAGS_IDX += $(CFLAGS) -march=native

all: index find_ep.js Makefile

index: index.o
	$(CC) $(CFLAGS_IDX) index.o -o index
index.o: index.c
	$(CC) $(CFLAGS_IDX) index.c -c

find_ep.js: find_ep.o
	$(CC_JS) $(CFLAGS_JS) find_ep.o -o find_ep.js
find_ep.o: hashes find_ep.c
	$(CC_JS) $(CFLAGS_JS) find_ep.c -c

dedup: hashes
	python dedup.py hashes new_hashes
	mv hashes original_hashes
	mv new_hashes hashes

clean:
	rm -f find_ep.js find_ep.wasm index *.o
