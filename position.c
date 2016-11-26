#include "position.h"
#include <string.h>
#include <assert.h>
#include <inttypes.h>

int position_from_fen(struct position *restrict pos, const char *fen) {
    int rank;
    int file;
    char c;
    int halfmoves = 0;
    int nmoves = 0;

    pos->nmoves = 1;
    pos->wtm = WHITE;
    pos->halfmoves = 0;
    pos->castle = CSL_NONE;
    pos->enpassant = EP_NONE;
    memset(&pos->sqtopc[0], EMPTY, sizeof(pos->sqtopc[0]) * 64);
    memset(&pos->brd[0], 0, sizeof(pos->brd[0]) * NPIECES * 2);
    pos->side[WHITE] = 0ull;
    pos->side[BLACK] = 0ull;

    // piece placements
    for (rank = RANK_8; rank >= RANK_1; --rank) {
	for (file = FILE_A; file <= FILE_H; ++file) {
	    c = *fen++;
	    switch (c) {
	    case 0:
		return 1;
	    case 'P':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, PAWN);
		pos->brd[PIECE(WHITE, PAWN)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));
		break;		
	    case 'N':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, KNIGHT);
		pos->brd[PIECE(WHITE, KNIGHT)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'B':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, BISHOP);
		pos->brd[PIECE(WHITE, BISHOP)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'R':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, ROOK);
		pos->brd[PIECE(WHITE, ROOK)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'Q':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, QUEEN);
		pos->brd[PIECE(WHITE, QUEEN)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'K':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, KING);
		pos->brd[PIECE(WHITE, KING)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'p':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, PAWN);
		pos->brd[PIECE(BLACK, PAWN)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));
		break;
	    case 'n':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, KNIGHT);
		pos->brd[PIECE(BLACK, KNIGHT)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'b':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, BISHOP);
		pos->brd[PIECE(BLACK, BISHOP)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'r':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, ROOK);
		pos->brd[PIECE(BLACK, ROOK)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'q':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, QUEEN);
		pos->brd[PIECE(BLACK, QUEEN)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'k':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, KING);
		pos->brd[PIECE(BLACK, KING)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    default:
		if (c >= '1' && c <= '8') {
		    file += c - '0' - 1; // file will get incremented by for loop
		} else {
		    return 2;
		}
	    }
	}
	assert(rank == RANK_1 || *fen == '/');
	assert(rank != RANK_1 || *fen == ' ');
	++fen;
    }

    // active color
    c = *fen++;
    switch (c) {
    case 'w':
	pos->wtm = WHITE;
	break;
    case 'b':
	pos->wtm = BLACK;
	break;
    default:
	return 3;
    }

    if (*fen++ != ' ') {
	return 4;
    }

    // castling availability
    while ((c = *fen++) != ' ') {
	switch (c) {
	case 0:
	    return 5;
	case '-':
	    pos->castle = CSL_NONE;
	    break;
	case 'K':
	    pos->castle |= CSL_WKSIDE;
	    break;
	case 'Q':
	    pos->castle |= CSL_WQSIDE;
	    break;
	case 'k':
	    pos->castle |= CSL_BKSIDE;
	    break;
	case 'q':
	    pos->castle |= CSL_BQSIDE;
	    break;
	default:
	    return 6;
	}
    }

    // en passant
    c = *fen++;
    if (c == 0) {
	return 7;
    } else if (c == '-') {
	pos->enpassant = EP_NONE;
    } else if (c >= 'a' && c <= 'h') {
	file = c - 'a';
	c = *fen++;
	if (c >= '1' && c <= '8') {
	    rank = c - '1';
	    if (rank == RANK_3) {
		pos->enpassant = SQUARE(file, rank);
	    } else if (rank == RANK_6) {
		pos->enpassant = SQUARE(file, rank);
	    } else {
		return 10;
	    }
	} else {
	    return 8;
	}
    } else {
	return 9;
    }
    
    assert((pos->enpassant == EP_NONE) ||
	   (pos->enpassant >= A3 && pos->enpassant <= H3) ||
	   (pos->enpassant >= A6 && pos->enpassant <= H6));

    if (*fen++ != ' ') {
	return 11;
    }

    // half moves
    while ((c = *fen++) != ' ') {
	if (c == 0) {
	    return 12;
	}
	if (c < '0' || c > '9') {
	    return 13;
	}
	halfmoves *= 10;
	halfmoves += c - '0';
    }
    pos->halfmoves = halfmoves;

    // full moves
    while ((c = *fen++)) {
	if (c < '0' || c > '9') {
	    return 14;
	}
	nmoves *= 10;
	nmoves += c - '0';
    }
    pos->nmoves = nmoves;
        
    return 0;
}

