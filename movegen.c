#include "movegen.h"
#include "magic_tables.h"

// returns 1 if a piece from `side` attacks `square`
int attacks(const struct position * const restrict pos, uint8_t side, int square) {
    uint64_t pcs;
    uint64_t occupied = FULLSIDE(*pos, side) | FULLSIDE(*pos, FLIP(side));
    pcs = pos->brd[PC(side,ROOK)] | pos->brd[PC(side,QUEEN)];
    if ((rook_attacks(square, occupied) & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PC(side,BISHOP)] | pos->brd[PC(side,QUEEN)];
    if ((bishop_attacks(square, occupied) & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PC(side,KNIGHT)];
    if ((knight_attacks[square] & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PC(side,PAWN)];
    if ((pawn_attacks(FLIP(side), square) & pcs) != 0) {
        return 1;
    }
    pcs = pos->brd[PC(side,KING)];
    if ((king_attacks[square] & pcs) != 0) {
        return 1;
    }
    return 0;
}

int in_check(const struct position * const restrict pos, uint8_t side) {
    // find `side`'s king
    uint64_t kings = pos->brd[PC(side,KING)];
    int kingloc = 0;
    assert(kings != 0); // there should be a king...
    for (; (kings & ((uint64_t)1 << kingloc)) == 0; ++kingloc);
    // check if the other side attacks the king location
    return attacks(pos, FLIP(side), kingloc);
}

uint32_t gen_legal_moves(const struct position *const restrict pos, move *restrict moves) {
    struct savepos sp;
    const uint8_t side = pos->wtm;
    move *curm, *endm;
    int legal;
    struct position p;
    memcpy(&p, pos, sizeof(p));	
    const uint32_t nmoves = generate_moves(pos, moves);
    curm = moves;
    endm = moves + nmoves;
    while (curm < endm) {
	make_move(&p, *curm, &sp);
	legal = in_check(&p, side) == 0;
	undo_move(&p, *curm, &sp);
	if (!legal) {
	    --endm;
	    *curm = *endm;
	} else {
	    ++curm;
	}
    }

    return curm - moves;
}

uint32_t generate_moves(const struct position *const restrict pos, move *restrict moves) {
    uint32_t from;
    uint32_t to;
    uint64_t pcs;
    uint64_t posmoves;
    uint32_t nmove = 0;    
    uint8_t side = pos->wtm;
    uint8_t contraside = FLIP(pos->wtm);
    uint64_t same = FULLSIDE(*pos, side);
    uint64_t contra = FULLSIDE(*pos, contraside);
    uint64_t occupied = same | contra;
    uint64_t opp_or_empty = ~same;
    uint8_t castle = pos->castle;
    
    // knight moves
    pcs = PIECES(*pos, side, KNIGHT);
    while (pcs) {
        from = __builtin_ctzll(pcs);
        posmoves = knight_attacks[from] & opp_or_empty;
        while (posmoves) {
            to = __builtin_ctzll(posmoves);
            if ((MASK(to) & same) == 0) {
                moves[nmove++] = SIMPLEMOVE(from, to);
            }
            posmoves &= (posmoves - 1);            
        }
        pcs &= (pcs - 1);
    }

    // king moves
    pcs = PIECES(*pos, side, KING);
    assert(pcs != 0);
    while (pcs) {
        from = __builtin_ctzll(pcs);
        assert(from < 64 && from >= 0);
        assert(pos->sqtopc[from] == PC(side,KING));
        posmoves = king_attacks[from] & opp_or_empty;
        while (posmoves) {
            to = __builtin_ctzll(posmoves);
            if ((MASK(to) & same) == 0) {
                moves[nmove++] = SIMPLEMOVE(from, to);
            }
            posmoves &= (posmoves - 1);
        }
        pcs &= (pcs - 1);
    }

    // castling
    if (side == WHITE) {
        if ((castle & WKINGSD) != 0    &&
            (from == E1)               &&
            (pos->sqtopc[F1] == EMPTY) &&
            (pos->sqtopc[G1] == EMPTY) &&
            (attacks(pos, contraside, E1) == 0) &&
            (attacks(pos, contraside, F1) == 0) &&
            (attacks(pos, contraside, G1) == 0)) {
            assert(pos->sqtopc[H1] == PC(WHITE,ROOK));
            moves[nmove++] = MOVE(E1, G1, MV_FALSE, MV_FALSE, MV_TRUE);
        }
        if ((castle & WQUEENSD) != 0   &&
            (from == E1)               &&
            (pos->sqtopc[D1] == EMPTY) &&
            (pos->sqtopc[C1] == EMPTY) &&
            (pos->sqtopc[B1] == EMPTY) &&
            (attacks(pos, contraside, E1) == 0) &&
            (attacks(pos, contraside, D1) == 0) &&
            (attacks(pos, contraside, C1) == 0)) {
            assert(pos->sqtopc[A1] == PC(WHITE,ROOK));
            moves[nmove++] = MOVE(E1, C1, MV_FALSE, MV_FALSE, MV_TRUE);
        }
    } else {
        if ((castle & BKINGSD) != 0    &&
            (from == E8)               &&
            (pos->sqtopc[F8] == EMPTY) &&
            (pos->sqtopc[G8] == EMPTY) &&
            (attacks(pos, contraside, E8) == 0) &&
            (attacks(pos, contraside, F8) == 0) &&
            (attacks(pos, contraside, G8) == 0)) {
            assert(pos->sqtopc[H8] == PC(BLACK,ROOK));
            moves[nmove++] = MOVE(E8, G8, MV_FALSE, MV_FALSE, MV_TRUE);
        }
        if ((castle & BQUEENSD) != 0   &&
            (from == E8)               &&
            (pos->sqtopc[D8] == EMPTY) &&
            (pos->sqtopc[C8] == EMPTY) &&
            (pos->sqtopc[B8] == EMPTY) &&
            (attacks(pos, contraside, E8) == 0) &&
            (attacks(pos, contraside, D8) == 0) &&
            (attacks(pos, contraside, C8) == 0)) {
            assert(pos->sqtopc[A8] == PC(BLACK,ROOK));
            moves[nmove++] = MOVE(E8, C8, MV_FALSE, MV_FALSE, MV_TRUE);
        }
    }

    // bishop moves
    pcs = PIECES(*pos, side, BISHOP);
    while (pcs) {
        from = __builtin_ctzll(pcs);
        posmoves = bishop_attacks(from, occupied);
        while (posmoves) {
            to = __builtin_ctzll(posmoves);
            if ((MASK(to) & same) == 0) {
                moves[nmove++] = SIMPLEMOVE(from, to);
            }
            posmoves &= (posmoves - 1);
        }
        pcs &= (pcs - 1);
    }

    // rook moves
    pcs = PIECES(*pos, side, ROOK);
    while (pcs) {
        from = __builtin_ctzll(pcs);
        posmoves = rook_attacks(from, occupied);
        while (posmoves) {
            to = __builtin_ctzll(posmoves);
            if ((MASK(to) & same) == 0) {
                moves[nmove++] = SIMPLEMOVE(from, to);
            }
            posmoves &= (posmoves - 1);
        }
        pcs &= (pcs - 1);
    }

    // queen moves
    pcs = PIECES(*pos, side, QUEEN);
    while (pcs) {
        from = __builtin_ctzll(pcs);
        posmoves = queen_attacks(from, occupied);
        while (posmoves) {
            to = __builtin_ctzll(posmoves);
            if ((MASK(to) & same) == 0) {
                moves[nmove++] = SIMPLEMOVE(from, to);
            }
            posmoves &= (posmoves - 1);
        }
        pcs &= (pcs - 1);
    }

    // pawn moves
    pcs = PIECES(*pos, side, PAWN);

    // forward 1 square
    posmoves = side == WHITE ? pcs << 8 : pcs >> 8;
    posmoves &= ~occupied;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = side == WHITE ? to - 8 : to + 8;
        assert(pos->sqtopc[from] == PC(side,PAWN));        
        if (to >= A8 || to <= H1) { // promotion
            moves[nmove++] = MOVE(from, to, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_BISHOP, MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_ROOK  , MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_QUEEN , MV_FALSE, MV_FALSE);
        } else {
            moves[nmove++] = SIMPLEMOVE(from, to);
        }        
        posmoves &= (posmoves - 1);
    }

    // forward 2 squares
    posmoves = pcs & RANK2(side);
    posmoves = side == WHITE ? posmoves << 16 : posmoves >> 16;
    posmoves &= ~occupied;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = side == WHITE ? to - 16 : to + 16;
        assert(pos->sqtopc[from] == PC(side,PAWN));
        // TODO(plesslie): do this with bitmasks?
        // make sure we aren't jumping over another piece
        if (pos->sqtopc[side == WHITE ? to - 8 : to + 8] == EMPTY) {
            moves[nmove++] = SIMPLEMOVE(from, to);            
        }        
        posmoves &= (posmoves - 1);
    }

    // capture left
    posmoves = pcs & ~A_FILE;
    posmoves = side == WHITE ? posmoves << 7 : posmoves >> 9;
    posmoves &= contra;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = side == WHITE ? to - 7 : to + 9;
        assert(pos->sqtopc[from] == PC(side,PAWN));
        assert(pos->sqtopc[to] != EMPTY);
        if (to >= A8 || to <= H1) { // last rank => promotion
            moves[nmove++] = MOVE(from, to, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_BISHOP, MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_ROOK  , MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_QUEEN , MV_FALSE, MV_FALSE);
        } else {
            moves[nmove++] = SIMPLEMOVE(from, to);
        }
        posmoves &= (posmoves - 1);
    }

    // capture right
    posmoves = pcs & ~H_FILE;
    posmoves = side == WHITE ? posmoves << 9 : posmoves >> 7;
    posmoves &= contra;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = side == WHITE ? to - 9 : to + 7;
        assert(pos->sqtopc[from] == PC(side,PAWN));
        assert(pos->sqtopc[to] != EMPTY);
        if (to >= A8 || to <= H1) { // last rank => promotion
            moves[nmove++] = MOVE(from, to, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_BISHOP, MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_ROOK  , MV_FALSE, MV_FALSE);
            moves[nmove++] = MOVE(from, to, MV_PRM_QUEEN , MV_FALSE, MV_FALSE);
        } else {
            moves[nmove++] = SIMPLEMOVE(from, to);
        }
        posmoves &= (posmoves - 1);
    }    

    // en passant
    if (pos->enpassant != NO_ENPASSANT) {
        uint32_t epsq = pos->enpassant + 23;
        if (epsq != 24 && epsq != 32) {
            // try capture left
            from = epsq - 1;
            // TODO: shouldn't need to check that the ep square is occupied by a pawn on the other side
            if (pos->sqtopc[from] == PC(side,PAWN)) {                
                to = side == WHITE ? from + 9 : from - 7;
                if (pos->sqtopc[to] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert((side == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           (side == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert((side == WHITE && (from >= A5 && from <= H5)) ||
                           (side == BLACK && (from >= A4 && from <= H4)));
                    assert((side == WHITE && (to >= A6 && to <= H6)) ||
                           (side == BLACK && (to >= A3 && to <= H3)));
                    assert(pos->sqtopc[from] == PC(side,PAWN));
                    assert(pos->sqtopc[epsq] == PC(contraside,PAWN));
                    assert(pos->sqtopc[to] == EMPTY);
                    moves[nmove++] = MOVE(from, to, MV_FALSE, MV_TRUE, MV_FALSE);
                }
            }
        }
        if (epsq != 31 && epsq != 39) {
            // try capture right
            from = epsq + 1;
            // TODO: shouldn't need to check that the ep square is occupied by a pawn on the other side
            if (pos->sqtopc[from] == PC(side,PAWN)) {                
                to = side == WHITE ? from + 7 : from - 9;                
                if (pos->sqtopc[to] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert((side == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           (side == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert((side == WHITE && (from >= A5 && from <= H5)) ||
                           (side == BLACK && (from >= A4 && from <= H4)));
                    assert((side == WHITE && (to >= A6 && to <= H6)) ||
                           (side == BLACK && (to >= A3 && to <= H3)));
                    assert(pos->sqtopc[from] == PC(side,PAWN));
                    assert(pos->sqtopc[epsq] == PC(contraside,PAWN));
                    assert(pos->sqtopc[to] == EMPTY);
                    moves[nmove++] = MOVE(from, to, MV_FALSE, MV_TRUE, MV_FALSE);
                }
            }
        }
    }

    return nmove;
}
