CC = gcc
RM=/bin/rm -f
CFLAGS = -ggdb -O2
PROG = radp
OBJS = radp.o detail.o user.o misc.o # readline.o
SRCS = $(OBJS:.o=.c)

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	$(RM) a.out core $(OBJS)
