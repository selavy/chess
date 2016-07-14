#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "types.h"
#include "fen_reader.h"

struct move {
    uint8_t r1;
    uint8_t c1;
    uint8_t r2;
    uint8_t c2;
    int piece;
    int side;

    // TODO: capture
    //int is_capture;
    //int captured_piece_type;
    //int captured_row;
    //int captured_col;
};

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

int get_move(
        struct board * restrict board,
        struct move * restrict move,
        FILE * restrict input,
        FILE * restrict output) {
    return 0;
}

int valid_move(
        const struct board * restrict board,
        const struct move * restrict move,
        int wtm
        ) {
    return 0;
}

int place_move(
        struct board * restrict board,
        const struct move * restrict move
        ) {
    // TODO: captures
    /* uint64_t *pos = move->side == WHITE */
    /*     ? &board->white[move->piece] */
    /*     : &board->black[move->piece]; */
    /* CLEAR(*pos, SQUARE(move->c1, move->r1)); */
    /* PLACE(*pos, SQUARE(move->c2, move->r2)); */
    return 0;
}

int main(int argc, char **argv) {
    #if 0
    struct move move;
    #endif
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

    int wtm = WHITE;
    #if 0
    if (get_move(&board, &move, stdin, stdout) != 0) {
        fputs("Unable to get move!", stderr);
        exit(EXIT_FAILURE);
    }
    if (valid_move(&board, &move, wtm) != 0) {
        fputs("Invalid move!", stderr);
        exit(EXIT_FAILURE);
    }
    if (place_move(&board, &move) != 0) {
        assert(0); // should always pass if valid_move() was true
        fputs("Unable to place piece!", stderr);
        exit(EXIT_FAILURE);
    }
    #endif
    wtm ^= TRUE;
    print_board(stdout, &board);

    const char * fen = "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2";
    struct fen_position pos;
    memset(&pos, 0, sizeof(pos));
    if (fen_reader(&pos, fen) != 0) {
        fputs("FEN reader failed", stderr);
    } else {
        printf("\n---------------------------------\n|");
        for (int row = 7; row >= 0; --row) {
            for (int col = 0; col < 8; ++col) {
                printf(" %c |", pos.board[SQUARE(col, row)]);
            }
            printf("\n---------------------------------\n|");
        }
    }
    
    return 0;
}
