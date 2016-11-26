#include "perft.h"
#include <assert.h>
#include <string.h>
#include "movegen.h"

static uint64_t perft(int depth,
		      struct position *restrict pos,
		      uint64_t *captures,
		      uint64_t *eps,
		      uint64_t *castles,
		      uint64_t *promos,
		      uint64_t *checks,
		      uint64_t *mates) {
    int i;
    int nmoves;
    uint64_t nodes = 0;    
    move moves[MAX_MOVES];
    struct position tmp;
    struct savepos sp;
    uint32_t flags;
    
    if (depth == 0) {
#ifdef COUNT_CHECKS_AND_MATES
	nmoves = generate_legal_moves(pos, &moves[0]);
	if (nmoves == 0) {
	    ++(*mates);
	}
	if (in_check(pos, pos->wtm)) {
	    ++(*checks);
	}
#endif
	return 1;
    }

    assert(in_check(pos, FLIP(pos->wtm)) == 0);
    assert(validate_position(pos) == 0);    
    memcpy(&tmp, pos, sizeof(tmp));
    nmoves = generate_legal_moves(pos, &moves[0]);

    if (depth > 1) {
	for (i = 0; i < nmoves; ++i) {
	    make_move(pos, &sp, moves[i]);
	    nodes += perft(depth - 1, pos, captures, eps, castles, promos, checks, mates);
	    undo_move(pos, &sp, moves[i]);
	    assert(memcmp(pos, &tmp, sizeof(tmp)) == 0);
	}
    } else {
	nodes += nmoves;
	for (i = 0; i < nmoves; ++i) {
	    flags = FLAGS(moves[i]);
	    switch (flags) {
	    case FLG_NONE:
		if (pos->sqtopc[TO(moves[i])] != EMPTY) {
		    ++(*captures);
		}
		break;
	    case FLG_EP: ++(*eps); ++(*castles); break;
	    case FLG_PROMO: ++(*promos); break;
	    case FLG_CASTLE: ++(*castles); break;
	    default: break;
	    }

#if COUNT_CHECKS_AND_MATES
	    make_move(pos, &sp, moves[i]);
	    perft(depth - 1, pos, captures, eps, castles, promos, checks, mates);
	    undo_move(pos, &sp, moves[i]);
	    assert(memcmp(pos, &tmp, sizeof(tmp)) == 0);	    
#endif
	}
    }
    return nodes;
}

int perft_test(const struct position *restrict position,
	       int depth,
	       uint64_t *nodes,
	       uint64_t *captures,
	       uint64_t *eps,
	       uint64_t *castles,
	       uint64_t *promos,
	       uint64_t *checks,
	       uint64_t *mates) {
    *nodes = *captures = *eps = *castles = *promos = *checks = *mates = 0;
    if (depth < 0) {
	return 1;
    }
    struct position pos;
    memcpy(&pos, position, sizeof(pos));
    *nodes = perft(depth, &pos, captures, eps, castles, promos, checks, mates);
 
    return 0;
}

uint64_t perft_speed(struct position *restrict pos, int depth) {
    int i;
    int nmoves;
    uint64_t nodes = 0;    
    move moves[MAX_MOVES];
    struct savepos sp;
    
    if (depth == 0) {
	return 1;
    }

    nmoves = generate_legal_moves(pos, &moves[0]);
    if (depth == 1) {
	nodes = nmoves;
    } else {
	for (i = 0; i < nmoves; ++i) {
	    make_move(pos, &sp, moves[i]);
	    nodes += perft_speed(pos, depth - 1);	    
	    undo_move(pos, &sp, moves[i]);
	}
    }
    
    return nodes;
}
