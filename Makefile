TARGET = netem

OBJS = main.o probe.o

CC = gcc
CFLAGS = -g -lrt -std=c99 -Wall

all: $(TARGET) Makefile
	
$(TARGET): $(OBJS)
	$(CC) -o $@ $^
	
clean:
	rm -f $(TARGET)
	rm -f *.o *~

.PHONY: all clean
