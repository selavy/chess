CC=gcc
OBJS=main.o
CFLAGS=-g -O0
TARGET=experiment

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(CFLAGS) $(OBJS)
main.o: main.c
	$(CC) -o $@ $(CFLAGS) -c $<
.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJS)
