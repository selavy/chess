CC=gcc
DEBUG=-O2 -g -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -fbounds-check
RELEASE=-O3 -fstrict-aliasing -ffast-math -DNDEBUG -flto -msse -march=native -fomit-frame-pointer
CFLAGS=$(DEBUG) -Wall -Werror -pedantic -std=c11
OBJS=main.o
TARGET=chess

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS)
main.o: main.c
	$(CC) -o $@ $(CFLAGS) -c $<
%.o: %.c %.h
	$(CC) -o $@ $(CFLAGS) -c $<
.PHONY: clean
clean:
	rm -rf $(OBJS) $(TARGET)

