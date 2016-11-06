#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "position.h"

int main(int argc, char **argv) {
    struct position pos;

    if (read_fen(&pos, "") != 0) {
	fprintf(stderr, "Unable to read fen for position!\n");
	exit(EXIT_FAILURE);
    }

    pos.sqtopc[A1] = PIECE(WHITE, ROOK);
    pos.sqtopc[B1] = PIECE(WHITE, KNIGHT);
    pos.sqtopc[C1] = PIECE(WHITE, BISHOP);

    position_print(stdout, &pos);
    
    return 0;
}
