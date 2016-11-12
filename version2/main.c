#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "move.h"
#include "position.h"
#include "movegen.h"

int main(int argc, char **argv) {
    int ret;
    struct position pos;
    move moves[MAX_MOVES];
    int nmoves;
    int i;
    const char *fen[] = {
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
	"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
	"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
	0
    };
    const char **cur = &fen[0];

    while (*cur) {
	ret = position_from_fen(&pos, *cur);
	if (ret != 0) {
	    fprintf(stderr, "Unable to read fen for position! Error(%d), FEN = %s\n",
		    ret, *cur);
	    exit(EXIT_FAILURE);
	}
	position_print(stdout, &pos);
	printf("\n");

	memset(&moves[0], 0, sizeof(moves[0]) * MAX_MOVES);
	nmoves = generate_legal_moves(&pos, &moves[0]);
	printf("Legal moves:\n");
	for (i = 0; i < nmoves; ++i) {
	    move_print(moves[i]);
	}
	printf("\n");
	
	++cur;
    }
    
    return EXIT_SUCCESS;
}
