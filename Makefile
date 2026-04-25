CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c99
LDFLAGS = -pthread

TARGETS = pipeline warehouse scheduler
SRCS = src/helpers.c
OBJS = $(SRCS:.c=.o)
HEADER_DIR = include

.PHONY: all clean

all: $(TARGETS)

pipeline: src/problem1.c $(OBJS)
	$(CC) $(CFLAGS) -I$(HEADER_DIR) $^ -o $@

warehouse: src/problem2.c $(OBJS)
	$(CC) $(CFLAGS) -I$(HEADER_DIR) $^ -o $@

scheduler: src/problem3.c $(OBJS)
	$(CC) $(CFLAGS) -I$(HEADER_DIR) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -I$(HEADER_DIR) -c $< -o $@

clean:
	rm -f $(TARGETS) $(OBJS)
