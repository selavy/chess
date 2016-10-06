#include "movegen.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "read_fen.h"

//#define EXTRA_INFO

//#include "first.c"

void make_move(struct position *restrict p, move m, struct savepos *restrict sp) {
    // --- loads ---
    const uint8_t  side    = p->wtm;
    const uint8_t  contra  = FLIP(side);    
    const uint32_t fromsq  = FROM(m);
    const uint32_t tosq    = TO(m);
    const uint32_t promo   = PROMOPC[PROMO_PC(m)]; // only valid if promo flag set
    const uint32_t promopc = PC(side,promo);
    const uint32_t flags   = FLAGS(m);
    const uint32_t pc      = p->sqtopc[fromsq];
    const uint32_t topc    = p->sqtopc[tosq];
    const uint64_t from    = MASK(fromsq);
    const uint64_t to      = MASK(tosq);
    const uint32_t epsq    = p->enpassant + 23;       // only valid if ep flag set

    uint64_t *restrict pcs = &p->brd[pc];
    uint8_t  *restrict s2p = p->sqtopc;
    
    #ifdef EXTRA_INFO
    printf("make_move: fromsq(%s) tosq(%s) promo(%c) flags(%u) side(%s) "
           "contra(%s) pc(%c) topc(%c) from(0x%08" PRIX64 ") to(0x%08" PRIX64 ") "
           "epsq(%s)\n",
           sq_to_str[fromsq], sq_to_str[tosq], flags==FLG_PROMO?vpcs[promo]:' ', flags, side==WHITE?"WHITE":"BLACK",
           contra==WHITE?"WHITE":"BLACK", vpcs[pc], vpcs[topc], from, to, sq_to_str[epsq]);
    #endif

    // --- validate position before ---
    assert(validate_position(p) == 0);
    assert(fromsq >= A1 && fromsq <= H8);
    assert(tosq   >= A1 && tosq   <= H8);
    assert(epsq   >= A1 && epsq   <= H8);
    assert(side   == WHITE || side   == BLACK);
    assert(contra == WHITE || contra == BLACK);
    assert(flags >= FLG_NONE && flags <= FLG_CASTLE);
    assert(pc   >= PC(WHITE,PAWN) && pc   <= EMPTY);
    assert(topc >= PC(WHITE,PAWN) && topc <= EMPTY);
    assert(promo != FLG_PROMO || (promopc >= PC(side,KNIGHT) && promopc <= PC(side,QUEEN)));

    // --- update savepos ---
    sp->halfmoves   = p->halfmoves;
    sp->enpassant   = p->enpassant;
    sp->castle      = p->castle;
    sp->was_ep      = MV_FALSE;
    sp->captured_pc = topc;

    p->enpassant = NO_ENPASSANT;
            
    // TODO(plesslie): make this a jump table? (or switch)
    if (flags == FLG_NONE) { // normal case
        *pcs &= ~from;
        *pcs |= to;
        s2p[fromsq] = EMPTY;
        s2p[tosq]   = pc;

        if (topc != EMPTY) { // capture
            p->brd[topc] &= ~to;
            if (tosq == A8) {
                p->castle &= ~BQUEENSD;
            } else if (tosq == H8) {
                p->castle &= ~BKINGSD;
            } else if (tosq == A1) {
                p->castle &= ~WQUEENSD;
            } else if (tosq == H1) {
                p->castle &= ~WKINGSD;
            }
        } else if (pc == PC(side,PAWN) && (from & RANK2(side)) && (to & EP_SQUARES(side))) {
            p->enpassant = tosq - 23;
        }
        if (pc == PC(side,KING)) {
            p->castle &= ~CSL(side);
        } else if (pc == PC(side,ROOK)) {
            switch (fromsq) {
            case A1: p->castle &= ~WQUEENSD; break;
            case H1: p->castle &= ~WKINGSD;  break;
            case A8: p->castle &= ~BQUEENSD; break;
            case H8: p->castle &= ~BKINGSD;  break;
            default: break;
            }            
        }
    } else if (flags == FLG_EP) {
        sp->was_ep = 1;
        if (side == WHITE) {
            assert(tosq >= A6 && tosq <= H6);
            assert(topc == EMPTY);
            assert(epsq != NO_ENPASSANT);
            assert(s2p[epsq] == PC(BLACK,PAWN));
            assert(epsq == (tosq - 8));
            *pcs &= ~from;
            *pcs |= to;
            p->brd[PC(BLACK,PAWN)] &= ~MASK(epsq);
            s2p[epsq] = EMPTY;
            s2p[fromsq] = EMPTY;            
            s2p[tosq] = PC(WHITE,PAWN);            
        } else {
            assert(tosq >= A3 && tosq <= H3);
            assert(topc == EMPTY);
            assert(epsq != NO_ENPASSANT);
            assert(s2p[epsq] == PC(WHITE,PAWN));
            assert(epsq == (tosq + 8));
            *pcs &= ~from;
            *pcs |= to;
            p->brd[PC(WHITE,PAWN)] &= ~MASK(epsq);            
            s2p[epsq] = EMPTY;
            s2p[fromsq] = EMPTY;            
            s2p[tosq] = PC(BLACK,PAWN);            
        }
    } else if (flags == FLG_PROMO) {
        *pcs            &= ~from;
        p->brd[promopc] |= to;
        s2p[tosq]        = promopc;
        s2p[fromsq]      = EMPTY;
        if (topc != EMPTY) { // capture
            p->brd[topc] &= ~to;
            switch (tosq) {
            case A1: p->castle &= ~WQUEENSD; break;
            case H1: p->castle &= ~WKINGSD;  break;
            case A8: p->castle &= ~BQUEENSD; break;
            case H8: p->castle &= ~BKINGSD;  break;
            default: break;
            }
        }
    } else if (flags == FLG_CASTLE) {
        assert(pc == PC(side,KING));
        uint64_t *restrict rooks = &p->brd[PC(side,ROOK)];
        if (side == WHITE) {
            assert(fromsq == E1);
            assert(tosq == C1 || tosq == G1);
            if (tosq == G1) { // king side castle
                assert(s2p[E1] == PC(WHITE,KING));
                assert(s2p[F1] == EMPTY);
                assert(s2p[G1] == EMPTY);
                assert(s2p[H1] == PC(WHITE,ROOK));
                *pcs   &= ~MASK(E1);
                *pcs   |= MASK(G1);
                *rooks &= ~MASK(H1);
                *rooks |= MASK(F1);
                s2p[E1] = EMPTY;
                s2p[F1] = PC(WHITE,ROOK);
                s2p[G1] = PC(WHITE,KING);
                s2p[H1] = EMPTY;
            } else { // queen side castle
                assert(s2p[E1] == PC(WHITE,KING));
                assert(s2p[D1] == EMPTY);
                assert(s2p[C1] == EMPTY);
                assert(s2p[B1] == EMPTY);
                assert(s2p[A1] == PC(WHITE,ROOK));
                *pcs   &= ~MASK(E1);
                *pcs   |= MASK(C1);
                *rooks &= ~MASK(A1);
                *rooks |= MASK(D1);
                s2p[A1] = EMPTY;
                s2p[B1] = EMPTY;
                s2p[C1] = PC(WHITE,KING);
                s2p[D1] = PC(WHITE,ROOK);
                s2p[E1] = EMPTY;
            }
            p->castle &= ~(WQUEENSD | WKINGSD);
        } else {
            assert(fromsq == E8);
            assert(tosq == C8 || tosq == G8);
            if (tosq == G8) { // king side castle
                assert(s2p[E8] == PC(WHITE,KING));
                assert(s2p[F8] == EMPTY);
                assert(s2p[G8] == EMPTY);
                assert(s2p[H8] == PC(WHITE,ROOK));
                *pcs   &= ~MASK(E8);
                *pcs   |= MASK(G8);
                *rooks &= ~MASK(H8);
                *rooks |= MASK(F8);
                s2p[E8] = EMPTY;
                s2p[F8] = PC(BLACK,ROOK);
                s2p[G8] = PC(BLACK,KING);
                s2p[H8] = EMPTY;
            } else { // queen side castle
                assert(s2p[E8] == PC(BLACK,KING));
                assert(s2p[D8] == EMPTY);
                assert(s2p[C8] == EMPTY);
                assert(s2p[B8] == EMPTY);
                assert(s2p[A8] == PC(BLACK,ROOK));
                *pcs   &= ~MASK(E8);
                *pcs   |= MASK(C8);
                *rooks &= ~MASK(A8);
                *rooks |= MASK(D8);
                s2p[A8] = EMPTY;
                s2p[B8] = EMPTY;
                s2p[C8] = PC(BLACK,KING);
                s2p[D8] = PC(BLACK,ROOK);
                s2p[E8] = EMPTY;
            }
            p->castle &= ~(BQUEENSD | BKINGSD);
        }
    } else {
        assert(0);
    }

    // --- stores ---
    p->wtm = contra;
    p->halfmoves = pc == PC(side,PAWN) ? 0 : p->halfmoves + 1;
    ++p->nmoves;

    // --- validate position after ---
    assert(validate_position(p) == 0);
    assert(p->enpassant == NO_ENPASSANT || pc == PC(side,PAWN));
    
    // no pawns on 1st or 8th ranks
    assert(p->sqtopc[A1] != PC(WHITE,PAWN));
    assert(p->sqtopc[B1] != PC(WHITE,PAWN));
    assert(p->sqtopc[C1] != PC(WHITE,PAWN));
    assert(p->sqtopc[D1] != PC(WHITE,PAWN));
    assert(p->sqtopc[E1] != PC(WHITE,PAWN));
    assert(p->sqtopc[F1] != PC(WHITE,PAWN));
    assert(p->sqtopc[G1] != PC(WHITE,PAWN));
    assert(p->sqtopc[H1] != PC(WHITE,PAWN));
    assert(p->sqtopc[A1] != PC(BLACK,PAWN));
    assert(p->sqtopc[B1] != PC(BLACK,PAWN));
    assert(p->sqtopc[C1] != PC(BLACK,PAWN));
    assert(p->sqtopc[D1] != PC(BLACK,PAWN));
    assert(p->sqtopc[E1] != PC(BLACK,PAWN));
    assert(p->sqtopc[F1] != PC(BLACK,PAWN));
    assert(p->sqtopc[G1] != PC(BLACK,PAWN));
    assert(p->sqtopc[H1] != PC(BLACK,PAWN));
    assert(p->sqtopc[A8] != PC(WHITE,PAWN));
    assert(p->sqtopc[B8] != PC(WHITE,PAWN));
    assert(p->sqtopc[C8] != PC(WHITE,PAWN));
    assert(p->sqtopc[D8] != PC(WHITE,PAWN));
    assert(p->sqtopc[E8] != PC(WHITE,PAWN));
    assert(p->sqtopc[F8] != PC(WHITE,PAWN));
    assert(p->sqtopc[G8] != PC(WHITE,PAWN));
    assert(p->sqtopc[H8] != PC(WHITE,PAWN));
    assert(p->sqtopc[A8] != PC(BLACK,PAWN));
    assert(p->sqtopc[B8] != PC(BLACK,PAWN));
    assert(p->sqtopc[C8] != PC(BLACK,PAWN));
    assert(p->sqtopc[D8] != PC(BLACK,PAWN));
    assert(p->sqtopc[E8] != PC(BLACK,PAWN));
    assert(p->sqtopc[F8] != PC(BLACK,PAWN));
    assert(p->sqtopc[G8] != PC(BLACK,PAWN));
    assert(p->sqtopc[H8] != PC(BLACK,PAWN));
    
    // either "cant castle"              OR "king and rook are still on original squares"
    if (!(((p->castle & WQUEENSD) == 0) || (p->sqtopc[A1] == PC(WHITE,ROOK) && p->sqtopc[E1] == PC(WHITE,KING)))) {
        full_position_print(p);
        move_print(m);
    }
    assert(((p->castle & WQUEENSD) == 0) || (p->sqtopc[A1] == PC(WHITE,ROOK) && p->sqtopc[E1] == PC(WHITE,KING)));        
    assert(((p->castle & WKINGSD)  == 0) || (p->sqtopc[H1] == PC(WHITE,ROOK) && p->sqtopc[E1] == PC(WHITE,KING)));
    assert(((p->castle & BQUEENSD) == 0) || (p->sqtopc[A8] == PC(BLACK,ROOK) && p->sqtopc[E8] == PC(BLACK,KING)));
    assert(((p->castle & BKINGSD)  == 0) || (p->sqtopc[H8] == PC(BLACK,ROOK) && p->sqtopc[E8] == PC(BLACK,KING)));
}

