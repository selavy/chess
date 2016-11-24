#include "perft.h"
#include <assert.h>
#include <string.h>
#include "movegen.h"

static uint64_t perft(int depth,
		      struct position *restrict pos,
		      uint64_t *captures,
		      uint64_t *eps,
		      uint64_t *castles,
		      uint64_t *promos) {
    int i;
    int nmoves;
    uint64_t nodes = 0;    
    move moves[MAX_MOVES];
    struct position tmp;
    struct savepos sp;
    uint32_t flags;
    
    if (depth == 0) {
	return 1;
    }

    assert(validate_position(pos) == 0);    
    memcpy(&tmp, pos, sizeof(tmp));
    nmoves = generate_legal_moves(pos, &moves[0]);

    if (depth > 1) {
	for (i = 0; i < nmoves; ++i) {
	    make_move(pos, &sp, moves[i]);
	    nodes += perft(depth - 1, pos, captures, eps, castles, promos);
	    memcpy(pos, &tmp, sizeof(tmp));
	    assert(validate_position(pos) == 0);
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
	    case FLG_EP: ++(*eps); break;
	    case FLG_PROMO: ++(*promos); break;
	    case FLG_CASTLE: ++(*castles); break;
	    default: break;
	    }
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
	       uint64_t *promos) {
    if (depth < 0) {
	return 0;
    }
    struct position pos;
    memcpy(&pos, position, sizeof(pos));
    *nodes = perft(depth, &pos, captures, eps, castles, promos);

    return 0;
}

