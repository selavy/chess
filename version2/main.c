#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "position.h"

int main(int argc, char **argv) {
    int ret;
    struct position pos;
    const char *starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


    /* pos.sqtopc[A1] = PIECE(WHITE, ROOK); */
    /* pos.sqtopc[B1] = PIECE(WHITE, KNIGHT); */
    /* pos.sqtopc[C1] = PIECE(WHITE, BISHOP); */

    ret = position_from_fen(&pos, starting_position);
    if (ret != 0) {
	fprintf(stderr, "Unable to read fen for position: %d!\n", ret);
	exit(EXIT_FAILURE);
    }

    position_print(stdout, &pos);
    
    return 0;
}
