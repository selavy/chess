#include "movegen.h"
#include <assert.h>

// TODO: maybe should be caching this in the position

static int legal_move(const struct position *const restrict pos, move m);
static move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves);

static int legal_move(const struct position *const restrict pos, move m) {
    // TODO: return true if playing move `m' would be legal in position `pos'
    return 1;
}

static move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves) {
    /* uint8_t side = pos->wtm; */
    /* uint8_t contraside = FLIP(pos->wtm); */
    /* uint64_t same =  */
    // king moves
    // knight moves
    // bishop moves
    // rook moves
    // queen moves
    // castling
    // pawn moves - 1 square
    // pawn moves - 2 squares
    // pawn moves - captures
    // en passant
    
    return moves;
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
