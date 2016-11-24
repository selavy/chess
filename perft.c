#include "perft.h"
#include <assert.h>
#include <string.h>
#include "movegen.h"

static uint64_t perft(int depth, struct position *restrict pos) {
    int i;
    int nmoves;
    uint64_t nodes = 0;    
    move moves[MAX_MOVES];
    struct position tmp;
    struct savepos sp;
    
    if (depth == 0) {
	return 1;
    }

    assert(validate_position(pos) == 0);    
    memcpy(&tmp, pos, sizeof(tmp));
    nmoves = generate_legal_moves(pos, &moves[0]);
    for (i = 0; i < nmoves; ++i) {
	make_move(pos, &sp, moves[i]);
	nodes += perft(depth - 1, pos);
	memcpy(pos, &tmp, sizeof(tmp));
	assert(validate_position(pos) == 0);
    }
    return nodes;
}

uint64_t perft_test(const struct position *restrict position, int depth) {
    if (depth < 0) {
	return 0;
    }
    struct position pos;
    memcpy(&pos, position, sizeof(pos));
    return perft(depth, &pos);
}

