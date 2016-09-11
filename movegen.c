#include "movegen.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "read_fen.h"

#define EXTRA_INFO

void make_move(struct position * restrict p, move m, struct savepos * restrict sp) {
    uint32_t pc = PIECE(m);
    uint32_t fromsq = FROM(m);
    uint32_t tosq = TO(m);
    uint32_t capture = CAPTURE(m);
    uint32_t promotion = PROMOTE(m);
    uint8_t side = p->wtm;
    uint64_t from = MASK(fromsq);
    uint64_t to = MASK(tosq);
    uint64_t * restrict pcs = &p->brd[pc];
    
    assert(validate_position(p) == 0);
    
    sp->halfmoves = p->halfmoves;
    sp->enpassant = p->enpassant;
    sp->castle = p->castle;
    sp->was_ep = 0;

    p->wtm = FLIP(p->wtm);
    if (pc == PC(side,PAWN)) {
        p->halfmoves = 0;
    } else {
        ++p->halfmoves;
    }
    ++p->nmoves;

    // TODO: improve
    if (pc == PC(WHITE,KING) && fromsq == E1 && tosq == C1) {
        assert(p->sqtopc[B1] == EMPTY);
        assert(p->sqtopc[C1] == EMPTY);
        assert(p->sqtopc[D1] == EMPTY);
        p->castle &= ~(WQUEENSD|WKINGSD);
        p->brd[PC(WHITE,KING)] |= MASK(C1);
        p->brd[PC(WHITE,KING)] &= ~MASK(E1);
        p->brd[PC(WHITE,ROOK)] |= MASK(D1);
        p->brd[PC(WHITE,ROOK)] &= ~MASK(A1);
        p->sqtopc[E1] = EMPTY;
        p->sqtopc[A1] = EMPTY;
        p->sqtopc[C1] = PC(WHITE,KING);
        p->sqtopc[D1] = PC(WHITE,ROOK);
        p->enpassant = NO_ENPASSANT;
        goto end;
    } else if (pc == PC(WHITE,KING) && fromsq == E1 && tosq == G1) {
        assert(p->sqtopc[F1] == EMPTY);
        assert(p->sqtopc[G1] == EMPTY);
        p->castle &= ~(WQUEENSD|WKINGSD);
        p->brd[PC(WHITE,KING)] |= MASK(G1);
        p->brd[PC(WHITE,KING)] &= ~MASK(E1);
        p->brd[PC(WHITE,ROOK)] |= MASK(F1);
        p->brd[PC(WHITE,ROOK)] &= ~MASK(H1);
        p->sqtopc[E1] = EMPTY;
        p->sqtopc[H1] = EMPTY;
        p->sqtopc[G1] = PC(WHITE,KING);
        p->sqtopc[F1] = PC(WHITE,ROOK);
        p->enpassant = NO_ENPASSANT;
        goto end;
    } else if (pc == PC(BLACK,KING) && fromsq == E8 && tosq == C8) {
        assert(p->sqtopc[B8] == EMPTY);
        assert(p->sqtopc[C8] == EMPTY);
        assert(p->sqtopc[D8] == EMPTY);
        p->castle &= ~(BQUEENSD|BKINGSD);
        p->brd[PC(BLACK,KING)] |= MASK(C8);
        p->brd[PC(BLACK,KING)] &= ~MASK(E8);
        p->brd[PC(BLACK,ROOK)] |= MASK(D8);
        p->brd[PC(BLACK,ROOK)] &= ~MASK(A8);
        p->sqtopc[E8] = EMPTY;
        p->sqtopc[A8] = EMPTY;
        p->sqtopc[C8] = PC(BLACK,KING);
        p->sqtopc[D8] = PC(BLACK,ROOK);
        p->enpassant = NO_ENPASSANT;
        goto end;
    } else if (pc == PC(BLACK,KING) && fromsq == E8 && tosq == G8) {
        assert(p->sqtopc[F8] == EMPTY);
        assert(p->sqtopc[G8] == EMPTY);
        p->castle &= ~(BQUEENSD|BKINGSD);
        p->brd[PC(BLACK,KING)] |= MASK(G8);
        p->brd[PC(BLACK,KING)] &= ~MASK(E8);
        p->brd[PC(BLACK,ROOK)] |= MASK(F8);
        p->brd[PC(BLACK,ROOK)] &= ~MASK(H8);
        p->sqtopc[E8] = EMPTY;
        p->sqtopc[H8] = EMPTY;
        p->sqtopc[G8] = PC(BLACK,KING);
        p->sqtopc[F8] = PC(BLACK,ROOK);
        p->enpassant = NO_ENPASSANT;
        goto end;
    }

    *pcs &= ~from;
    if (promotion == NO_PROMOTION) {
        *pcs |= to;
    } else {
        pcs = &PIECES(*p, side, promotion);
        *pcs |= to;
    }
    
    if (capture != NO_CAPTURE) {
        if (pc == PC(side, PAWN) && p->sqtopc[tosq] == EMPTY) { // e.p.
            sp->was_ep = 1;
            if (side == WHITE) {
                assert(tosq >= A6 && tosq <= H6);
                int sq = tosq - 8;
                assert(p->enpassant != NO_ENPASSANT);
                assert(sq == 23 + p->enpassant);
                assert(p->sqtopc[sq] == PC(BLACK,PAWN));
                assert(p->sqtopc[tosq] == EMPTY);
                p->sqtopc[sq] = EMPTY;
                p->brd[PC(BLACK,PAWN)] &= ~MASK(sq);
            } else {
                assert(tosq >= A3 && tosq <= H3);
                int sq = tosq + 8;
                assert(p->enpassant != NO_ENPASSANT);
                assert(sq == 23 + p->enpassant);
                assert(p->sqtopc[sq] == PC(WHITE,PAWN));
                assert(p->sqtopc[tosq] == EMPTY);                
                p->sqtopc[sq] = EMPTY;
                p->brd[PC(WHITE,PAWN)] &= ~MASK(sq);
            }
        } else {
            assert(p->sqtopc[tosq] != EMPTY);
            assert((p->brd[p->sqtopc[tosq]] & to) != 0);
            p->brd[p->sqtopc[tosq]] &= ~to;
        }
    }
    p->sqtopc[fromsq] = EMPTY;
    // TODO: should be able to avoid checking this twice
    if (promotion == NO_PROMOTION) {
        p->sqtopc[tosq] = pc;
    } else {
        assert(promotion != PAWN);
        assert(promotion != KING);
        p->sqtopc[tosq] = PC(side, promotion);
    }

    // REVISIT: improve
    // update castling flags if needed
    if (p->castle != 0) {
        // detect rook moving
        if (pc == PC(WHITE,ROOK) && fromsq == A1) {
            p->castle &= ~WQUEENSD;
        } else if (pc == PC(WHITE,ROOK) && fromsq == H1) {
            p->castle &= ~WKINGSD;
        } else if (pc == PC(BLACK,ROOK) && fromsq == A8) {
            p->castle &= ~BQUEENSD;
        } else if (pc == PC(BLACK,ROOK) && fromsq == H8) {
            p->castle &= ~BKINGSD;
        }
        // detect king moving
        else if (pc == PC(WHITE,KING)) {
            p->castle &= ~(WQUEENSD | WKINGSD);
        } else if (pc == PC(BLACK,KING)) {
            p->castle &= ~(BQUEENSD | BKINGSD);
        }
        // TODO: check if faster to always remove even if bits not present
        // i.e. check if ((p->castle & BIT) != 0) is true as well.
        // detect capturing on rook squares
        if (p->sqtopc[A1] != PC(WHITE,ROOK)) {
            p->castle &= ~WQUEENSD;
        }
        if (p->sqtopc[H1] != PC(WHITE,ROOK)) {
            p->castle &= ~WKINGSD;
        }
        if (p->sqtopc[A8] != PC(BLACK,ROOK)) {
            p->castle &= ~BQUEENSD;
        }
        if (p->sqtopc[H8] != PC(BLACK,ROOK)) {
            p->castle &= ~BKINGSD;
        }
    }

    p->enpassant = NO_ENPASSANT;
    if (capture == NO_CAPTURE) {
        if (pc == PC(WHITE,PAWN) && (to & WHITE_ENPASSANT_SQUARES) != 0 && (from & RANK2(side)) != 0) {
            p->enpassant = tosq - 23;
            assert(p->enpassant >= 1 && p->enpassant <= 8);
        } else if (pc == PC(BLACK,PAWN) && (to & BLACK_ENPASSANT_SQUARES) != 0 && (from & RANK2(side)) != 0) {
            p->enpassant = tosq - 23;
            assert(p->enpassant >= 9 && p->enpassant <= 16);            
        }
    }
    
 end: // verification tests should always be run
    assert(validate_position(p) == 0);

    // either en passant, or the last move was a pawn move
    assert((p->enpassant == NO_ENPASSANT) || ((pc == PC(WHITE,PAWN) || pc == PC(BLACK,PAWN)) && capture == NO_CAPTURE));
    
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
    assert(((p->castle & WQUEENSD) == 0) || (p->sqtopc[A1] == PC(WHITE,ROOK) && p->sqtopc[E1] == PC(WHITE,KING)));        
    assert(((p->castle & WKINGSD)  == 0) || (p->sqtopc[H1] == PC(WHITE,ROOK) && p->sqtopc[E1] == PC(WHITE,KING)));
    assert(((p->castle & BQUEENSD) == 0) || (p->sqtopc[A8] == PC(BLACK,ROOK) && p->sqtopc[E8] == PC(BLACK,KING)));
    assert(((p->castle & BKINGSD)  == 0) || (p->sqtopc[H8] == PC(BLACK,ROOK) && p->sqtopc[E8] == PC(BLACK,KING)));
}

