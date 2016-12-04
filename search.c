#include "search.h"
#include <stdlib.h>
#include <string.h>
#include "move.h"
#include "position.h"
#include "movegen.h"
#include "eval.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define DEBUGF(...) do { fprintf(stderr, __VA_ARGS__); } while(0)

int alphabeta(struct position *restrict pos, int depth, int alpha, int beta, int maximizing) {
    int best;
    int nmoves;
    int i;
    int value;
    move moves[MAX_MOVES];
    struct savepos sp;
    
    if (depth == 0) {
	return eval(pos);
    }
    nmoves = generate_legal_moves(pos, &moves[0]);
    if (nmoves == 0) { // checkmate
	return pos->wtm ? WHITE_WIN : BLACK_WIN;
    }

    if (maximizing) {
	best = NEG_INFINITI;
	for (i = 0; i < nmoves; ++i) {
	    make_move(pos, &sp, moves[i]);
	    value = alphabeta(pos, depth - 1, alpha, beta, 0);
	    undo_move(pos, &sp, moves[i]);
	    best = MAX(best, value);
	    alpha = MAX(alpha, best);
	    if (beta <= alpha) {
		break; // beta cutoff
	    }
	}
    } else {
	best = INFINITI;
	for (i = 0; i < nmoves; ++i) {
	    make_move(pos, &sp, moves[i]);
	    value = alphabeta(pos, depth - 1, alpha, beta, 1);
	    undo_move(pos, &sp, moves[i]);
	    best = MIN(best, value);
	    beta = MIN(beta, best);
	    if (beta <= alpha) {
		break; // alpha cutoff
	    }
	}
    }

    return best;
}

/*extern*/ move search(const struct position *restrict const position) {
    struct savepos sp;
    struct position pos;
    move moves[MAX_MOVES];
    int nmoves;
    int i;
    int best = position->wtm == WHITE ? NEG_INFINITI : INFINITI;
    int value;
    move rval;

    memcpy(&pos, position, sizeof(pos));
    nmoves = generate_legal_moves(&pos, &moves[0]);
    DEBUGF("Generated %d legal moves\n", nmoves);

    if (pos.wtm == WHITE) {
	for (i = 0; i < nmoves; ++i) {
	    make_move(&pos, &sp, moves[i]);
	    value = alphabeta(&pos, 3, NEG_INFINITI, INFINITI, 1);
	    DEBUGF("value of %s -> %d\n", xboard_move_print(moves[i]), value);
	    if (value > best) {
		rval = moves[i];
		best = value;
	    }
	    undo_move(&pos, &sp, moves[i]);
	}
    } else {
	for (i = 0; i < nmoves; ++i) {
	    make_move(&pos, &sp, moves[i]);
	    value = alphabeta(&pos, 3, NEG_INFINITI, INFINITI, 0);
	    DEBUGF("value of %s -> %d\n", xboard_move_print(moves[i]), value);
	    if (value < best) {
		rval = moves[i];
		best = value;
	    }
	    undo_move(&pos, &sp, moves[i]);
	}
    }

    return rval;
}
