CC=gcc
DEBUG=-O0 -g -fno-omit-frame-pointer -fsanitize=address
RELEASE=-O3 -fstrict-aliasing -ffast-math -flto
CFLAGS=$(RELEASE) -Wall -Werror -pedantic -std=c99 
GENERATED=magic_tables.o
OBJS=$(GENERATED) main.o
TARGET=chess
GEN=generate_magic_tables

$(TARGET): $(OBJS)
	./$(GEN)
	$(CC) -o $@ $(CFLAGS) $(OBJS)
magic_tables.o: $(GEN)
	./$(GEN)
	$(CC) -o $@ $(CFLAGS) -c magic_tables.c
$(GEN): $(GEN).c
	$(CC) -o $@ $<
main.o: main.c
	$(CC) -o $@ $(CFLAGS) -c $<
.PHONY: clean
clean:
	rm -rf $(OBJS) $(TARGET) $(GEN) magic_tables.* *~
