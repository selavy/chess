#include "movegen.h"
#include <assert.h>
#include "magic_tables.h"

// TODO: maybe should be caching this in the position

#define lsb(bb) __builtin_ctzll(bb)
#define clear_lsb(bb) bb &= (bb - 1)
#define popcountll(bb) __builtin_popcountll(bb)

static int legal_move(const struct position *const restrict pos, move m);
static move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves);

static int legal_move(const struct position *const restrict pos, move m) {
    // TODO: return true if playing move `m' would be legal in position `pos'
    return 1;
}

static move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves) {
    uint8_t side = pos->wtm;
    uint8_t contraside = FLIP(pos->wtm);
    uint64_t same = pos->side[side];
    uint64_t contra = pos->side[contraside];
    uint64_t occupied = same | contra;
    uint64_t opp_or_empty = ~same;
    //uint8_t castle = pos->castle;
    uint64_t posmoves;    
    uint64_t pcs;
    uint32_t from;
    uint32_t to;

    // knight moves
    pcs = PIECES(*pos, side, KNIGHT);
    while (pcs) {
	from = lsb(pcs);
	posmoves = knight_attacks[from] & opp_or_empty;
	while (posmoves) {
	    to = lsb(posmoves);
	    *moves++ = SIMPLEMOVE(from, to);
	    clear_lsb(posmoves);
	}
	clear_lsb(pcs);
    }

    // king moves
    pcs = PIECES(*pos, side, KING);
    assert(pcs);
    assert(popcountll(pcs) == 1);
    from = lsb(pcs);
    posmoves = king_attacks[from] & opp_or_empty;
    while (posmoves) {
	to = lsb(posmoves);
	*moves++ = SIMPLEMOVE(from, to);
	clear_lsb(posmoves);
    }

    // bishop moves
    pcs = PIECES(*pos, side, BISHOP);
    while (pcs) {
	from = lsb(pcs);
	posmoves = bishop_attacks(from, occupied);
	while (posmoves) {
	    to = lsb(posmoves);
	    if ((MASK(to) & same) == 0) {
		*moves++ = SIMPLEMOVE(from, to);
	    }
	    clear_lsb(posmoves);
	}
	clear_lsb(pcs);
    }
    
    // rook moves
    pcs = PIECES(*pos, side, ROOK);
    while (pcs) {
	from = lsb(pcs);
	posmoves = rook_attacks(from, occupied);
	while (posmoves) {
	    to = lsb(posmoves);
	    if ((MASK(to) & same) == 0) {
		*moves++ = SIMPLEMOVE(from, to);
	    }
	    clear_lsb(posmoves);
	}
	clear_lsb(pcs);
    }
    
    // queen moves
    pcs = PIECES(*pos, side, QUEEN);
    while (pcs) {
	from = lsb(pcs);
	posmoves = queen_attacks(from, occupied);
	while (posmoves) {
	    to = lsb(posmoves);
	    if ((MASK(to) & same) == 0) {
		*moves++ = SIMPLEMOVE(from, to);
	    }
	    clear_lsb(posmoves);
	}
	clear_lsb(pcs);
    }
    
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
