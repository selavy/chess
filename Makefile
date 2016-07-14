CC=gcc
DEBUG=-O0 -g
RELEASE=-O2 -fstrict-aliasing -ffast-math -lto
CFLAGS=$(DEBUG) -Wall -Werror -pedantic -std=c99
OBJS=main.o types.o fen_reader.o
TARGET=chess

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS)
%.o: %.c %.h
	$(CC) -o $@ $(CFLAGS) -c $<
main.o: main.c
	$(CC) -o $@ $(CFLAGS) -c $<
.PHONY: clean
clean:
	rm -rf $(OBJS) $(TARGET)