void position_print(FILE *os, const struct position *restrict pos) {
    fprintf(os, "+---+---+---+---+---+---+---+---+\n");
    for (int rank = RANK_8; rank >= RANK_1; --rank) {
	for (int file = FILE_A; file <= FILE_H; ++file) {
	    fprintf(os, "| %c ", visual_pcs[pos->sqtopc[SQUARE(file, rank)]]);
	}
	fprintf(os, "|\n+---+---+---+---+---+---+---+---+\n");
    }
    fprintf(os, "%s to move\n", pos->wtm == WHITE ? "White" : "Black");
    if ((pos->castle & CSL_WKSIDE) != 0) {
	fprintf(os, "K");
    }
    if ((pos->castle & CSL_WQSIDE) != 0) {
	fprintf(os, "Q");
    }
    if ((pos->castle & CSL_BKSIDE) != 0) {
	fprintf(os, "k");
    }
    if ((pos->castle & CSL_BQSIDE) != 0) {
	fprintf(os, "q");
    }
    fprintf(os, "\n");
    fprintf(os, "Half moves: %d\n", pos->halfmoves);
    fprintf(os, "Full moves: %d\n", pos->nmoves);
    if (pos->enpassant == EP_NONE) {
	fprintf(os, "En Passant target: none\n");
    } else {
	fprintf(os, "En Passant target: %s\n", sq_to_str[pos->enpassant]);
    }
}