void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp) {
    const uint8_t  side   = FLIP(p->wtm);    
    const uint32_t fromsq = FROM(m);
    const uint32_t tosq   = TO(m);
    const uint32_t promo  = PROMOPC[PROMO_PC(m)]; // only valid if promo flag set
    const uint32_t promopc = PC(side,promo);        
    const uint32_t flags  = FLAGS(m);
    const uint32_t pc     = p->sqtopc[tosq];
    const uint32_t cappc  = sp->captured_pc;
    const uint64_t from   = MASK(fromsq);
    const uint64_t to     = MASK(tosq);
    const uint32_t epsq   = sp->enpassant + 23;      // only valid if ep flag set    

    uint64_t *restrict pcs = &p->brd[pc];
    uint8_t  *restrict s2p = p->sqtopc;

    // --- validate position before ---
    assert(validate_position(p) == 0);
    assert(fromsq >= A1 && fromsq <= H8);
    assert(tosq   >= A1 && tosq   <= H8);
    assert(side == WHITE || side == BLACK);
    assert(flags >= FLG_NONE && flags <= FLG_CASTLE);
    assert(pc    >= PC(WHITE,PAWN) && pc    <= EMPTY);
    assert(cappc >= PC(WHITE,PAWN) && cappc <= EMPTY);
    assert(promo != FLG_PROMO || (promopc >= PC(side,KNIGHT) && promopc <= PC(side,QUEEN)));
    
    p->halfmoves = sp->halfmoves;
    p->enpassant = sp->enpassant;
    p->castle = sp->castle;
    p->wtm = side;
    --p->nmoves;

    // TODO(plesslie): make this a jump table? (or switch)
    if (flags == FLG_NONE) {
        s2p[fromsq] = pc;
        *pcs |= from;
        *pcs &= ~to;
        s2p[tosq] = cappc;
        if (cappc != EMPTY) {
            p->brd[cappc] |= MASK(tosq);
        }
    } else if (flags == FLG_EP) {
        s2p[fromsq] = pc;
        *pcs |= from;
        *pcs &= ~to;
        s2p[tosq] = EMPTY;
        if (side == WHITE) {
            assert(epsq == (tosq - 8));
            s2p[epsq] = PC(BLACK,PAWN);
            p->brd[PC(BLACK,PAWN)] |= MASK(epsq);
        } else {
            assert(epsq == (tosq + 8));            
            s2p[epsq] = PC(WHITE,PAWN);
            p->brd[PC(WHITE,PAWN)] |= MASK(epsq);            
        }
    } else if (flags == FLG_PROMO) {
        p->brd[PC(side,PAWN)] |= from;
        p->brd[promopc] &= ~to;
        s2p[tosq] = cappc;        
        s2p[fromsq] = PC(side,PAWN);
        if (cappc != EMPTY) {
            p->brd[cappc] |= to;
        }
    } else if (flags == FLG_CASTLE) {
        assert(pc == PC(side,KING));
        // TODO(plesslie): switch statement?
        if (tosq == C1) {
            assert(fromsq == E1);
            assert(s2p[A1] == EMPTY);
            assert(s2p[B1] == EMPTY);
            assert(s2p[C1] == PC(WHITE,KING));
            assert(s2p[D1] == PC(WHITE,ROOK));
            assert(s2p[E1] == EMPTY);
            s2p[A1] = PC(WHITE,ROOK);
            s2p[B1] = EMPTY;
            s2p[C1] = EMPTY;
            s2p[D1] = EMPTY;
            s2p[E1] = PC(WHITE,KING);
            p->brd[PC(WHITE,ROOK)] &= ~MASK(D1);
            p->brd[PC(WHITE,ROOK)] |= MASK(A1);
            p->brd[PC(WHITE,KING)] &= ~MASK(C1);
            p->brd[PC(WHITE,KING)] |= MASK(E1);
        } else if (tosq == G1) {
            assert(fromsq == E1);
            assert(s2p[E1] == EMPTY);
            assert(s2p[F1] == PC(WHITE,ROOK));
            assert(s2p[G1] == PC(WHITE,KING));
            assert(s2p[H1] == EMPTY);
            s2p[E1] = PC(WHITE,KING);
            s2p[F1] = EMPTY;
            s2p[G1] = EMPTY;
            s2p[H1] = PC(WHITE,ROOK);
            p->brd[PC(WHITE,KING)] &= ~MASK(G1);
            p->brd[PC(WHITE,KING)] |= MASK(E1);
            p->brd[PC(WHITE,ROOK)] &= ~MASK(F1);
            p->brd[PC(WHITE,ROOK)] |= MASK(H1);
        } else if (tosq == C8) {
            assert(fromsq == E8);
            assert(s2p[A8] == EMPTY);
            assert(s2p[B8] == EMPTY);
            assert(s2p[C8] == PC(BLACK,KING));
            assert(s2p[D8] == PC(BLACK,ROOK));
            assert(s2p[E8] == EMPTY);
            s2p[A8] = PC(BLACK,ROOK);
            s2p[B8] = EMPTY;
            s2p[C8] = EMPTY;
            s2p[D8] = EMPTY;
            s2p[E8] = PC(BLACK,KING);
            p->brd[PC(BLACK,ROOK)] &= ~MASK(D8);
            p->brd[PC(BLACK,ROOK)] |= MASK(A8);
            p->brd[PC(BLACK,KING)] &= ~MASK(C8);
            p->brd[PC(BLACK,KING)] |= MASK(E8);
        } else if (tosq == G8) {
            assert(fromsq == E8);
            assert(s2p[E8] == EMPTY);
            assert(s2p[F8] == PC(BLACK,ROOK));
            assert(s2p[G8] == PC(BLACK,KING));
            assert(s2p[H8] == EMPTY);
            s2p[E8] = PC(BLACK,KING);
            s2p[F8] = EMPTY;
            s2p[G8] = EMPTY;
            s2p[H8] = PC(BLACK,ROOK);
            p->brd[PC(BLACK,KING)] &= ~MASK(G8);
            p->brd[PC(BLACK,KING)] |= MASK(E8);
            p->brd[PC(BLACK,ROOK)] &= ~MASK(F8);
            p->brd[PC(BLACK,ROOK)] |= MASK(H8);            
        } else {
            assert(0);
        }
    } else {
        assert(0);
    }

    // --- validate position after ---
    assert(validate_position(p) == 0);
    
    // no pawns on 1st or 8th ranks
    assert(p->sqtopc[A1] != PC(WHITE,PAWN));
    assert(p->sqtopc[B1] != PC(WHITE,PAWN));
    assert(p->sqtopc[C1] != PC(WHITE,PAWN));
    assert(p->sqtopc[D1] != PC(WHITE,PAWN));
    assert(p->sqtopc[E1] != PC(WHITE,PAWN));
    assert(p->sqtopc[F1] != PC(WHITE,PAWN));
    assert(p->sqtopc[G1] != PC(WHITE,PAWN));
    assert(p->sqtopc[H1] != PC(WHITE,PAWN));
    assert(p->sqtopc[A1] != PC(BLACK,PAWN));
    assert(p->sqtopc[B1] != PC(BLACK,PAWN));
    assert(p->sqtopc[C1] != PC(BLACK,PAWN));
    assert(p->sqtopc[D1] != PC(BLACK,PAWN));
    assert(p->sqtopc[E1] != PC(BLACK,PAWN));
    assert(p->sqtopc[F1] != PC(BLACK,PAWN));
    assert(p->sqtopc[G1] != PC(BLACK,PAWN));
    assert(p->sqtopc[H1] != PC(BLACK,PAWN));
    assert(p->sqtopc[A8] != PC(WHITE,PAWN));
    assert(p->sqtopc[B8] != PC(WHITE,PAWN));
    assert(p->sqtopc[C8] != PC(WHITE,PAWN));
    assert(p->sqtopc[D8] != PC(WHITE,PAWN));
    assert(p->sqtopc[E8] != PC(WHITE,PAWN));
    assert(p->sqtopc[F8] != PC(WHITE,PAWN));
    assert(p->sqtopc[G8] != PC(WHITE,PAWN));
    assert(p->sqtopc[H8] != PC(WHITE,PAWN));
    assert(p->sqtopc[A8] != PC(BLACK,PAWN));
    assert(p->sqtopc[B8] != PC(BLACK,PAWN));
    assert(p->sqtopc[C8] != PC(BLACK,PAWN));
    assert(p->sqtopc[D8] != PC(BLACK,PAWN));
    assert(p->sqtopc[E8] != PC(BLACK,PAWN));
    assert(p->sqtopc[F8] != PC(BLACK,PAWN));
    assert(p->sqtopc[G8] != PC(BLACK,PAWN));
    assert(p->sqtopc[H8] != PC(BLACK,PAWN));
}




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

#if 0
uint32_t generate_moves(const struct position *const restrict pos, move *restrict moves) {
    return pos->wtm == WHITE ? gen_move_white(pos, moves) : gen_move_black(pos, moves);	
}
#endif

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

uint32_t gen_legal_moves(const struct position *const restrict pos, move *restrict moves) {
	struct savepos sp;
	uint32_t ret = 0;
	uint32_t i = 0;
	const uint8_t side = pos->wtm;
	move tmp[MAX_MOVES];
	const uint32_t nmoves = generate_moves(pos, &tmp[0]);
	struct position p;
	memcpy(&p, pos, sizeof(p));
	
	for (i = 0; i < nmoves; ++i) {
		make_move(&p, tmp[i], &sp);
		if (in_check(&p, side) == 0) {
			moves[ret++] = tmp[i];
		}
		undo_move(&p, tmp[i], &sp);
	}
	
	return ret;
}