void make_move_ex(struct position *restrict p, smove_t m, struct saveposex *restrict sp) {
    // --- loads ---
    const uint32_t fromsq = SM_FROM(m);
    const uint32_t tosq   = SM_TO(m);
    const uint32_t promo  = PROMOPC[SM_PROMO_PC(m)]; // only valid if promo flag set
    const uint32_t flags  = SM_FLAGS(m);
    const uint8_t  side   = p->wtm;
    const uint8_t  contra = FLIP(side);
    const uint32_t pc     = p->sqtopc[fromsq];
    const uint32_t topc   = p->sqtopc[tosq];
    const uint64_t from   = MASK(fromsq);
    const uint64_t to     = MASK(tosq);
    const uint32_t epsq   = p->enpassant + 23;       // only valid if ep flag set

    #ifdef EXTRA_INFO
    printf("make_move_ex: fromsq(%s) tosq(%s) promo(%c) flags(%u) side(%s) "
           "contra(%s) pc(%c) topc(%c) from(0x%08" PRIX64 ") to(0x%08" PRIX64 ") "
           "epsq(%s)\n",
           sq_to_str[fromsq], sq_to_str[tosq], flags==SM_PROMO?vpcs[promo]:' ', flags, side==WHITE?"WHITE":"BLACK",
           contra==WHITE?"WHITE":"BLACK", vpcs[pc], vpcs[topc], from, to, sq_to_str[epsq]);
    #endif
    
    uint64_t *restrict pcs = &p->brd[pc];
    uint8_t  *restrict s2p = p->sqtopc;

    // --- validate position before ---
    assert(validate_position(p) == 0);

    // --- update saveposex ---
    sp->halfmoves = p->halfmoves;
    sp->enpassant = p->enpassant;
    sp->castle    = p->castle;
    sp->was_ep    = SM_FALSE;

    // TODO(plesslie): make this a jump table? (or switch)
    if (flags == SM_NONE) { // normal case
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
            if (fromsq == A1) {
                p->castle &= ~WQUEENSD;
            } else if (fromsq == H1) {
                p->castle &= ~WKINGSD;
            } else if (fromsq == A8) {
                p->castle &= ~BQUEENSD;
            } else if (fromsq == H8) {
                p->castle &= ~BKINGSD;
            }
        }
    } else if (flags == SM_EP) {
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
        p->enpassant = NO_ENPASSANT;
    } else if (flags == SM_PROMO) {
        const uint32_t promopc = PC(side,promo);
        *pcs            &= ~from;
        p->brd[promopc] |= to;
        s2p[tosq]        = promopc;        
        s2p[fromsq]      = EMPTY;
        if (topc != EMPTY) { // capture
            p->brd[topc] &= ~to;
        }
        p->enpassant = NO_ENPASSANT;
    } else if (flags == SM_CASTLE) {
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
        p->enpassant = NO_ENPASSANT;
    } else {
        assert(0);
    }

    // --- stores ---
    p->wtm = contra;
    p->halfmoves = pc == PC(side,PAWN) ? 0 : p->halfmoves + 1;
    ++p->nmoves;

    // --- validate position after ---
    assert(validate_position(p) == 0);
    assert((p->enpassant == NO_ENPASSANT) || (pc == PC(WHITE,PAWN) || pc == PC(BLACK,PAWN)));
    
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
    assert(((p->castle & WQUEENSD) == 0) || (p->sqtopc[A1] == PC(WHITE,ROOK) && p->sqtopc[E1] == PC(WHITE,KING)));        
    assert(((p->castle & WKINGSD)  == 0) || (p->sqtopc[H1] == PC(WHITE,ROOK) && p->sqtopc[E1] == PC(WHITE,KING)));
    assert(((p->castle & BQUEENSD) == 0) || (p->sqtopc[A8] == PC(BLACK,ROOK) && p->sqtopc[E8] == PC(BLACK,KING)));
    assert(((p->castle & BKINGSD)  == 0) || (p->sqtopc[H8] == PC(BLACK,ROOK) && p->sqtopc[E8] == PC(BLACK,KING)));
}

