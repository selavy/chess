
// ---- Generated file, DO NOT edit by hand (see movegen.m4) ----

static uint32_t gen_move_white(const struct position *const restrict pos, move *restrict moves);

uint32_t gen_move_white(const struct position *const restrict pos, move *restrict moves) {
    uint32_t from;
    uint32_t to;
    uint64_t pcs;
    uint64_t posmoves;
    uint32_t nmove = 0;    
    const uint64_t same = pos->brd[PC(WHITE,PAWN)]|pos->brd[PC(WHITE,KNIGHT)]|pos->brd[PC(WHITE,BISHOP)]|pos->brd[PC(WHITE,ROOK)]|pos->brd[PC(WHITE,QUEEN)]|pos->brd[PC(WHITE,KING)];
    const uint64_t contra = pos->brd[PC(BLACK,PAWN)]|pos->brd[PC(BLACK,KNIGHT)]|pos->brd[PC(BLACK,BISHOP)]|pos->brd[PC(BLACK,ROOK)]|pos->brd[PC(BLACK,QUEEN)]|pos->brd[PC(BLACK,KING)];
    const uint64_t occupied = same | contra;
    const uint64_t opp_or_empty = ~same;
    const uint8_t castle = pos->castle;

    // knight moves
    pcs = pos->brd[PC(WHITE,KNIGHT)];
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
    pcs = pos->brd[PC(WHITE,KING)];
    assert(pcs != 0);
    while (pcs) {
        from = __builtin_ctzll(pcs);
        assert(from < 64 && from >= 0);
        assert(pos->sqtopc[from] == PC(WHITE,KING));
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
    if ((castle & WKINGSD) != 0    &&
        (from == E1)               &&
        (pos->sqtopc[F1] == EMPTY) &&
        (pos->sqtopc[G1] == EMPTY) &&
        (attacks(pos, BLACK, E1) == 0) &&
        (attacks(pos, BLACK, F1) == 0) &&
        (attacks(pos, BLACK, G1) == 0)) {
        assert(pos->sqtopc[H1] == PC(WHITE,ROOK));
        moves[nmove++] = MOVE(E1, G1, MV_FALSE, MV_FALSE, MV_TRUE);
    }
    if ((castle & WQUEENSD) != 0   &&
        (from == E1)               &&
        (pos->sqtopc[D1] == EMPTY) &&
        (pos->sqtopc[C1] == EMPTY) &&
        (pos->sqtopc[B1] == EMPTY) &&
        (attacks(pos, BLACK, E1) == 0) &&
        (attacks(pos, BLACK, D1) == 0) &&
        (attacks(pos, BLACK, C1) == 0)) {
        assert(pos->sqtopc[A1] == PC(WHITE,ROOK));
        moves[nmove++] = MOVE(E1, C1, MV_FALSE, MV_FALSE, MV_TRUE);
    }

    // bishop moves
    pcs = pos->brd[PC(WHITE,BISHOP)];
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
    pcs = pos->brd[PC(WHITE,ROOK)];
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
    pcs = pos->brd[PC(WHITE,QUEEN)];
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
    pcs = pos->brd[PC(WHITE,PAWN)];

    // forward 1 square
    posmoves = pcs << 8;
    posmoves &= ~occupied;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = to - 8;
        assert(pos->sqtopc[from] == PC(WHITE,PAWN));        
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
    posmoves = pcs & RANK2(WHITE);
    posmoves = posmoves << 16;
    posmoves &= ~occupied;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = to - 16;
        assert(pos->sqtopc[from] == PC(WHITE,PAWN));
        // TODO(plesslie): do this with bitmasks?
        // make sure we arent jumping over another piece
	//HERE: WHITE
        if (pos->sqtopc[to + 8] == EMPTY) {
            moves[nmove++] = SIMPLEMOVE(from, to);            
        }        
        posmoves &= (posmoves - 1);
    }

    // capture left
    posmoves = pcs & ~A_FILE;
    posmoves = posmoves >> 9;
    posmoves &= WHITE;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = to + 9;
        assert(pos->sqtopc[from] == PC(WHITE,PAWN));
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
    posmoves = posmoves >> 7;
    posmoves &= WHITE;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = to + 7;
        assert(pos->sqtopc[from] == PC(WHITE,PAWN));
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
            if (pos->sqtopc[from] == PC(WHITE,PAWN)) {                
                to = from - 7;
                if (pos->sqtopc[to] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert((WHITE == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           (WHITE == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert((WHITE == WHITE && (from >= A5 && from <= H5)) ||
                           (WHITE == BLACK && (from >= A4 && from <= H4)));
                    assert((WHITE == WHITE && (to >= A6 && to <= H6)) ||
                           (WHITE == BLACK && (to >= A3 && to <= H3)));
                    assert(pos->sqtopc[from] == PC(WHITE,PAWN));
                    assert(pos->sqtopc[epsq] == PC(WHITE,PAWN));
                    assert(pos->sqtopc[to] == EMPTY);
                    moves[nmove++] = MOVE(from, to, MV_FALSE, MV_TRUE, MV_FALSE);
                }
            }
        }
        if (epsq != 31 && epsq != 39) {
            // try capture right
            from = epsq + 1;
            // TODO: shouldn't need to check that the ep square is occupied by a pawn on the other side
            if (pos->sqtopc[from] == PC(WHITE,PAWN)) {                
                to = from - 9;
                if (pos->sqtopc[to] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert((WHITE == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           (WHITE == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert((WHITE == WHITE && (from >= A5 && from <= H5)) ||
                           (WHITE == BLACK && (from >= A4 && from <= H4)));
                    assert((WHITE == WHITE && (to >= A6 && to <= H6)) ||
                           (WHITE == BLACK && (to >= A3 && to <= H3)));
                    assert(pos->sqtopc[from] == PC(WHITE,PAWN));
                    assert(pos->sqtopc[epsq] == PC(WHITE,PAWN));
                    assert(pos->sqtopc[to] == EMPTY);
                    moves[nmove++] = MOVE(from, to, MV_FALSE, MV_TRUE, MV_FALSE);
                }
            }
        }
    }

    return nmove;
}

static uint32_t gen_move_black(const struct position *const restrict pos, move *restrict moves);

uint32_t gen_move_black(const struct position *const restrict pos, move *restrict moves) {
    uint32_t from;
    uint32_t to;
    uint64_t pcs;
    uint64_t posmoves;
    uint32_t nmove = 0;    
    const uint64_t same = pos->brd[PC(BLACK,PAWN)]|pos->brd[PC(BLACK,KNIGHT)]|pos->brd[PC(BLACK,BISHOP)]|pos->brd[PC(BLACK,ROOK)]|pos->brd[PC(BLACK,QUEEN)]|pos->brd[PC(BLACK,KING)];
    const uint64_t contra = pos->brd[PC(WHITE,PAWN)]|pos->brd[PC(WHITE,KNIGHT)]|pos->brd[PC(WHITE,BISHOP)]|pos->brd[PC(WHITE,ROOK)]|pos->brd[PC(WHITE,QUEEN)]|pos->brd[PC(WHITE,KING)];
    const uint64_t occupied = same | contra;
    const uint64_t opp_or_empty = ~same;
    const uint8_t castle = pos->castle;

    // knight moves
    pcs = pos->brd[PC(BLACK,KNIGHT)];
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
    pcs = pos->brd[PC(BLACK,KING)];
    assert(pcs != 0);
    while (pcs) {
        from = __builtin_ctzll(pcs);
        assert(from < 64 && from >= 0);
        assert(pos->sqtopc[from] == PC(BLACK,KING));
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
    if ((castle & BKINGSD) != 0    &&
        (from == E8)               &&
        (pos->sqtopc[F8] == EMPTY) &&
        (pos->sqtopc[G8] == EMPTY) &&
        (attacks(pos, WHITE, E8) == 0) &&
        (attacks(pos, WHITE, F8) == 0) &&
        (attacks(pos, WHITE, G8) == 0)) {
        assert(pos->sqtopc[H8] == PC(BLACK,ROOK));
        moves[nmove++] = MOVE(E8, G8, MV_FALSE, MV_FALSE, MV_TRUE);
    }
    if ((castle & BQUEENSD) != 0   &&
        (from == E8)               &&
        (pos->sqtopc[D8] == EMPTY) &&
        (pos->sqtopc[C8] == EMPTY) &&
        (pos->sqtopc[B8] == EMPTY) &&
        (attacks(pos, WHITE, E8) == 0) &&
        (attacks(pos, WHITE, D8) == 0) &&
        (attacks(pos, WHITE, C8) == 0)) {
        assert(pos->sqtopc[A8] == PC(BLACK,ROOK));
        moves[nmove++] = MOVE(E8, C8, MV_FALSE, MV_FALSE, MV_TRUE);
    }

    // bishop moves
    pcs = pos->brd[PC(BLACK,BISHOP)];
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
    pcs = pos->brd[PC(BLACK,ROOK)];
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
    pcs = pos->brd[PC(BLACK,QUEEN)];
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
    pcs = pos->brd[PC(BLACK,PAWN)];

    // forward 1 square
    posmoves = pcs >> 8;
    posmoves &= ~occupied;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = to + 8;
        assert(pos->sqtopc[from] == PC(BLACK,PAWN));        
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
    posmoves = pcs & RANK2(BLACK);
    posmoves = posmoves >> 16;
    posmoves &= ~occupied;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = to + 16;
        assert(pos->sqtopc[from] == PC(BLACK,PAWN));
        // TODO(plesslie): do this with bitmasks?
        // make sure we arent jumping over another piece
	//HERE: BLACK
        if (pos->sqtopc[to + 8] == EMPTY) {
            moves[nmove++] = SIMPLEMOVE(from, to);            
        }        
        posmoves &= (posmoves - 1);
    }

    // capture left
    posmoves = pcs & ~A_FILE;
    posmoves = posmoves >> 9;
    posmoves &= WHITE;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = to + 9;
        assert(pos->sqtopc[from] == PC(BLACK,PAWN));
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
    posmoves = posmoves >> 7;
    posmoves &= WHITE;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = to + 7;
        assert(pos->sqtopc[from] == PC(BLACK,PAWN));
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
            if (pos->sqtopc[from] == PC(BLACK,PAWN)) {                
                to = from - 7;
                if (pos->sqtopc[to] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert((BLACK == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           (BLACK == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert((BLACK == WHITE && (from >= A5 && from <= H5)) ||
                           (BLACK == BLACK && (from >= A4 && from <= H4)));
                    assert((BLACK == WHITE && (to >= A6 && to <= H6)) ||
                           (BLACK == BLACK && (to >= A3 && to <= H3)));
                    assert(pos->sqtopc[from] == PC(BLACK,PAWN));
                    assert(pos->sqtopc[epsq] == PC(WHITE,PAWN));
                    assert(pos->sqtopc[to] == EMPTY);
                    moves[nmove++] = MOVE(from, to, MV_FALSE, MV_TRUE, MV_FALSE);
                }
            }
        }
        if (epsq != 31 && epsq != 39) {
            // try capture right
            from = epsq + 1;
            // TODO: shouldn't need to check that the ep square is occupied by a pawn on the other side
            if (pos->sqtopc[from] == PC(BLACK,PAWN)) {                
                to = from - 9;
                if (pos->sqtopc[to] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert((BLACK == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           (BLACK == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert((BLACK == WHITE && (from >= A5 && from <= H5)) ||
                           (BLACK == BLACK && (from >= A4 && from <= H4)));
                    assert((BLACK == WHITE && (to >= A6 && to <= H6)) ||
                           (BLACK == BLACK && (to >= A3 && to <= H3)));
                    assert(pos->sqtopc[from] == PC(BLACK,PAWN));
                    assert(pos->sqtopc[epsq] == PC(WHITE,PAWN));
                    assert(pos->sqtopc[to] == EMPTY);
                    moves[nmove++] = MOVE(from, to, MV_FALSE, MV_TRUE, MV_FALSE);
                }
            }
        }
    }

    return nmove;
}