int validate_position(struct position *restrict const pos) {
    int pc;
    int color;
    int contra;
    int sq;
    int white_kings = 0;
    int black_kings = 0;
    
    for (sq = A1; sq <= H8; ++sq) {
	if (pos->sqtopc[sq] == EMPTY) {
	    if ((pos->side[WHITE] & MASK(sq)) != 0) {
		printf("validate_position: sqtopc empty on %s, but white full bitboard is not empty\n", sq_to_str[sq]);
		return 1;
	    }
	    if ((pos->side[BLACK] & MASK(sq)) != 0) {
		printf("validate_position: sqtopc empty on %s, but black full bitboard is not empty\n", sq_to_str[sq]);
		return 2;
	    }
	} else {
	    pc = pos->sqtopc[sq];
	    color = PIECECOLOR(pc);
	    contra = FLIP(color);

	    // check full side bitboards
	    if ((pos->side[color] & MASK(sq)) == 0) {
		// full side board doesn't have this square occupied
		printf("validate_position: sqtopc has %c on %s, but %s (same) bitboard is empty\n",
		       visual_pcs[pc], sq_to_str[sq], COLORSTR(color));
		return 3;
	    }
	    if ((pos->side[contra] & MASK(sq)) != 0) {
		// other side's full board has this square occupied
		printf("validate_position: sqtopc has %c on %s, but %s (contra) bitboard is not empty\n",
		       visual_pcs[pc], sq_to_str[sq], COLORSTR(color));
		printf("%" PRIu64 "\n", pos->side[contra]);
		return 4;
	    }

	    // check piece bitboards
	    if ((pos->brd[pc] & MASK(sq)) == 0) {
		printf("validate_position: sqtoc has %c on %s, but bitboard does not\n",
		       visual_pcs[pc], sq_to_str[sq]);
		return 5;
	    }

	    if (pc == PIECE(WHITE, KING)) {
		++white_kings;
	    }
	    if (pc == PIECE(BLACK, KING)) {
		++black_kings;
	    }
	}
    }

    for (pc = PIECE(WHITE, KNIGHT); pc <= PIECE(BLACK, KING); ++pc) {
	for (sq = A1; sq <= H8; ++sq) {
	    if ((pos->brd[pc] & MASK(sq)) != 0) {
		if (pos->sqtopc[sq] != pc) {
		    printf("validate_position: pos->brd[%c] has a piece on %s, sqtopc has %c\n",
			   visual_pcs[pc], sq_to_str[sq], visual_pcs[pos->sqtopc[sq]]);
		    return 6;
		}
	    }
	}
    }

    if (white_kings != 1) {
	printf("validate_position: %d white kings found!\n", white_kings);
	return 7;
    }
    if (black_kings != 1) {
	printf("validate_position: %d black kings found!\n", black_kings);	
	return 8;
    }

    for (sq = A1; sq <= H1; ++sq) {
	if (pos->sqtopc[sq] == PIECE(WHITE, PAWN)) {
	    printf("validate_position: white pawn on %s\n", sq_to_str[sq]);
	    return 9;
	} else if (pos->sqtopc[sq] == PIECE(BLACK, PAWN)) {
	    printf("validate_position: black pawn on %s\n", sq_to_str[sq]);
	    return 10;
	}
    }

    if ((pos->castle & CSL_WQSIDE) != 0) {
	if (pos->sqtopc[A1] != PIECE(WHITE, ROOK)) {
	    return 11;
	} else if (pos->sqtopc[E1] != PIECE(WHITE, KING)) {
	    return 12;
	}
    } else if ((pos->castle & CSL_WKSIDE) != 0) {
	if (pos->sqtopc[H1] != PIECE(WHITE, ROOK)) {
	    return 13;
	} else if (pos->sqtopc[E1] != PIECE(WHITE, KING)) {
	    return 14;
	}
    } else if ((pos->castle & CSL_BQSIDE) != 0) {
	if (pos->sqtopc[A8] != PIECE(BLACK, ROOK)) {
	    return 15;
	} else if (pos->sqtopc[E8] != PIECE(BLACK, KING)) {
	    return 16;
	}
    } else if ((pos->castle & CSL_BKSIDE) != 0) {
	if (pos->sqtopc[H8] != PIECE(BLACK, ROOK)) {
	    return 17;
	} else if (pos->sqtopc[E8] != PIECE(BLACK, KING)) {
	    return 18;
	}
    }
    
    return 0;
}

