CC = gcc
CFLAGS = -g -lm

SRC = rate_test.c rate.c cms/count_min_sketch.c
OUT = rate_test

all:
	$(CC) $(SRC) $(CFLAGS) -o $(OUT)

clean:
	rm -f $(OUT)
