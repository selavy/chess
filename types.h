#ifndef TYPES__H_
#define TYPES__H_

#include <stdlib.h>
#include <inttypes.h>

#define ROWS 8
#define COLS 8
#define MASK(x) ((uint64_t)1 << (x))
#define SQUARE(col, row) ((row)*ROWS + (col))
#define PLACE(brd, sqr) (brd) |= MASK(sqr)
#define CLEAR(brd, sqr) (brd) &= ~MASK(sqr)
#define TRUE 1
#define FALSE 0

enum {
    PAWN = 0,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    NUM_PIECES
};

enum {
    WHITE = 0,
    BLACK = 1
};

struct board {
    uint64_t white[NUM_PIECES];
    uint64_t black[NUM_PIECES];
} __attribute__((packed));

int board_init(struct board * restrict brd);

#endif // TYPES__H_