extern void make_move(struct position *restrict pos, struct savepos *restrict sp, move m) {
    const uint8_t  side      = pos->wtm;
    const uint8_t  contra    = FLIP(side);
    const uint32_t tosq      = TO(m);
    const uint32_t fromsq    = FROM(m);
    const uint64_t from      = MASK(fromsq);
    const uint64_t to        = MASK(tosq);    
    const uint32_t pc        = pos->sqtopc[fromsq];
    const uint32_t topc      = pos->sqtopc[tosq];
    const uint32_t flags     = FLAGS(m);
    const int      promopc   = PIECE(side, PROMO_PC(m));
    uint64_t *restrict pcs   = &pos->brd[pc];
    uint8_t  *restrict s2p   = pos->sqtopc;
    uint64_t *restrict rooks = &pos->brd[PIECE(side, ROOK)];
    int epsq;

    assert(tosq != fromsq);
    assert(topc != PIECE(WHITE, KING) && topc != PIECE(BLACK, KING));
    
    // update savepos
    sp->halfmoves = pos->halfmoves;
    sp->enpassant = pos->enpassant;
    sp->castle = pos->castle;
    sp->was_ep = 0;
    sp->captured_pc = topc;

    pos->enpassant = EP_NONE;

    switch (flags) {
    case FLG_NONE:
	*pcs &= ~from;
	*pcs |= to;
	s2p[fromsq] = EMPTY;
	s2p[tosq] = pc;
	pos->side[side] &= ~from;
	pos->side[side] |= to;

	// capture?
	if (topc != EMPTY) {
	    pos->brd[topc] &= ~to;
	    pos->side[contra] &= ~to;
	    switch (tosq) {
	    case A8: pos->castle &= ~CSL_BQSIDE; break;
	    case H8: pos->castle &= ~CSL_BKSIDE; break;
	    case A1: pos->castle &= ~CSL_WQSIDE; break;
	    case H1: pos->castle &= ~CSL_WKSIDE; break;
	    default: break;
	    }
	} else if (pc == PIECE(side, PAWN) && (from & RANK2(side)) && (to & EP_SQUARES(side))) {
	    pos->enpassant = side == WHITE ? tosq - 8 : tosq + 8;
	    assert((pos->enpassant >= A3 && pos->enpassant <= H3) ||
		   (pos->enpassant >= A6 && pos->enpassant <= H6));
	}

	if (pc == PIECE(side, KING)) {
	    pos->castle &= ~CSL_SIDE(side);
	} else if (pc == PIECE(side, ROOK)) {
	    switch (fromsq) {
	    case A1: pos->castle &= ~CSL_WQSIDE; break;
	    case H1: pos->castle &= ~CSL_WKSIDE; break;
	    case A8: pos->castle &= ~CSL_BQSIDE; break;
	    case H8: pos->castle &= ~CSL_BKSIDE; break;
	    default: break;
	    }
	}
	break;
    case FLG_EP:
	sp->was_ep = 1;
	if (side == WHITE) {
	    epsq = tosq - 8;
	    *pcs &= ~from;
	    *pcs |= to;
	    pos->brd[PIECE(BLACK, PAWN)] &= ~MASK(epsq);
	    s2p[epsq] = EMPTY;
	    s2p[fromsq] = EMPTY;
	    s2p[tosq] = PIECE(WHITE, PAWN);
	    pos->side[WHITE] &= ~from;
	    pos->side[WHITE] |= to;
	    pos->side[BLACK] &= ~MASK(epsq);
	} else {
	    epsq = tosq + 8;
	    *pcs &= ~from;
	    *pcs |= to;
	    pos->brd[PIECE(WHITE, PAWN)] &= ~MASK(epsq);
	    s2p[epsq] = EMPTY;
	    s2p[fromsq] = EMPTY;
	    s2p[tosq] = PIECE(BLACK, PAWN);
	    pos->side[BLACK] &= ~from;
	    pos->side[BLACK] |= to;
	    pos->side[WHITE] &= ~MASK(epsq);
	}
	break;
    case FLG_PROMO:
	*pcs              &= ~from;
	pos->brd[promopc] |= to;
	s2p[tosq]       = promopc;
	s2p[fromsq]     = EMPTY;
	pos->side[side] &= ~from;
	pos->side[side] |= to;
	if (topc != EMPTY) {
	    pos->brd[topc]    &= ~to;
	    pos->side[contra] &= ~to;
	    switch (tosq) {
	    case A1: pos->castle &= ~CSL_WQSIDE; break;
	    case H1: pos->castle &= ~CSL_WKSIDE; break;
	    case A8: pos->castle &= ~CSL_BQSIDE; break;
	    case H8: pos->castle &= ~CSL_BKSIDE; break;
	    default: break;
	    }
	}
	break;
    case FLG_CASTLE:
	assert(pc == PIECE(side, KING));
	if (side == WHITE) {
	    assert(fromsq == E1);
	    assert(tosq == C1 || tosq == G1);
	    if (tosq == G1) { // king side castle
		assert(s2p[E1] == PIECE(WHITE, KING));
		assert(s2p[F1] == EMPTY);
		assert(s2p[G1] == EMPTY);
		assert(s2p[H1] == PIECE(WHITE, ROOK));
		*pcs &= ~MASK(E1);
		*pcs |= MASK(G1);
		*rooks &= ~MASK(H1);
		*rooks |= MASK(F1);
		s2p[E1] = EMPTY;
		s2p[F1] = PIECE(WHITE, ROOK);
		s2p[G1] = PIECE(WHITE, KING);
		s2p[H1] = EMPTY;
		pos->side[side] &= ~(MASK(E1) | MASK(H1));
		pos->side[side] |= (MASK(F1) | MASK(G1));
	    } else {          // queen side castle
		assert(s2p[E1] == PIECE(WHITE, KING));
		assert(s2p[D1] == EMPTY);
		assert(s2p[C1] == EMPTY);
		assert(s2p[B1] == EMPTY);
		assert(s2p[A1] == PIECE(WHITE, KING));
		*pcs   &= ~MASK(E1);
                *pcs   |= MASK(C1);
                *rooks &= ~MASK(A1);
                *rooks |= MASK(D1);
                s2p[A1] = EMPTY;
                s2p[B1] = EMPTY;
                s2p[C1] = PIECE(WHITE, KING);
                s2p[D1] = PIECE(WHITE, ROOK);
                s2p[E1] = EMPTY;
		pos->side[side] &= ~(MASK(E1) | MASK(A1));
		pos->side[side] |= (MASK(C1) | MASK(D1));
	    }
	    pos->castle &= ~(CSL_WQSIDE | CSL_WKSIDE);
	} else {
	    assert(fromsq == E8);
	    assert(tosq == C8 || tosq == G8);
	    if (tosq == G8) { // king side castle
                assert(s2p[E8] == PIECE(WHITE, KING));
                assert(s2p[F8] == EMPTY);
                assert(s2p[G8] == EMPTY);
                assert(s2p[H8] == PIECE(WHITE, ROOK));
                *pcs   &= ~MASK(E8);
                *pcs   |= MASK(G8);
                *rooks &= ~MASK(H8);
                *rooks |= MASK(F8);
                s2p[E8] = EMPTY;
                s2p[F8] = PIECE(BLACK, ROOK);
                s2p[G8] = PIECE(BLACK, KING);
                s2p[H8] = EMPTY;
		pos->side[side] &= ~(MASK(E8) | MASK(H8));
		pos->side[side] |= (MASK(F8) | MASK(G8));
	    } else {          // queen side castle
                assert(s2p[E8] == PIECE(BLACK, KING));
                assert(s2p[D8] == EMPTY);
                assert(s2p[C8] == EMPTY);
                assert(s2p[B8] == EMPTY);
                assert(s2p[A8] == PIECE(BLACK, ROOK));
                *pcs   &= ~MASK(E8);
                *pcs   |= MASK(C8);
                *rooks &= ~MASK(A8);
                *rooks |= MASK(D8);
                s2p[A8] = EMPTY;
                s2p[B8] = EMPTY;
                s2p[C8] = PIECE(BLACK, KING);
                s2p[D8] = PIECE(BLACK, ROOK);
                s2p[E8] = EMPTY;
		pos->side[side] &= ~(MASK(E8) | MASK(A8));
		pos->side[side] |= (MASK(C8) | MASK(D8));		
	    }
	    pos->castle &= ~(CSL_BQSIDE | CSL_BKSIDE);
	}
	break;
    default:
	assert(0);
    }

    pos->wtm = contra;
    if (pc == PIECE(side, PAWN) || topc != EMPTY) {
	pos->halfmoves = 0;
    } else {
	++pos->halfmoves;
    }
    ++pos->nmoves;

    assert(pos->enpassant == EP_NONE || pc == PIECE(side,PAWN));
    assert(validate_position(pos) == 0);
}

