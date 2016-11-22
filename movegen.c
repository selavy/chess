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

// returns 1 if a piece from `side` attacks `square`
int attacks(const struct position * const restrict pos, uint8_t side, int square) {
    uint64_t pcs;
    uint64_t occupied = FULLSIDE(*pos, side) | FULLSIDE(*pos, FLIP(side));
    pcs = pos->brd[PIECE(side,ROOK)] | pos->brd[PIECE(side,QUEEN)];
    if ((rook_attacks(square, occupied) & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PIECE(side,BISHOP)] | pos->brd[PIECE(side,QUEEN)];
    if ((bishop_attacks(square, occupied) & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PIECE(side,KNIGHT)];
    if ((knight_attacks(square) & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PIECE(side,PAWN)];
    if ((pawn_attacks(FLIP(side), square) & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PIECE(side,KING)];
    if ((king_attacks(square) & pcs) != 0) {
        return 1;
    }
    return 0;
}

static move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves) {
    uint64_t posmoves;    
    uint64_t pcs;
    uint32_t from;
    uint32_t to;    
    uint8_t side = pos->wtm;
    uint8_t contraside = FLIP(pos->wtm);
    uint64_t same = pos->side[side];
    uint64_t contra = pos->side[contraside];
    uint64_t occupied = same | contra;
    uint64_t opp_or_empty = ~same;
    uint8_t castle = pos->castle;

    // knight moves
    pcs = PIECES(*pos, side, KNIGHT);
    while (pcs) {
	from = lsb(pcs);
	posmoves = knight_attacks(from) & opp_or_empty;
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
    posmoves = king_attacks(from) & opp_or_empty;
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
    if (side == WHITE) {
        if ((castle & CSL_WKSIDE) != 0 &&
            (from == E1)               &&
            (pos->sqtopc[F1] == EMPTY) &&
            (pos->sqtopc[G1] == EMPTY) &&
            (attacks(pos, contraside, E1) == 0) &&
            (attacks(pos, contraside, F1) == 0) &&
            (attacks(pos, contraside, G1) == 0)) {
            assert(pos->sqtopc[H1] == PIECE(WHITE,ROOK));
            *moves++ = CASTLE(E1, G1);
        }
        if ((castle & CSL_WQSIDE) != 0 &&
            (from == E1)               &&
            (pos->sqtopc[D1] == EMPTY) &&
            (pos->sqtopc[C1] == EMPTY) &&
            (pos->sqtopc[B1] == EMPTY) &&
            (attacks(pos, contraside, E1) == 0) &&
            (attacks(pos, contraside, D1) == 0) &&
            (attacks(pos, contraside, C1) == 0)) {
            assert(pos->sqtopc[A1] == PIECE(WHITE,ROOK));
            *moves++ = CASTLE(E1, C1);
        }
    } else {
        if ((castle & CSL_BKSIDE) != 0 &&
            (from == E8)               &&
            (pos->sqtopc[F8] == EMPTY) &&
            (pos->sqtopc[G8] == EMPTY) &&
            (attacks(pos, contraside, E8) == 0) &&
            (attacks(pos, contraside, F8) == 0) &&
            (attacks(pos, contraside, G8) == 0)) {
            assert(pos->sqtopc[H8] == PIECE(BLACK,ROOK));
            *moves++ = CASTLE(E8, G8);
        }
        if ((castle & CSL_BQSIDE) != 0 &&
            (from == E8)               &&
            (pos->sqtopc[D8] == EMPTY) &&
            (pos->sqtopc[C8] == EMPTY) &&
            (pos->sqtopc[B8] == EMPTY) &&
            (attacks(pos, contraside, E8) == 0) &&
            (attacks(pos, contraside, D8) == 0) &&
            (attacks(pos, contraside, C8) == 0)) {
            assert(pos->sqtopc[A8] == PIECE(BLACK,ROOK));
            *moves++ = CASTLE(E8, C8);
        }
    }
    
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
