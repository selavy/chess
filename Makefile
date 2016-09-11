CC=gcc
DEBUG=-O2 -g -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -fbounds-check
GEN_PROFILE=-fprofile-generate
USE_PROFILE=-fprofile-use
RELEASE=-O3 -fstrict-aliasing -ffast-math -DNDEBUG -flto -msse -march=native -fomit-frame-pointer
CFLAGS=$(RELEASE) -Wall -Werror -pedantic -std=c11
GENERATED=magic_tables.o
OBJS=$(GENERATED) types.o move.o movegen.o read_fen.o perft.o main.o
TARGET=chess
GEN=generate_magic_tables

$(TARGET): $(OBJS)
	./$(GEN)
	$(CC) -o $@ $(CFLAGS) $(OBJS)
magic_tables.o: $(GEN)
	./$(GEN)
	$(CC) -o $@ $(CFLAGS) -c magic_tables.c
$(GEN): $(GEN).c
	$(CC) -o $@ $(CFLAGS) $<
main.o: main.c
	$(CC) -o $@ $(CFLAGS) -c $<
%.o: %.c %.h
	$(CC) -o $@ $(CFLAGS) -c $<
pgn_parser: pgn_parser.c
	$(CC) -o $@ $(CFLAGS) $<
.PHONY: clean
clean:
	rm -rf $(OBJS) $(TARGET) $(GEN) magic_tables.* *~