static int test_move_creation() {
    // verify that move creation macro works correctly
    
    int i;
    int ret;
    struct test_t {
        uint32_t from;
        uint32_t to;
        uint32_t prm;        
        uint32_t ep;
        uint32_t csl;
    } tests[] = {
        { E2, E4, SM_FALSE     , SM_FALSE, SM_FALSE }, // regular case
        { E6, D5, SM_FALSE     , SM_TRUE , SM_FALSE }, // ep case
        { E8, E7, SM_PRM_QUEEN , SM_FALSE, SM_FALSE }, // prm case - white queen
        { E8, E7, SM_PRM_ROOK  , SM_FALSE, SM_FALSE }, // prm case - white rook
        { E8, E7, SM_PRM_BISHOP, SM_FALSE, SM_FALSE }, // prm case - white bishop
        { E8, E7, SM_PRM_KNIGHT, SM_FALSE, SM_FALSE }, // prm case - white knight
        { E1, E2, SM_PRM_KNIGHT, SM_FALSE, SM_FALSE }, // prm case - black knight
        { G1, E1, SM_FALSE     , SM_FALSE, SM_TRUE  },
        { A1, H8, SM_FALSE     , SM_FALSE, SM_FALSE },
        { A1, B1, SM_FALSE     , SM_FALSE, SM_FALSE }
    };

    printf("Testing move creation...\n");
    for (i = 0; i < (sizeof(tests)/sizeof(tests[0])); ++i) {
        const smove_t mv = SMALLMOVE(tests[i].from,
                                     tests[i].to,
                                     tests[i].prm,
                                     tests[i].ep,
                                     tests[i].csl);
        const uint32_t to    = SM_TO(mv);
        const uint32_t from  = SM_FROM(mv);
        const uint32_t prm   = SM_PROMO_PC(mv);
        const uint32_t flags = SM_FLAGS(mv);

        if (to != tests[i].to) {
            printf("to(%u) != tests[i].to(%u)\n", to, tests[i].to);
            ret = 1;
        } else if (from != tests[i].from) {
            printf("from(%u) != tests[i].from(%u)\n", from, tests[i].from);
            ret = 1;
        } else if (tests[i].prm != SM_FALSE && flags != SM_PROMO) {
            printf("prm != FALSE && flags(%u) != SM_PROMO\n", flags);
            ret = 1;
        } else if (tests[i].prm != SM_FALSE && prm != tests[i].prm) {
            printf("prm(%u) != tests[i].prm(%u)\n", prm, tests[i].prm);
            ret = 1;
        } else if (tests[i].ep == SM_TRUE && flags != SM_EP) {
            printf("ep != FALSE && flags(%u) != SM_EP\n", flags);
            ret = 1;
        } else if (tests[i].csl == SM_TRUE && flags != SM_CASTLE) {
            printf("csl != FALSE && flags(%u) != SM_CASTLE\n", flags);
            ret = 1;
        } else {
            ret = 0;
        }

        if (ret != 0) {
            printf("Failed on test case: (frm=%s, to=%s, prm=%u, ep=%u, csl=%u) sm = 0x%04" PRIx32 "\n",
                   sq_to_str[tests[i].from], sq_to_str[tests[i].to], tests[i].prm,
                   tests[i].ep, tests[i].csl, mv);
            pbin(mv);
            return ret;
        }
                
    }
    printf("Success.\n");
    return 0;
}

