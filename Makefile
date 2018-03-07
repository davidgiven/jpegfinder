CC = cc
CFLAGS = -g -Os

jpegfinder: jpegfinder.c
	$(CC) $(CFLAGS) -o $@ $<
