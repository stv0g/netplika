TARGETS = netem mark.so

OBJS = main.o probe.o emulate.o timing.o hist.o utils.o ts.o tc.o tcp.o dist.o dist-maketable.o

CC      = gcc

CFLAGS  = -g -lrt -std=c99 -Wall

CFLAGS += -I/usr/local/include/libnl3
CFLAGS += -I/usr/include/libnl3

LDLIBS = -lnl-3 -lnl-route-3 -lm

all: $(TARGETS) Makefile
	
netem: $(OBJS)
	$(CC) $(LDLIBS) -o $@ $^

mark.so: mark.c
	$(CC) -g -fPIC -c -o mark.o mark.c
	$(CC) -shared -o mark.so mark.o -ldl
	
clean:
	rm -f $(TARGETS)
	rm -f *.o *~

.PHONY: all clean libnl
