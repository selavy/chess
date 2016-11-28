#include "movegen.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "magic_tables.h"

#define lsb(bb) __builtin_ctzll(bb)
#define clear_lsb(bb) bb &= (bb - 1)
#define popcountll(bb) __builtin_popcountll(bb)
#define power_of_two(bb) (bb & (bb - 1))
#define more_than_one_piece(bb) power_of_two(bb)

//static int legal_move(const struct position *const restrict pos, move m);
/*static*/int legal_move(const struct position *const restrict pos, move m) {
    // if enpassant

    // else if king move
    //   +check if moving into check
    //   +castling is checked in movegen

    // else
    //   +legal iff it is not pinned or it is moving along the ray towards
    //    or away from the king (i.e. it will still be pinned after the move)
    
    // TODO: return true if playing move `m' would be legal in position `pos'
    // REVISIT(plesslie): better implementation possible that doesn't need a copy
    struct position tmp;
    struct savepos sp;
    memcpy(&tmp, pos, sizeof(tmp));
    make_move(&tmp, &sp, m);
    const int rval = in_check(&tmp, pos->wtm);
    return rval == 0;
}

// TEMP TEMP
int in_check(const struct position *const restrict pos, uint8_t side) {
    const uint64_t kings = pos->brd[PIECE(side,KING)];
    assert(kings != 0);
    const int kingloc = lsb(kings);
    assert(kingloc >= A1 && kingloc <= H8);
    return attacks(pos, FLIP(side), kingloc);
}

// return bitboard of pieces that attack `side's king
uint64_t generate_checkers(const struct position *const restrict pos, uint8_t side) {
    uint64_t ret = 0;
    const int ksq = lsb(PIECES(*pos, side, KING));
    const uint8_t contra = FLIP(side);    
    const uint64_t occupied = pos->side[side] | pos->side[contra];
    const uint64_t knights = PIECES(*pos, contra, KNIGHT);
    const uint64_t bishops = PIECES(*pos, contra, BISHOP);
    const uint64_t rooks = PIECES(*pos, contra, ROOK);
    const uint64_t queens = PIECES(*pos, contra, QUEEN);
    const uint64_t pawns = PIECES(*pos, contra, PAWN);
    const uint64_t king = PIECES(*pos, contra, KING);

    ret |= rook_attacks(ksq, occupied) & (rooks | queens);
    ret |= bishop_attacks(ksq, occupied) & (bishops | queens);
    ret |= knight_attacks(ksq) & knights;
    ret |= king_attacks(ksq) & king;
    ret |= pawn_attacks(side, ksq) & pawns;
    
    return ret;
}

