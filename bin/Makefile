EXEC = run

OBJDIR = ../obj

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

CC = gcc
CFLAGS = -g -Wall

$(EXEC): $(OBJS)
	$(CC) -o $@ $^

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
