#include "movegen.h"
#include <assert.h>

static int legal_move(const struct position *const restrict pos, move m);
static move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves);

static int legal_move(const struct position *const restrict pos, move m) {
    return 0;
}

static move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves) {
    return (moves + 1);
}

int generate_legal_moves(const struct position *const restrict pos, move *restrict moves) {
    move *restrict cur = moves;
    move *restrict end = generate_non_evasions(pos, moves);

    while (cur != end) {
	if (!legal_move(pos, *cur)) {
	    *cur = *(--end);
	} else {
	    ++cur;
	}
    }

    return (int)(end - moves);
}
