#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define ROWS 8
#define COLS 8
#define MASK(x) ((uint64_t)1 << (x))
#define SQUARE(col, row) ((row)*ROWS + (col))
#define PLACE(brd, sqr) (brd) |= MASK(sqr)

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

int board_init(struct board *brd) {
    memset(&brd->white[0], 0, sizeof(brd->white[0]) * NUM_PIECES * 2);
    return 0;
}

void print_board(FILE* fp, const struct board * restrict brd) {
    int square;
    uint64_t mask;
    char p;
    fprintf(fp, "---------------------------------\n");
    for (int i = ROWS - 1; i >= 0; --i) {
        fprintf(fp, "|");
        for (int j = 0; j < COLS; ++j) {
            square = i * ROWS + j;        
            mask = MASK(square);
            if (brd->white[PAWN] & mask) {
                p = 'p';
            } else if (brd->white[KNIGHT] & mask) {
                p = 'n';
            } else if (brd->white[BISHOP] & mask) {
                p = 'b';
            } else if (brd->white[ROOK] & mask) {
                p = 'r';
            } else if (brd->white[QUEEN] & mask) {
                p = 'q';
            } else if (brd->white[KING] & mask) {
                p = 'k';
            } else if (brd->black[PAWN] & mask) {
                p = 'P';
            } else if (brd->black[KNIGHT] & mask) {
                p = 'N';
            } else if (brd->black[BISHOP] & mask) {
                p = 'B';
            } else if (brd->black[ROOK] & mask) {
                p = 'R';
            } else if (brd->black[QUEEN] & mask) {
                p = 'Q';
            } else if (brd->black[KING] & mask) {
                p = 'K';
            } else {
                p = ' ';
            }
            fprintf(fp, " %c |", p);
        }
        fprintf(fp, "\n---------------------------------\n");
    }
}


int main(int argc, char **argv) {
    struct board board;
    board_init(&board);

    PLACE(board.white[ROOK],   SQUARE(0, 0));
    PLACE(board.white[KNIGHT], SQUARE(1, 0));
    PLACE(board.white[BISHOP], SQUARE(2, 0));
    PLACE(board.white[QUEEN],  SQUARE(3, 0));
    PLACE(board.white[KING],   SQUARE(4, 0));
    PLACE(board.white[BISHOP], SQUARE(5, 0));
    PLACE(board.white[KNIGHT], SQUARE(6, 0));
    PLACE(board.white[ROOK],   SQUARE(7, 0));

    PLACE(board.white[PAWN], SQUARE(0, 1));
    PLACE(board.white[PAWN], SQUARE(1, 1));
    PLACE(board.white[PAWN], SQUARE(2, 1));
    PLACE(board.white[PAWN], SQUARE(3, 1));
    PLACE(board.white[PAWN], SQUARE(4, 1));
    PLACE(board.white[PAWN], SQUARE(5, 1));
    PLACE(board.white[PAWN], SQUARE(6, 1));
    PLACE(board.white[PAWN], SQUARE(7, 1));

    PLACE(board.black[ROOK],   SQUARE(0, 7));
    PLACE(board.black[KNIGHT], SQUARE(1, 7));
    PLACE(board.black[BISHOP], SQUARE(2, 7));
    PLACE(board.black[QUEEN],  SQUARE(3, 7));
    PLACE(board.black[KING],   SQUARE(4, 7));
    PLACE(board.black[BISHOP], SQUARE(5, 7));
    PLACE(board.black[KNIGHT], SQUARE(6, 7));
    PLACE(board.black[ROOK],   SQUARE(7, 7));

    PLACE(board.black[PAWN], SQUARE(0, 6));
    PLACE(board.black[PAWN], SQUARE(1, 6));
    PLACE(board.black[PAWN], SQUARE(2, 6));
    PLACE(board.black[PAWN], SQUARE(3, 6));
    PLACE(board.black[PAWN], SQUARE(4, 6));
    PLACE(board.black[PAWN], SQUARE(5, 6));
    PLACE(board.black[PAWN], SQUARE(6, 6));
    PLACE(board.black[PAWN], SQUARE(7, 6));

    print_board(stdout, &board);
    return 0;
}