// returns 1 if a piece from `side` attacks `square`
int attacks(const struct position * const restrict pos, uint8_t side, int square) {
    uint64_t pcs;
    const uint8_t contra = FLIP(side);
    //const uint64_t occupied = FULLSIDE(*pos, side) | FULLSIDE(*pos, contra);
    const uint64_t occupied = pos->side[side] | pos->side[contra];
    pcs = pos->brd[PIECE(side, ROOK)] | pos->brd[PIECE(side, QUEEN)];
    if ((rook_attacks(square, occupied) & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PIECE(side, BISHOP)] | pos->brd[PIECE(side, QUEEN)];
    if ((bishop_attacks(square, occupied) & pcs) != 0) {
        return 2;
    }
    pcs = pos->brd[PIECE(side, KNIGHT)];
    if ((knight_attacks(square) & pcs) != 0) {
        return 3;
    }
    pcs = pos->brd[PIECE(side, PAWN)];
    if ((pawn_attacks(contra, square) & pcs) != 0) {
        return 4;
    }
    pcs = pos->brd[PIECE(side, KING)];
    if ((king_attacks(square) & pcs) != 0) {
        return 5;
    }
    return 0;
}

/*static*/ uint64_t pinned_pieces(const struct position *const restrict pos, uint8_t side, uint8_t kingcolor) {
    // REVISIT: make new macros for pseudo attacks that don't need occupied bitboard.  not sure if that will be faster
    //          because LUT will be smaller.
    int sq;
    uint64_t bb;
    uint64_t ret = 0;
    const uint64_t allpieces = pos->side[WHITE] | pos->side[BLACK];    
    const uint64_t kingbb = pos->brd[PIECE(kingcolor, KING)];
    const uint32_t ksq = lsb(kingbb);
    const uint64_t rooks = PIECES(*pos, side, ROOK);
    const uint64_t queens = PIECES(*pos, side, QUEEN);
    const uint64_t bishops = PIECES(*pos, side, BISHOP);
    uint64_t pinners = ((rooks | queens) & rook_attacks(ksq, 0)) | ((bishops | queens) & bishop_attacks(ksq, 0));    
    assert(kingbb);

//#define DEBUG
#ifdef DEBUG
    printf("pinners = %" PRIu64 "\n", pinners);
    printf("allpieces = %" PRIu64 "\n", allpieces);
#endif
    
//#define more_than_one_piece_between(b) power_of_two(b)
#define more_than_one_piece_between(b) more_than_one_piece(b)    
    while (pinners) {
	sq = lsb(pinners);
	bb = between_sqs(sq, ksq) & allpieces;
	#ifdef DEBUG
	printf("between_sqs(%d, %d) = %" PRIu64 "\n", sq, ksq, between_sqs(sq, ksq));
	#endif
	if (!more_than_one_piece_between(bb)) {
	    ret |= (uint64_t)1 << sq;
	    #ifdef DEBUG
	    printf("valid pinner on %d\n", sq);
	    #endif
	}
	clear_lsb(pinners);
    }
    
    return ret;
}

// bitboard with squares attacked by `side' if other side's king is removed.
// removes the other side's king because the king won't be able to move to any square
// that not currently under attack because he is blocking the path.
uint64_t generate_attacked(const struct position *const restrict pos, const uint8_t side) {
    uint64_t ret = 0;
    uint64_t pcs;
    uint32_t from;
    const uint8_t contraside = FLIP(side);
    const uint64_t same = pos->side[side];
    const uint64_t contra = pos->side[contraside];
    const uint64_t ksq = lsb(PIECES(*pos, contraside, KING));
    const uint64_t occupied = (same | contra) & ~MASK(ksq);
    const uint64_t knights = PIECES(*pos, side, KNIGHT);
    const uint64_t bishops = PIECES(*pos, side, BISHOP);
    const uint64_t rooks = PIECES(*pos, side, ROOK);
    const uint64_t queens = PIECES(*pos, side, QUEEN);
    const uint64_t king = PIECES(*pos, side, KING);
    const uint64_t pawns = PIECES(*pos, side, PAWN);

    printf("occupied: %" PRIu64 "\n", occupied);

    pcs = knights;
    while (pcs) {
	from = lsb(pcs);
	ret |= knight_attacks(from);
	clear_lsb(pcs);
    }

    pcs = bishops | queens;
    while (pcs) {
	from = lsb(pcs);
	ret |= bishop_attacks(from, occupied);
	clear_lsb(pcs);
    }

    pcs = rooks | queens;
    while (pcs) {
	from = lsb(pcs);
	ret |= rook_attacks(from, occupied);
	clear_lsb(pcs);
    }

    assert(king);
    assert(popcountll(king) == 1);    
    from = lsb(king);
    ret |= king_attacks(from);

    // capture left
    pcs = pawns & ~A_FILE;
    pcs = side == WHITE ? pcs << 7 : pcs >> 9;
    ret |= pcs;

    // capture right
    pcs = pawns & ~H_FILE;
    pcs = side == WHITE ? pcs << 9 : pcs >> 7;
    ret |= pcs;

    return ret;
}
 
// TODO: generate moves that get out of check
move *generate_evasions(const struct position *const restrict pos, const uint64_t checkers, move *restrict moves) {
    uint32_t to;
    uint32_t from;
    uint64_t pcs;    
    uint64_t posmoves;
    const uint8_t side = pos->wtm;
    const uint8_t contra = FLIP(side); // TODO: does this create a data dependency on `side'?
    const uint64_t attacked = generate_attacked(pos, contra);
    const uint64_t safe = ~(pos->side[side]) & ~attacked;
    // REVISIT(plesslie): check that these loads are scheduled well
    const int checksq = lsb(checkers);
    const int checkpc = pos->sqtopc[checksq];
    const uint64_t kings = PIECES(*pos, side, KING);
    const uint64_t pawns = PIECES(*pos, side, PAWN);
    const uint64_t knights = PIECES(*pos, side, KNIGHT);
    const uint64_t bishops = PIECES(*pos, side, BISHOP);
    const uint64_t rooks = PIECES(*pos, side, ROOK);
    const uint64_t queens = PIECES(*pos, side, QUEEN);
    const uint64_t occupied = pos->side[WHITE] | pos->side[BLACK];
    const int ksq = lsb(kings);

    assert(in_check(pos, pos->wtm) != 0);
    assert(checkers);    

    // 0. General case: either move king, capture piece, or block
    // 1. Knight or pawn check: either move king, or capture knight
    // 2. If more than 1 checker, then must move king
    // 3. en passant could remove the attacker

    // generate king moves to squares that are not under attack
    posmoves = king_attacks(ksq) & safe;
    while (posmoves) {
	to = lsb(posmoves);
	*moves++ = MOVE(ksq, to);
	clear_lsb(posmoves);
    }

    if (!more_than_one_piece(checkers)) {
	uint64_t targets = checkers;

	
	if (checkpc == PIECE(contra, PAWN)) {
	    if (pos->enpassant != EP_NONE) {
		// TODO: check enpassant
	    }
	} else if (checkpc != PIECE(contra, KNIGHT)) {
	    const uint64_t between = between_sqs(checksq, ksq);
	    targets |= between;

	    pcs = pawns;
	    posmoves = side == WHITE ? pcs << 8 : pcs >> 8;
	    posmoves &= between;
	    while (posmoves) {
		to = lsb(posmoves);
		from = side == WHITE ? to - 8 : to + 8;
		assert(pos->sqtopc[from] == PIECE(side, PAWN));
		if (to >= A8 || to <= H1) { // promotion
		    *moves++ = PROMOTION(from, to, KNIGHT);
		    *moves++ = PROMOTION(from, to, BISHOP);
		    *moves++ = PROMOTION(from, to, ROOK);
		    *moves++ = PROMOTION(from, to, QUEEN);
		} else {
		    *moves++ = MOVE(from, to);
		}
		clear_lsb(posmoves);
	    }

	    posmoves = pawns & RANK2(side);
	    posmoves = side == WHITE ? posmoves << 16 : posmoves >> 16;
	    posmoves &= between;
	    while (posmoves) {
		to = lsb(posmoves);
		from = side == WHITE ? to - 16 : to + 16;
		assert(pos->sqtopc[from] == PIECE(side, PAWN));
		// TODO(plesslie): do this with bitmasks?
		// make sure we aren't jumping over another piece
		if (pos->sqtopc[side == WHITE ? to - 8 : to + 8] == EMPTY) {
		    *moves++ = MOVE(from, to);            
		}
		clear_lsb(posmoves);
	    }
	}

	pcs = knights;
	while (pcs) {
	    from = lsb(pcs);
	    posmoves = knight_attacks(from) & targets;
	    while (posmoves) {
		to = lsb(posmoves);
		*moves++ = MOVE(from, to);
		clear_lsb(posmoves);
	    }
	    clear_lsb(pcs);
	}

	pcs = bishops;
	while (pcs) {
	    from = lsb(pcs);
	    posmoves = bishop_attacks(from, occupied) & targets;
	    while (posmoves) {
		to = lsb(posmoves);
		*moves++ = MOVE(from, to);
		clear_lsb(posmoves);
	    }
	    clear_lsb(pcs);
	}

	pcs = rooks;
	while (pcs) {
	    from = lsb(pcs);
	    posmoves = rook_attacks(from, occupied) & targets;
	    while (posmoves) {
		to = lsb(posmoves);
		*moves++ = MOVE(from, to);
		clear_lsb(posmoves);
	    }
	    clear_lsb(pcs);
	}

	pcs = queens;
	while (pcs) {
	    from = lsb(pcs);
	    posmoves = queen_attacks(from, occupied) & targets;
	    while (posmoves) {
		to = lsb(posmoves);
		*moves++ = MOVE(from, to);
		clear_lsb(posmoves);
	    }
	    clear_lsb(pcs);
	}

	// capture left
	posmoves = pawns & ~A_FILE;
	posmoves = side == WHITE ? posmoves << 7 : posmoves >> 9;
	posmoves &= checkers;
	while (posmoves) {
	    to = lsb(posmoves);
	    from = side == WHITE ? to - 7 : to + 9;
	    if (to >= A8 || to <= H1) { // last rank => promotion
		*moves++ = PROMOTION(from, to, KNIGHT);
		*moves++ = PROMOTION(from, to, BISHOP);
		*moves++ = PROMOTION(from, to, ROOK);
		*moves++ = PROMOTION(from, to, QUEEN);
	    } else {
		*moves++ = MOVE(from, to);
	    }
	    clear_lsb(posmoves);
	}

	// capture right
	posmoves = pawns & ~H_FILE;
	posmoves = side == WHITE ? posmoves << 9 : posmoves >> 7;
	posmoves &= checkers;
	while (posmoves) {
	    to = lsb(posmoves);
	    from = side == WHITE ? to - 9 : to + 7;
	    if (to >= A8 || to <= H1) { // last rank => promotion
		*moves++ = PROMOTION(from, to, KNIGHT);
		*moves++ = PROMOTION(from, to, BISHOP);
		*moves++ = PROMOTION(from, to, ROOK);
		*moves++ = PROMOTION(from, to, QUEEN);
	    } else {
		*moves++ = MOVE(from, to);
	    }	    
	    clear_lsb(posmoves);
	}
	
    } // else more than 1 checker, and can't block or capture to get out of check
    
    return moves;
}

static move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves) {
    uint64_t posmoves;    
    uint64_t pcs;
    uint32_t from;
    uint32_t to;    
    const uint8_t side = pos->wtm;
    const uint8_t contraside = FLIP(side); // TODO: does this create a data dependency on `side'?
    const uint64_t same = pos->side[side];
    const uint64_t contra = pos->side[contraside];
    const uint64_t occupied = same | contra;
    const uint64_t opp_or_empty = ~same;
    const uint8_t castle = pos->castle;

#define TOSQ_NOT_KING(sq)				\
    assert(pos->sqtopc[sq] != PIECE(WHITE, KING));	\
    assert(pos->sqtopc[sq] != PIECE(BLACK, KING));

    // knight moves
    pcs = PIECES(*pos, side, KNIGHT);
    while (pcs) {
	from = lsb(pcs);
	posmoves = knight_attacks(from) & opp_or_empty;
	while (posmoves) {
	    to = lsb(posmoves);
	    TOSQ_NOT_KING(to);
	    *moves++ = MOVE(from, to);
	    clear_lsb(posmoves);
	}
	clear_lsb(pcs);
    }

    // bishop moves
    pcs = PIECES(*pos, side, BISHOP);
    while (pcs) {
	from = lsb(pcs);
	posmoves = bishop_attacks(from, occupied) & opp_or_empty;
	while (posmoves) {
	    to = lsb(posmoves);
	    TOSQ_NOT_KING(to);
	    *moves++ = MOVE(from, to);
	    clear_lsb(posmoves);
	}
	clear_lsb(pcs);
    }
    
    // rook moves
    pcs = PIECES(*pos, side, ROOK);
    while (pcs) {
	from = lsb(pcs);
	posmoves = rook_attacks(from, occupied) & opp_or_empty;
	while (posmoves) {
	    to = lsb(posmoves);
	    TOSQ_NOT_KING(to);		
	    *moves++ = MOVE(from, to);
	    clear_lsb(posmoves);
	}
	clear_lsb(pcs);
    }
    
    // queen moves
    pcs = PIECES(*pos, side, QUEEN);
    while (pcs) {
	from = lsb(pcs);
	posmoves = queen_attacks(from, occupied) & opp_or_empty;
	while (posmoves) {
	    to = lsb(posmoves);
	    TOSQ_NOT_KING(to);
	    *moves++ = MOVE(from, to);
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
	TOSQ_NOT_KING(to);
	*moves++ = MOVE(from, to);
	clear_lsb(posmoves);
    }
    
    // castling - `from' still has king position
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

    pcs = PIECES(*pos, side, PAWN);
    // pawn moves - 1 square
    posmoves = side == WHITE ? pcs << 8 : pcs >> 8;
    posmoves &= ~occupied;
    while (posmoves) {
        to = lsb(posmoves);
        from = side == WHITE ? to - 8 : to + 8;
        assert(pos->sqtopc[from] == PIECE(side,PAWN));        
        if (to >= A8 || to <= H1) { // promotion
	    TOSQ_NOT_KING(to);
            *moves++ = PROMOTION(from, to, KNIGHT);
            *moves++ = PROMOTION(from, to, BISHOP);
            *moves++ = PROMOTION(from, to, ROOK);
            *moves++ = PROMOTION(from, to, QUEEN);
        } else {
	    TOSQ_NOT_KING(to);
            *moves++ = MOVE(from, to);
        }        
        clear_lsb(posmoves);
    }
    
    // pawn moves - 2 squares
    posmoves = pcs & RANK2(side);
    posmoves = side == WHITE ? posmoves << 16 : posmoves >> 16;
    posmoves &= ~occupied;
    while (posmoves) {
        to = lsb(posmoves);
        from = side == WHITE ? to - 16 : to + 16;
        assert(pos->sqtopc[from] == PIECE(side,PAWN));
        // TODO(plesslie): do this with bitmasks?
        // make sure we aren't jumping over another piece
        if (pos->sqtopc[side == WHITE ? to - 8 : to + 8] == EMPTY) {
	    TOSQ_NOT_KING(to);
            *moves++ = MOVE(from, to);            
        }        
	clear_lsb(posmoves);
    }
    
    // pawn moves - capture left
    posmoves = pcs & ~A_FILE;
    posmoves = side == WHITE ? posmoves << 7 : posmoves >> 9;
    posmoves &= contra;
    while (posmoves) {
        to = lsb(posmoves);
        from = side == WHITE ? to - 7 : to + 9;
        assert(pos->sqtopc[from] == PIECE(side,PAWN));
        assert(pos->sqtopc[to] != EMPTY);
        if (to >= A8 || to <= H1) { // last rank => promotion
	    TOSQ_NOT_KING(to);
            *moves++ = PROMOTION(from, to, KNIGHT);
            *moves++ = PROMOTION(from, to, BISHOP);
            *moves++ = PROMOTION(from, to, ROOK);
            *moves++ = PROMOTION(from, to, QUEEN);
        } else {
	    TOSQ_NOT_KING(to);
            *moves++ = MOVE(from, to);
        }
	clear_lsb(posmoves);
    }

    // pawn moves - capture right
    posmoves = pcs & ~H_FILE;
    posmoves = side == WHITE ? posmoves << 9 : posmoves >> 7;
    posmoves &= contra;
    while (posmoves) {
        to = lsb(posmoves);
        from = side == WHITE ? to - 9 : to + 7;
        assert(pos->sqtopc[from] == PIECE(side,PAWN));
        assert(pos->sqtopc[to] != EMPTY);
        if (to >= A8 || to <= H1) { // last rank => promotion
	    TOSQ_NOT_KING(to);
            *moves++ = PROMOTION(from, to, KNIGHT);
            *moves++ = PROMOTION(from, to, BISHOP);
            *moves++ = PROMOTION(from, to, ROOK);
            *moves++ = PROMOTION(from, to, QUEEN);
        } else {
	    TOSQ_NOT_KING(to);
            *moves++ = MOVE(from, to);
        }
	clear_lsb(posmoves);
    }

    // TODO: branch on side earlier?
    // en passant
    if (pos->enpassant != EP_NONE) {
	to = pos->enpassant;
	assert((side == WHITE && to >= A6 && to <= H6) ||
	       (side == BLACK && to >= A3 && to <= H3));
	assert(pos->sqtopc[to] == EMPTY);
	// capture left
	if (to != H6 && to != H3) {
	    from = side == WHITE ? to - 7 : to + 9;
	    assert((side == WHITE && from >= A5 && from <= H5) ||
		   (side == BLACK && from >= A4 && from <= H4));
	    if (pos->sqtopc[from] == PIECE(side, PAWN)) {
		TOSQ_NOT_KING(to);
		*moves++ = EP_CAPTURE(from, to);
	    }
	}
	
	// capture right
	if (to != A6 && to != A3) {
	    from = side == WHITE ? to - 9 : to + 7;
	    assert((side == WHITE && from >= A5 && from <= H5) ||
		   (side == BLACK && from >= A4 && from <= H4));
	    if (pos->sqtopc[from] == PIECE(side, PAWN)) {
		TOSQ_NOT_KING(to);
		*moves++ = EP_CAPTURE(from, to);
	    }
	}
    }
    
    return moves;
}

int generate_legal_moves(const struct position *const restrict pos, move *restrict moves) {
    move *restrict cur = moves;
    move *restrict end = generate_non_evasions(pos, moves);

#define FAST_VERSION
#ifdef FAST_VERSION
    // TODO(plesslie): if in check, then only generate evasions, then this branch will go away		
    const int incheck = in_check(pos, pos->wtm);
    if (incheck) {
	while (cur != end) {
	    if (!legal_move(pos, *cur)) {
		*cur = *(--end);
	    } else {
		++cur;
	    }
	}
    } else {
	const uint8_t side = pos->wtm;
	const uint8_t contra = FLIP(side);
	const uint64_t kingbb = pos->brd[PIECE(side, KING)];
	const int ksq = lsb(kingbb);
	const uint64_t pinpcs = pinned_pieces(pos, contra, side);
    
	while (cur != end) {
	    // need to check legality of move if:
	    //   +it is the king moving
	    //   +there are pinned pieces
	    //   +move is enpassant
	    if ((FROM(*cur) == ksq || pinpcs || FLAGS(*cur) == FLG_EP) && !legal_move(pos, *cur)) {
		*cur = *(--end);
	    } else {
		++cur;
	    }
	}
    }
#else
    while (cur != end) {
	if (!legal_move(pos, *cur)) {
	    *cur = *(--end);
	} else {
	    ++cur;
	}
    }
#endif

    return (int)(end - moves);
}
