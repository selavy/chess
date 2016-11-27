CC=gcc
DEVELOPMENT_FLAGS=-Wno-unused-function
DEBUG=-O0 -g -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -fbounds-check
RELEASE=-O3 -fstrict-aliasing -ffast-math -DNDEBUG -flto -msse -march=native -fomit-frame-pointer -fstrict-aliasing
MODE=$(RELEASE)
CFLAGS=$(MODE) -Wall -Werror -pedantic -std=c11 $(DEVELOPMENT_FLAGS)
OBJS=main.o magic_tables.o move.o position.o movegen.o perft.o
MT_GENERATOR=generate_magic_tables
TARGET=chess

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS)
$(MT_GENERATOR): $(MT_GENERATOR).c
	$(CC) -o $@ -std=c11 -Wall -Werror -pedantic -O3 $<
	./$(MT_GENERATOR)
magic_tables.o: $(MT_GENERATOR)
	$(CC) -o $@ $(CFLAGS) -c magic_tables.c
main.o: main.c
	$(CC) -o $@ $(CFLAGS) -c $<
%.o: %.c %.h
	$(CC) -o $@ $(CFLAGS) -c $<
.PHONY: clean
clean:
	rm -rf $(OBJS) $(TARGET) $(MT_GENERATOR) *~