int test_make_move_ex(const char *fen, const smove_t *moves) {
    struct position pos;
    struct position tmp;
    struct saveposex sp;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }
    memcpy(&tmp, &pos, sizeof(tmp));

    const smove_t *m = moves;
    while (*m) {
        #ifdef EXTRA_INFO
        printf("Testing: "); smove_print(*m);
        #endif
        
        make_move_ex(&pos, *m, &sp);
        if (validate_position(&pos) != 0) {
            fputs("Failed to make move!\n", stderr);
            return 1;
        }
        // restore pos
        memcpy(&pos, &tmp, sizeof(tmp));
        ++m;
    }

    return 0;
}

void test_make_move() {
    if (test_move_creation() != 0) {
        return;
    }

    printf("Running make move tests...\n");
    const char *start_pos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";    
    smove_t start_pos_moves[] = {
        SMALLMOVE(A2, A3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(A2, A4, SM_FALSE, SM_FALSE, SM_FALSE),        
        SMALLMOVE(B2, B3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(B2, B4, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(C2, C3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(C2, C4, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(D2, D3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(D2, D4, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(E2, E3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(E2, E4, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(F2, F3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(F2, F4, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(G2, G3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(G2, G4, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(H2, H3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(H2, H4, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(B1, A3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(B1, C3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(G1, F3, SM_FALSE, SM_FALSE, SM_FALSE),
        SMALLMOVE(G1, H3, SM_FALSE, SM_FALSE, SM_FALSE),
        0
    };
    if (test_make_move_ex(start_pos_fen, &start_pos_moves[0]) != 0) {
        printf("Failed test for moves from starting position!\n");
        return;
    }

    const char *kiwi_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    smove_t kiwi_moves[] = {
        SMALLMOVE(E2, A6, SM_FALSE, SM_FALSE, SM_FALSE), // bishop captures bishop
        SMALLMOVE(G2, H3, SM_FALSE, SM_FALSE, SM_FALSE), // pawn captures pawn
        SMALLMOVE(D2, G5, SM_FALSE, SM_FALSE, SM_FALSE), // bishop move
        SMALLMOVE(E1, G1, SM_FALSE, SM_FALSE, SM_TRUE ), // castle king side
        SMALLMOVE(E1, C1, SM_FALSE, SM_FALSE, SM_TRUE ), // castle queen side
        SMALLMOVE(F3, H3, SM_FALSE, SM_FALSE, SM_FALSE), // queen captures pawn
        SMALLMOVE(H1, F1, SM_FALSE, SM_FALSE, SM_FALSE), // rook move
        SMALLMOVE(E1, F1, SM_FALSE, SM_FALSE, SM_FALSE), // king move (not castling)
        0
    };
    if (test_make_move_ex(kiwi_fen, &kiwi_moves[0]) != 0) {
        printf("Failed test for moves from kiwi position!\n");
        return;
    }

    const char *ep_fen = "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6";
    smove_t ep_moves[] = {
        SMALLMOVE(E5, D6, SM_FALSE, SM_TRUE, SM_FALSE),
        0
    };
    if (test_make_move_ex(ep_fen, &ep_moves[0]) != 0) {
        printf("Failed test for moves from e.p. position!\n");
        return;
    }

    const char *promo_fen = "8/1Pp5/7r/8/KR1p1p1k/8/4P1P1/8 w - -";
    smove_t promo_moves[] = {
        SMALLMOVE(B7, B8, SM_PRM_KNIGHT, SM_FALSE, SM_FALSE), // knight promo
        SMALLMOVE(B7, B8, SM_PRM_BISHOP, SM_FALSE, SM_FALSE), // bishop promo
        SMALLMOVE(B7, B8, SM_PRM_ROOK  , SM_FALSE, SM_FALSE), // rook promo
        SMALLMOVE(B7, B8, SM_PRM_QUEEN , SM_FALSE, SM_FALSE), // queen promo
        0
    };
    if (test_make_move_ex(promo_fen, &promo_moves[0]) != 0) {
        printf("Failed test for moves from e.p. position!\n");
        return;
    }
    
    printf("Succeeded.\n");
}

void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp) {
    uint32_t pc = PIECE(m);
    uint32_t fromsq = FROM(m);
    uint32_t tosq = TO(m);
    uint32_t capture = CAPTURE(m);
    uint32_t promotion = PROMOTE(m);
    uint8_t  side = FLIP(p->wtm);
    uint64_t from = MASK(fromsq);
    uint64_t to = MASK(tosq);
    uint64_t *restrict pcs = &p->brd[pc];

    p->halfmoves = sp->halfmoves;
    p->enpassant = sp->enpassant;
    p->castle = sp->castle;
    p->wtm = side;
    --p->nmoves;

    if (pc == PC(WHITE,KING) && fromsq == E1 && tosq == C1) {
        assert(p->sqtopc[A1] == EMPTY);
        assert(p->sqtopc[B1] == EMPTY);
        assert(p->sqtopc[C1] == PC(WHITE,KING));
        assert(p->sqtopc[D1] == PC(WHITE,ROOK));
        assert(p->sqtopc[E1] == EMPTY);
        p->sqtopc[A1] = PC(WHITE,ROOK);
        p->sqtopc[B1] = EMPTY;
        p->sqtopc[C1] = EMPTY;
        p->sqtopc[D1] = EMPTY;
        p->sqtopc[E1] = PC(WHITE,KING);
        p->brd[PC(WHITE,ROOK)] &= ~MASK(D1);
        p->brd[PC(WHITE,ROOK)] |= MASK(A1);
        p->brd[PC(WHITE,KING)] &= ~MASK(C1);
        p->brd[PC(WHITE,KING)] |= MASK(E1);
        return;
    } else if (pc == PC(WHITE,KING) && fromsq == E1 && tosq == G1) {
        assert(p->sqtopc[E1] == EMPTY);
        assert(p->sqtopc[F1] == PC(WHITE,ROOK));
        assert(p->sqtopc[G1] == PC(WHITE,KING));
        assert(p->sqtopc[H1] == EMPTY);
        p->sqtopc[E1] = PC(WHITE,KING);
        p->sqtopc[F1] = EMPTY;
        p->sqtopc[G1] = EMPTY;
        p->sqtopc[H1] = PC(WHITE,ROOK);
        p->brd[PC(WHITE,KING)] &= ~MASK(G1);
        p->brd[PC(WHITE,KING)] |= MASK(E1);
        p->brd[PC(WHITE,ROOK)] &= ~MASK(F1);
        p->brd[PC(WHITE,ROOK)] |= MASK(H1);
        return;
    } else if (pc == PC(BLACK,KING) && fromsq == E8 && tosq == C8) {
        assert(p->sqtopc[A8] == EMPTY);
        assert(p->sqtopc[B8] == EMPTY);
        assert(p->sqtopc[C8] == PC(BLACK,KING));
        assert(p->sqtopc[D8] == PC(BLACK,ROOK));
        assert(p->sqtopc[E8] == EMPTY);
        p->sqtopc[A8] = PC(BLACK,ROOK);
        p->sqtopc[B8] = EMPTY;
        p->sqtopc[C8] = EMPTY;
        p->sqtopc[D8] = EMPTY;
        p->sqtopc[E8] = PC(BLACK,KING);
        p->brd[PC(BLACK,ROOK)] &= ~MASK(D8);
        p->brd[PC(BLACK,ROOK)] |= MASK(A8);
        p->brd[PC(BLACK,KING)] &= ~MASK(C8);
        p->brd[PC(BLACK,KING)] |= MASK(E8);
        return;
    } else if (pc == PC(BLACK,KING) && fromsq == E8 && tosq == G8) {
        assert(p->sqtopc[E8] == EMPTY);
        assert(p->sqtopc[F8] == PC(BLACK,ROOK));
        assert(p->sqtopc[G8] == PC(BLACK,KING));
        assert(p->sqtopc[H8] == EMPTY);
        p->sqtopc[E8] = PC(BLACK,KING);
        p->sqtopc[F8] = EMPTY;
        p->sqtopc[G8] = EMPTY;
        p->sqtopc[H8] = PC(BLACK,ROOK);
        p->brd[PC(BLACK,KING)] &= ~MASK(G8);
        p->brd[PC(BLACK,KING)] |= MASK(E8);
        p->brd[PC(BLACK,ROOK)] &= ~MASK(F8);
        p->brd[PC(BLACK,ROOK)] |= MASK(H8);
        return;
    }

    p->sqtopc[fromsq] = pc;
    *pcs |= from;
    *pcs &= ~to;
    if (sp->was_ep != 0) { // e.p.
        p->sqtopc[tosq] = EMPTY;
        if (side == WHITE) {
            assert(capture == PC(BLACK,PAWN));
            int sq = tosq - 8;
            p->sqtopc[sq] = PC(BLACK,PAWN);
            p->brd[PC(BLACK,PAWN)] |= MASK(sq);
        } else {
            assert(capture == PC(WHITE,PAWN));
            int sq = tosq + 8;
            p->sqtopc[sq] = PC(WHITE,PAWN);
            p->brd[PC(WHITE,PAWN)] |= MASK(sq);
        }
    } else {
        p->sqtopc[tosq] = capture;
        // if a capture, place captured piece back on bit boards
        if (capture != NO_CAPTURE) {
            p->brd[capture] |= to;
        }
        // if a promotion, remove placed piece from bit boards
        if (promotion != NO_PROMOTION) {
            p->brd[PC(side,promotion)] &= ~to;
        }
    }
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

uint32_t generate_moves(const struct position * const restrict pos, move * restrict moves) {
    uint32_t i;
    uint32_t pc;
    uint64_t pcs;
    uint64_t msk;
    uint64_t posmoves;
    uint64_t sq;
    uint32_t nmove = 0;
    uint8_t side = pos->wtm;
    uint8_t contraside = FLIP(pos->wtm);
    uint64_t same = FULLSIDE(*pos, side);
    uint64_t contra = FULLSIDE(*pos, FLIP(side));
    uint64_t occupied = same | contra;
    uint64_t opp_or_empty = ~same;
    uint8_t castle = pos->castle;

    // knight moves
    pcs = PIECES(*pos, side, KNIGHT);
    if (pcs != 0) {
        pc = PC(side, KNIGHT);
        for (i = 0; i < 64 && pcs; ++i, pcs >>= 1) {
            if ((pcs & 0x01) != 0) {
                posmoves = knight_attacks[i] & opp_or_empty;
                for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
                    msk = MASK(sq);
                    if ((posmoves & 0x01) != 0 && (msk & same) == 0) {
                        moves[nmove++] = MOVE(sq, i, pc, pos->sqtopc[sq], 0, 0);
                    }
                }
            }
        }
    }

    // king moves
    pcs = PIECES(*pos, side, KING);
    assert(pcs != 0);
    pc = PC(side, KING);
    for (i = 0; i < 64 && (pcs & 0x01) == 0; ++i, pcs >>= 1);
    assert(i < 64 && i >= 0);
    assert(pos->sqtopc[i] == pc);
    posmoves = king_attacks[i] & opp_or_empty;
    for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
        msk = MASK(sq);
        if ((posmoves & 0x1) != 0 && (msk & same) == 0) {
            moves[nmove++] = MOVE(sq, i, pc, pos->sqtopc[sq], 0, 0);
        }
    }

    // castling
    if (side == WHITE) {
        if ((castle & WKINGSD) != 0    &&
            (i == E1)                  &&
            (pos->sqtopc[F1] == EMPTY) &&
            (pos->sqtopc[G1] == EMPTY) &&
            (attacks(pos, contraside, E1) == 0) &&
            (attacks(pos, contraside, F1) == 0) &&
            (attacks(pos, contraside, G1) == 0)) {
            assert(pos->sqtopc[H1] == PC(WHITE,ROOK));
            moves[nmove++] = MOVE(G1, E1, PC(WHITE,KING), EMPTY, 0, 0);
        }
        if ((castle & WQUEENSD) != 0   &&
            (i == E1)                  &&
            (pos->sqtopc[D1] == EMPTY) &&
            (pos->sqtopc[C1] == EMPTY) &&
            (pos->sqtopc[B1] == EMPTY) &&
            (attacks(pos, contraside, E1) == 0) &&
            (attacks(pos, contraside, D1) == 0) &&
            (attacks(pos, contraside, C1) == 0)) {
            assert(pos->sqtopc[A1] == PC(WHITE,ROOK));
            moves[nmove++] = MOVE(C1, E1, PC(WHITE,KING), EMPTY, 0, 0);
        }
    } else {
        if ((castle & BKINGSD) != 0    &&
            (i == E8)                  &&
            (pos->sqtopc[F8] == EMPTY) &&
            (pos->sqtopc[G8] == EMPTY) &&
            (attacks(pos, contraside, E8) == 0) &&
            (attacks(pos, contraside, F8) == 0) &&
            (attacks(pos, contraside, G8) == 0)) {
            assert(pos->sqtopc[H8] == PC(BLACK,ROOK));
            moves[nmove++] = MOVE(G8, E8, PC(BLACK,KING), EMPTY, 0, 0);
        }
        if ((castle & BQUEENSD) != 0   &&
            (i == E8)                  &&
            (pos->sqtopc[D8] == EMPTY) &&
            (pos->sqtopc[C8] == EMPTY) &&
            (pos->sqtopc[B8] == EMPTY) &&
            (attacks(pos, contraside, E8) == 0) &&
            (attacks(pos, contraside, D8) == 0) &&
            (attacks(pos, contraside, C8) == 0)) {
            assert(pos->sqtopc[A8] == PC(BLACK,ROOK));
            moves[nmove++] = MOVE(C8, E8, PC(BLACK,KING), EMPTY, 0, 0);
        }
    }

    // bishop moves
    pcs = PIECES(*pos, side, BISHOP);
    if (pcs != 0) {
        pc = PC(side, BISHOP);
        for (i = 0; i < 64 && pcs; ++i, pcs >>= 1) {
            if ((pcs & 0x01) != 0) {
                posmoves = bishop_attacks(i, occupied);
                for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
                    msk = MASK(sq);
                    if ((posmoves & 0x01) != 0 && (msk & same) == 0) {
                        moves[nmove++] = MOVE(sq, i, pc, pos->sqtopc[sq], 0, 0);
                    }
                }
            }
        }
    }

    // rook moves
    pcs = PIECES(*pos, side, ROOK);
    if (pcs != 0) {
        pc = PC(side, ROOK);
        for (i = 0; i < 64 && pcs; ++i, pcs >>= 1) {
            if ((pcs & 0x01) != 0) {
                posmoves = rook_attacks(i, occupied);
                for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
                    msk = MASK(sq);
                    if ((posmoves & 0x01) != 0 && (msk & same) == 0) {
                        moves[nmove++] = MOVE(sq, i, pc, pos->sqtopc[sq], 0, 0);
                    }
                }
            }
        }
    }

    // queen moves
    pcs = PIECES(*pos, side, QUEEN);
    if (pcs != 0) {
        pc = PC(side, QUEEN);
        for (i = 0; i < 64 && pcs; ++i, pcs >>= 1) {
            if ((pcs & 0x01) != 0) {
                posmoves = queen_attacks(i, occupied);
                for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
                    msk = MASK(sq);
                    if ((posmoves & 0x01) != 0 && (msk & same) == 0) {
                        moves[nmove++] = MOVE(sq, i, pc, pos->sqtopc[sq], 0, 0);
                    }
                }
            }
        }
    }

    // pawn moves
    uint32_t fromsq;
    pc = PC(side, PAWN);
    pcs = PIECES(*pos, side, PAWN);

    // forward 1 square
    posmoves = side == WHITE ? pcs << 8 : pcs >> 8;
    posmoves &= ~occupied;
    for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
        if ((posmoves & 0x01) != 0) {
            fromsq = side == WHITE ? sq - 8 : sq + 8;
            assert(pos->sqtopc[fromsq] == PC(side,PAWN));
            if (sq >= A8 || sq <= H1) { // promotion
                moves[nmove++] = MOVE(sq, fromsq, pc, NO_CAPTURE, KNIGHT, NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, NO_CAPTURE, BISHOP, NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, NO_CAPTURE, ROOK  , NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, NO_CAPTURE, QUEEN , NO_ENPASSANT);
            } else {
                moves[nmove++] = MOVE(sq, fromsq, pc, NO_CAPTURE, 0, 0);
            }
        }
    }

    // forward 2 squares
    posmoves = pcs & RANK2(side);
    posmoves = side == WHITE ? posmoves << 16 : posmoves >> 16;
    posmoves &= ~occupied;
    for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
        if ((posmoves & 0x01) != 0) {
            fromsq = side == WHITE ? sq - 16 : sq + 16;
            assert(pos->sqtopc[fromsq] == PC(side,PAWN));            
            // TODO: do this with the bitmasks?
            if (pos->sqtopc[side == WHITE ? sq - 8 : sq + 8] != EMPTY) {
                continue;
            }
            moves[nmove++] = MOVE(sq, fromsq, pc, EMPTY, 0, 0);
        }
    }

    // capture left
    posmoves = pcs & ~A_FILE;
    posmoves = side == WHITE ? posmoves << 7 : posmoves >> 9;
    posmoves &= contra;
    for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
        if ((posmoves & 0x01) != 0) {
            fromsq = side == WHITE ? sq - 7 : sq + 9;
            assert(pos->sqtopc[fromsq] == PC(side,PAWN));
            assert(pos->sqtopc[sq] != EMPTY);
            if (sq >= A8 || sq <= H1) { // last rank => promotion
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], KNIGHT, NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], BISHOP, NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], ROOK  , NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], QUEEN , NO_ENPASSANT);
            } else {
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], 0, 0);
            }
        }
    }

    // capture right
    posmoves = pcs & ~H_FILE;
    posmoves = side == WHITE ? posmoves << 9 : posmoves >> 7;
    posmoves &= contra;
    for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
        if ((posmoves & 0x01) != 0) {
            fromsq = side == WHITE ? sq - 9 : sq + 7;
            assert(pos->sqtopc[fromsq] == PC(side,PAWN));
            assert(pos->sqtopc[sq] != EMPTY);            
            if (sq >= A8 || sq <= H1) { // last rank => promotion
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], KNIGHT, NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], BISHOP, NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], ROOK  , NO_ENPASSANT);
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], QUEEN , NO_ENPASSANT);
            } else {
                moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], 0, 0);
            }
        }
    }

    // en passant
    if (pos->enpassant != NO_ENPASSANT) {
        uint32_t epsq = pos->enpassant + 23;
        if (epsq != 24 && epsq != 32) {
            // try capture left
            fromsq = epsq - 1;
            // TODO: shouldn't need to check that the ep square is occupied by a pawn on the other side
            if (pos->sqtopc[fromsq] == PC(side,PAWN)) {                
                sq = side == WHITE ? fromsq + 9 : fromsq - 7;
                if (pos->sqtopc[sq] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert((side == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           (side == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert((side == WHITE && (fromsq >= A5 && fromsq <= H5)) ||
                           (side == BLACK && (fromsq >= A4 && fromsq <= H4)));
                    assert((side == WHITE && (sq >= A6 && sq <= H6)) ||
                           (side == BLACK && (sq >= A3 && sq <= H3)));
                    assert(pos->sqtopc[fromsq] == PC(side,PAWN));
                    assert(pos->sqtopc[epsq] == PC(contraside,PAWN));
                    assert(pos->sqtopc[sq] == EMPTY);
                    moves[nmove++] = MOVE(sq, fromsq, pc, PC(contraside,PAWN), 0, 1);
                }
            }
        }
        if (epsq != 31 && epsq != 39) {
            // try capture right
            fromsq = epsq + 1;
            // TODO: shouldn't need to check that the ep square is occupied by a pawn on the other side            
            if (pos->sqtopc[fromsq] == PC(side,PAWN)) {                
                sq = side == WHITE ? fromsq + 7 : fromsq - 9;                
                if (pos->sqtopc[sq] == EMPTY) {
                    assert(pos->enpassant != NO_ENPASSANT);
                    assert((side == WHITE && (epsq >= A5 && epsq <= H5)) ||
                           (side == BLACK && (epsq >= A4 && epsq <= H4)));
                    assert((side == WHITE && (fromsq >= A5 && fromsq <= H5)) ||
                           (side == BLACK && (fromsq >= A4 && fromsq <= H4)));
                    assert((side == WHITE && (sq >= A6 && sq <= H6)) ||
                           (side == BLACK && (sq >= A3 && sq <= H3)));
                    assert(pos->sqtopc[fromsq] == PC(side,PAWN));
                    assert(pos->sqtopc[epsq] == PC(contraside,PAWN));
                    assert(pos->sqtopc[sq] == EMPTY);                    
                    moves[nmove++] = MOVE(sq, fromsq, pc, PC(contraside,PAWN), 0, 1);
                }
            }
        }
    }

    return nmove;
}
