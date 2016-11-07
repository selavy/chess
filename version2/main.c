#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "position.h"

int main(int argc, char **argv) {
    int ret;
    struct position pos;
    /* const char *starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; */
    /* ret = position_from_fen(&pos, starting_position); */
    /* if (ret != 0) { */
    /* 	fprintf(stderr, "Unable to read fen for position: %d!\n", ret); */
    /* 	exit(EXIT_FAILURE); */
    /* } */
    /* position_print(stdout, &pos); */

    const char *fen[] = {
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
	"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
	"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"
    };
    const size_t nfen = sizeof(fen) / sizeof(*fen);

    for (int i = 0; i < nfen; ++i) {
	ret = position_from_fen(&pos, fen[i]);
	if (ret != 0) {
	    fprintf(stderr, "Unable to read fen for position! Error(%d), FEN = %s\n",
		    ret, fen[i]);
	    exit(EXIT_FAILURE);
	}
	position_print(stdout, &pos);
	printf("\n");
    }
    
    return EXIT_SUCCESS;
}
