divert(-1)
define(`funcname', `uint32_t gen_move_$1(const struct position *const restrict pos, move *restrict moves)')
define(`fullpcs', `pos->brd[PC($1,PAWN)]|pos->brd[PC($1,KNIGHT)]|pos->brd[PC($1,BISHOP)]|pos->brd[PC($1,ROOK)]|pos->brd[PC($1,QUEEN)]|pos->brd[PC($1,KING)]')
define(`pawns', `pos->brd[PC($1,PAWN)]')
define(`knights', `pos->brd[PC($1,KNIGHT)]')
define(`bishops', `pos->brd[PC($1,BISHOP)]')
define(`rooks', `pos->brd[PC($1,ROOK)]')
define(`queens', `pos->brd[PC($1,QUEEN)]')
define(`kings', `pos->brd[PC($1,KING)]')
define(`contra', `ifelse($1, `WHITE', `BLACK', `WHITE')')
define(`white_castling', `if ((castle & WKINGSD) != 0    &&
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
    }')
define(`black_castling', `if ((castle & BKINGSD) != 0    &&
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
    }')
define(`gen', `funcname(ifelse($1, `WHITE', `white', `black')) {
    uint32_t from;
    uint32_t to;
    uint64_t pcs;
    uint64_t posmoves;
    uint32_t nmove = 0;    
    const uint64_t same = fullpcs($1);
    const uint64_t `contra' = fullpcs(contra($1));
    const uint64_t occupied = same | `contra';
    const uint64_t opp_or_empty = ~same;
    const uint8_t castle = pos->castle;

    // knight moves
    pcs = knights($1);
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
    pcs = kings($1);
    assert(pcs != 0);
    while (pcs) {
        from = __builtin_ctzll(pcs);
        assert(from < 64 && from >= 0);
        assert(pos->sqtopc[from] == PC($1,KING));
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
    ifelse($1, `WHITE', white_castling, black_castling)

    // bishop moves
    pcs = bishops($1);
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
    pcs = rooks($1);
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
    pcs = queens($1);
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
    pcs = pawns($1);

    // forward 1 square
    posmoves = ifelse($1, `WHITE', `pcs << 8', `pcs >> 8');
    posmoves &= ~occupied;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = ifelse($1, `WHITE', `to - 8', `to + 8');
        assert(pos->sqtopc[from] == PC($1,PAWN));        
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
    posmoves = pcs & RANK2($1);
    posmoves = ifelse($1, `WHITE', `posmoves << 16', `posmoves >> 16');
    posmoves &= ~occupied;
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = ifelse($1, `WHITE', `to - 16', `to + 16');
        assert(pos->sqtopc[from] == PC($1,PAWN));
        // TODO(plesslie): do this with bitmasks?
        // make sure we aren't jumping over another piece
	//HERE: $1
        if (pos->sqtopc[ifelse($1, `WHITE', `to - 8', `to + 8')] == EMPTY) {
            moves[nmove++] = SIMPLEMOVE(from, to);            
        }        
        posmoves &= (posmoves - 1);
    }

    // capture left
    posmoves = pcs & ~A_FILE;
    posmoves = ifelse($1, `WHITE', `posmoves << 7', `posmoves >> 9');
    posmoves &= `contra';
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = ifelse($1, `WHITE', `to - 7', `to + 9');
        assert(pos->sqtopc[from] == PC($1,PAWN));
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
    posmoves = ifelse($1, `WHITE', `posmoves << 9', `posmoves >> 7');
    posmoves &= `contra';
    while (posmoves) {
        to = __builtin_ctzll(posmoves);
        from = ifelse($1, `WHITE', `to - 9', `to + 7');
        assert(pos->sqtopc[from] == PC($1,PAWN));
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
            if (pos->sqtopc[from] == PC($1,PAWN)) {                
                to = ifelse($1, `WHITE', `from + 9', `from - 7');
                if (pos->sqtopc[to] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert(($1 == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           ($1 == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert(($1 == WHITE && (from >= A5 && from <= H5)) ||
                           ($1 == BLACK && (from >= A4 && from <= H4)));
                    assert(($1 == WHITE && (to >= A6 && to <= H6)) ||
                           ($1 == BLACK && (to >= A3 && to <= H3)));
                    assert(pos->sqtopc[from] == PC($1,PAWN));
                    assert(pos->sqtopc[epsq] == PC(contra($1),PAWN));
                    assert(pos->sqtopc[to] == EMPTY);
                    moves[nmove++] = MOVE(from, to, MV_FALSE, MV_TRUE, MV_FALSE);
                }
            }
        }
        if (epsq != 31 && epsq != 39) {
            // try capture right
            from = epsq + 1;
            // TODO: shouldn't need to check that the ep square is occupied by a pawn on the other side
            if (pos->sqtopc[from] == PC($1,PAWN)) {                
                to = ifelse($1, `WHITE', `from + 7', `from - 9');
                if (pos->sqtopc[to] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert(($1 == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           ($1 == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert(($1 == WHITE && (from >= A5 && from <= H5)) ||
                           ($1 == BLACK && (from >= A4 && from <= H4)));
                    assert(($1 == WHITE && (to >= A6 && to <= H6)) ||
                           ($1 == BLACK && (to >= A3 && to <= H3)));
                    assert(pos->sqtopc[from] == PC($1,PAWN));
                    assert(pos->sqtopc[epsq] == PC(contra($1),PAWN));
                    assert(pos->sqtopc[to] == EMPTY);
                    moves[nmove++] = MOVE(from, to, MV_FALSE, MV_TRUE, MV_FALSE);
                }
            }
        }
    }

    return nmove;
})
divert
// ---- Generated file, DO NOT edit by hand (see movegen.m4) ----

static funcname(`white');

gen(`WHITE')

static funcname(`black');

gen(`BLACK')