void undo_move(struct position *restrict pos, const struct savepos *restrict sp, move m) {
    const uint8_t side     = FLIP(pos->wtm);
    const uint32_t fromsq  = FROM(m);
    const uint32_t tosq    = TO(m);
    const uint32_t promo   = PROMO_PC(m);
    const uint32_t promopc = PIECE(side, promo);
    const uint32_t flags   = FLAGS(m);
    const uint32_t pc      = pos->sqtopc[tosq];
    const uint32_t cappc   = sp->captured_pc;
    const uint64_t from    = MASK(fromsq);
    const uint64_t to      = MASK(tosq); // REVISIT: all uses of `to' are `~to' so just calculate that?
    // REVISIT: make sp->enpassant be the captured pawn square when FLG_EP?
    //const uint32_t epsq    = sp->enpassant; 
    const uint32_t epsq = side == WHITE ? tosq - 8 : tosq + 8;
    uint64_t *restrict pcs = &pos->brd[pc];
    uint8_t  *restrict s2p = pos->sqtopc;
    uint64_t *restrict sidebb = &pos->side[side];
    uint64_t *restrict contrabb = &pos->side[pos->wtm];    
    
    assert(validate_position(pos) == 0);
    assert(fromsq >= A1 && fromsq <= H8);
    assert(tosq   >= A1 && tosq   <= H8);
    assert(side == WHITE || side == BLACK);
    assert(flags == FLG_NONE  ||
	   flags == FLG_EP    ||
	   flags == FLG_PROMO ||
	   flags == FLG_CASTLE);
    assert((side == WHITE && (pc >= PIECE(WHITE, KNIGHT) && pc <= PIECE(WHITE, KING))) ||
	   (side == BLACK && (pc >= PIECE(BLACK, KNIGHT) && pc <= PIECE(BLACK, KING))));
    assert(cappc == EMPTY || (cappc >= PIECE(WHITE, KNIGHT) && cappc <= PIECE(BLACK, KING)));
    assert(flags != FLG_PROMO || (promopc >= PIECE(side, KNIGHT) && promopc <= PIECE(side, KING)));

    pos->halfmoves = sp->halfmoves;
    pos->enpassant = sp->enpassant;
    pos->castle = sp->castle;
    pos->wtm = side;
    --pos->nmoves;

    switch (flags) {
    case FLG_NONE:
	s2p[fromsq] = pc;
	*pcs |= from;
	*pcs &= ~to;
	*sidebb |= from;
	*sidebb &= ~to;
	s2p[tosq] = cappc;
	if (cappc != EMPTY) {
	    pos->brd[cappc] |= to;
	    *contrabb |= to;
	}
	break;
    case FLG_EP:
	s2p[fromsq] = pc;
        *pcs |= from;
        *pcs &= ~to;
	*sidebb |= from;
	*sidebb &= ~to;
	*contrabb |= MASK(epsq);	
        s2p[tosq] = EMPTY;
        if (side == WHITE) {
            assert(epsq == (tosq - 8));
            s2p[epsq] = PIECE(BLACK,PAWN);
            pos->brd[PIECE(BLACK,PAWN)] |= MASK(epsq);
        } else {
            assert(epsq == (tosq + 8));            
            s2p[epsq] = PIECE(WHITE,PAWN);
            pos->brd[PIECE(WHITE,PAWN)] |= MASK(epsq);            
        }
	break;
    case FLG_PROMO:
        pos->brd[PIECE(side,PAWN)] |= from;
        pos->brd[promopc] &= ~to;	
	s2p[tosq] = cappc;
	s2p[fromsq] = PIECE(side, PAWN);
	*sidebb |= from;
	*sidebb &= ~to;
	if (cappc != EMPTY) {
	    pos->brd[cappc] |= to;
	    *contrabb |= to;
	}
	break;
    case FLG_CASTLE:
        assert(pc == PIECE(side,KING));
	switch (tosq) {
	case C1:
            assert(fromsq == E1);
            assert(s2p[A1] == EMPTY);
            assert(s2p[B1] == EMPTY);
            assert(s2p[C1] == PIECE(WHITE,KING));
            assert(s2p[D1] == PIECE(WHITE,ROOK));
            assert(s2p[E1] == EMPTY);
            s2p[A1] = PIECE(WHITE,ROOK);
            s2p[B1] = EMPTY;
            s2p[C1] = EMPTY;
            s2p[D1] = EMPTY;
            s2p[E1] = PIECE(WHITE,KING);
	    *sidebb |= MASK(A1) | MASK(E1);
	    *sidebb &= ~(MASK(B1) | MASK(C1) | MASK(D1));
            pos->brd[PIECE(WHITE,ROOK)] &= ~MASK(D1);
            pos->brd[PIECE(WHITE,ROOK)] |= MASK(A1);
            pos->brd[PIECE(WHITE,KING)] &= ~MASK(C1);
            pos->brd[PIECE(WHITE,KING)] |= MASK(E1);
	    break;
	case G1:
            assert(fromsq == E1);
            assert(s2p[E1] == EMPTY);
            assert(s2p[F1] == PIECE(WHITE,ROOK));
            assert(s2p[G1] == PIECE(WHITE,KING));
            assert(s2p[H1] == EMPTY);
            s2p[E1] = PIECE(WHITE,KING);
            s2p[F1] = EMPTY;
            s2p[G1] = EMPTY;
            s2p[H1] = PIECE(WHITE,ROOK);
	    *sidebb |= MASK(E1) | MASK(H1);
	    *sidebb &= ~(MASK(F1) | MASK(G1));
            pos->brd[PIECE(WHITE,KING)] &= ~MASK(G1);
            pos->brd[PIECE(WHITE,KING)] |= MASK(E1);
            pos->brd[PIECE(WHITE,ROOK)] &= ~MASK(F1);
            pos->brd[PIECE(WHITE,ROOK)] |= MASK(H1);
	    break;
	case C8:
            assert(fromsq == E8);
            assert(s2p[A8] == EMPTY);
            assert(s2p[B8] == EMPTY);
            assert(s2p[C8] == PIECE(BLACK,KING));
            assert(s2p[D8] == PIECE(BLACK,ROOK));
            assert(s2p[E8] == EMPTY);
            s2p[A8] = PIECE(BLACK,ROOK);
            s2p[B8] = EMPTY;
            s2p[C8] = EMPTY;
            s2p[D8] = EMPTY;
            s2p[E8] = PIECE(BLACK,KING);
	    *sidebb |= MASK(A8) | MASK(E8);
	    *sidebb &= ~(MASK(B8) | MASK(C8) | MASK(D8));	    
            pos->brd[PIECE(BLACK,ROOK)] &= ~MASK(D8);
            pos->brd[PIECE(BLACK,ROOK)] |= MASK(A8);
            pos->brd[PIECE(BLACK,KING)] &= ~MASK(C8);
            pos->brd[PIECE(BLACK,KING)] |= MASK(E8);
	    break;
	case G8:
            assert(fromsq == E8);
            assert(s2p[E8] == EMPTY);
            assert(s2p[F8] == PIECE(BLACK,ROOK));
            assert(s2p[G8] == PIECE(BLACK,KING));
            assert(s2p[H8] == EMPTY);
            s2p[E8] = PIECE(BLACK,KING);
            s2p[F8] = EMPTY;
            s2p[G8] = EMPTY;
            s2p[H8] = PIECE(BLACK,ROOK);
	    *sidebb |= MASK(E8) | MASK(H8);
	    *sidebb &= ~(MASK(F8) | MASK(G8));	    
            pos->brd[PIECE(BLACK,KING)] &= ~MASK(G8);
            pos->brd[PIECE(BLACK,KING)] |= MASK(E8);
            pos->brd[PIECE(BLACK,ROOK)] &= ~MASK(F8);
            pos->brd[PIECE(BLACK,ROOK)] |= MASK(H8);
	    break;
	default:
	    unreachable();
	    break;
	}
	break;
    default:
	unreachable();
    }
    
    assert(validate_position(pos) == 0);
}

