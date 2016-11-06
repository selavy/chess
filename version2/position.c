#include "position.h"
#include <string.h>

const char *visual_pcs = "PNBRQKpnbrqk ";

int read_fen(struct position *restrict pos, const char *fen) {
    /* int row; */
    /* int col; */
    /* int color; */
    /* char c; */

    pos->nmoves = 1;
    pos->wtm = WHITE;
    pos->halfmoves = 0;
    pos->castle = CSL_NONE;
    pos->enpassant = EP_NONE;
    memset(&pos->sqtopc[0], EMPTY, sizeof(pos->sqtopc[0]) * 64);
    memset(&pos->brd[0], 0, sizeof(pos->brd[0]) * NPIECES * 2);

    return 0;
}

void position_print(FILE *os, struct position *restrict pos) {
    fprintf(os, "+---+---+---+---+---+---+---+---+\n");
    for (int rank = RANK_8; rank >= RANK_1; --rank) {
	for (int file = FILE_A; file <= FILE_H; ++file) {
	    fprintf(os, "| %c ", visual_pcs[pos->sqtopc[SQUARE(file, rank)]]);
	}
	fprintf(os, "|\n+---+---+---+---+---+---+---+---+\n");
    }
}
