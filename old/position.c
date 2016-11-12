#include "position.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

int read_fen(struct position * restrict pos, const char * const fen, int print) {
    int row;
    int col;
    const char *p = fen;
    int color;
    char c;

    // init position
    pos->nmoves = 0;
    pos->wtm = WHITE;
    pos->halfmoves = 0;
    pos->castle = 0;
    pos->enpassant = 0;
    for (row = 0; row < 64; ++row) {
        pos->sqtopc[row] = EMPTY;
    }
    memset(&pos->brd[0], 0, sizeof(pos->brd[0]) * NPIECES*2);

    for (row = 7; row >= 0; --row) {
        col = 0;
        do {
            c = *p++;
            if (c == 0) { // end-of-input
                return 1;
            } else if (c >= '1' && c <= '9') {
                col += c - '0';
            } else if (c != '/') {
                if (c >= 'a') {
                    color = BLACK;
                    c += 'A' - 'a';
                } else {
                    color = WHITE;
                }
                switch (c) {
                case 'P':
                    pos->sqtopc[row*8+col] = PC(color,PAWN);
                    PIECES(*pos, color, PAWN) |= MASK(SQ(col,row));
                    break;
                case 'N':
                    pos->sqtopc[row*8+col] = PC(color,KNIGHT);
                    PIECES(*pos, color, KNIGHT) |= MASK(SQ(col,row));                    
                    break;
                case 'B':
                    pos->sqtopc[row*8+col] = PC(color,BISHOP);
                    PIECES(*pos, color, BISHOP) |= MASK(SQ(col,row));                    
                    break;
                case 'R':
                    pos->sqtopc[row*8+col] = PC(color,ROOK);
                    PIECES(*pos, color, ROOK) |= MASK(SQ(col,row));                    
                    break;
                case 'Q':
                    pos->sqtopc[row*8+col] = PC(color,QUEEN);
                    PIECES(*pos, color, QUEEN) |= MASK(SQ(col,row));                    
                    break;
                case 'K':
                    pos->sqtopc[row*8+col] = PC(color,KING);
                    PIECES(*pos, color, KING) |= MASK(SQ(col,row));                    
                    break;
                default:
                    return 1;
                    break;
                }
                ++col;
            }
        } while (col < 8);
    }

    ++p;
    pos->wtm = *p == 'w' ? WHITE : BLACK;
    p++;
    pos->castle = 0;

    ++p;
    while (*p && *p != ' ') {
        switch (*p) {
        case 'K':
            pos->castle |= WKINGSD;
            break;
        case 'Q':
            pos->castle |= WQUEENSD;
            break;
        case 'k':
            pos->castle |= BKINGSD;
            break;
        case 'q':
            pos->castle |= BQUEENSD;
            break;
        case '-':
            break;
        default:
            fprintf(stderr, "Unexpected character: '%c'\n", *p);
            return 1;
        }
        ++p;
    }
    ++p;

    if (!*p) {
        fprintf(stderr, "Unexpected end of input!\n");
        return 1;
    } else if (*p == '-') {
        pos->enpassant = NO_ENPASSANT;
    } else {
        int sq = 0;
        if (*p >= 'a' && *p <= 'h') {
            sq = *p - 'a';
        } else {
            fprintf(stderr, "Unexpected character in enpassant square: '%c'\n", *p);
            return 1;
        }

        ++p;
        if (*p >= '1' && *p <= '8') {
            sq += (*p - '1') * 8;
        } else {
            fprintf(stderr, "Unexpected character in enpassant square: '%c'\n", *p);
            return 1;
        }

        if ((sq >= A3 && sq <= H3) || (sq >= A6 && sq <= H6)) {
            // I store the square the pawn moved to, FEN gives the square behind the pawn
            if (pos->wtm == WHITE) {
                pos->enpassant = sq - 8 - 23;
            } else {
                pos->enpassant = sq + 8 - 23;
            }
            //printf("pos->enpassant = %d -> %s\n", pos->enpassant, sq_to_str[pos->enpassant+23]);
        } else {
            fprintf(stderr, "Invalid enpassant square: %d\n", sq);
            return 1;
        }
    }
    ++p;

    if (*p) {
        int halfmoves = 0;
        while (*p && *p != ' ') {
            if (*p >= '0' && *p <= '9') {
                halfmoves *= 10;
                halfmoves += *p - '0';
            } else {
                fprintf(stderr, "Unexpected character in halfmoves: '%c'\n", *p);
                return 1;
            }
        }
        pos->halfmoves = halfmoves;

        ++p;
        int fullmoves = 0;
        while (*p && *p != ' ') {
            if (*p >= '0' && *p <= '9') {
                fullmoves *= 10;
                fullmoves += *p - '0';
            } else {
                fprintf(stderr, "Unexpected character in fullmoves: '%c'\n", *p);
                return 1;
            }
        }
        pos->nmoves = fullmoves;
    }

    if (print) {
        full_position_print(pos);
    }
    assert(validate_position(pos) == 0);

    return 0;
}

void position_print(const uint8_t * const restrict sqtopc, FILE *ostream) {
    char v;
    int sq, r, c;
    fputs("---------------------------------\n", ostream);
    for (r = (RANKS - 1); r >= 0; --r) {
        for (c = 0; c < COLS; ++c) {
            sq = SQ(c, r);
            v = vpcs[sqtopc[sq]];
            fprintf(ostream, "| %c ", v);
        }
        fputs("|\n---------------------------------\n", ostream);
    }
}

void full_position_print(const struct position *p) {
    position_print(&p->sqtopc[0], stdout);
    printf("%s\n", p->wtm == WHITE ? "WHITE":"BLACK");
    printf("Full moves: %d\n", p->nmoves);
    printf("Half moves: %d\n", p->halfmoves);
    printf("Castle: 0x%02X\n", p->castle);
    printf("Castling: ");
    if ((p->castle & WKINGSD) != 0) {
        printf("K");
    }
    if ((p->castle & WQUEENSD) != 0) {
        printf("Q");
    }
    if ((p->castle & BKINGSD) != 0) {
        printf("k");
    }
    if ((p->castle & BQUEENSD) != 0) {
        printf("q");
    }
    printf("\n");
    printf("E.P.: %d\n", p->enpassant);
}

int validate_position(const struct position * const restrict p) {
    int i;
    int pc;
    uint64_t msk;
    uint8_t found;
    // white king present
    if (p->brd[PC(WHITE,KING)] == 0) {
        fputs("No white king present", stderr);
        return 1;
    }
    // black king present
    if (p->brd[PC(BLACK,KING)] == 0) {
        fputs("No black king present", stderr);
        return 2;
    }
    if (__builtin_popcountll(p->brd[PC(WHITE,KING)]) != 1) {
        fputs("Too many white kings present", stderr);
    }
    if (__builtin_popcountll(p->brd[PC(BLACK,KING)]) != 1) {
        fputs("Too many black kings present", stderr);
    }
    for (i = 0; i < SQUARES; ++i) {
        msk = MASK(i);
        found = 0;
        for (pc = PC(WHITE,PAWN); pc <= PC(BLACK,KING); ++pc) {
            if ((p->brd[pc] & msk) != 0) {
                if (p->sqtopc[i] != pc) {
                    fprintf(stderr, "p->brd[%c] != p->sqtopc[%s] = %c, found = %d\n",
                            vpcs[pc],
                            sq_to_str[i],
                            vpcs[p->sqtopc[i]],
                            found);
                    return 3;
                }
                found = 1;
            }
        }
        if (found == 0 && p->sqtopc[i] != EMPTY) {
            fprintf(stderr, "ERROR on square(%d)\n", i);
            fprintf(stderr, "Value at p->sqtopc[i] = %d\n", p->sqtopc[i]);
            fprintf(stderr, "bitboards = EMPTY but sqtopc[%s] == %c\n",
                    sq_to_str[i], vpcs[p->sqtopc[i]]);
            return 4;
        }
    }
    return 0;
}

void set_initial_position(struct position * restrict p) {
    int i;
    p->wtm = WHITE;
    p->nmoves = 0;
    p->halfmoves = 0;
    p->castle = WKINGSD | WQUEENSD | BKINGSD | BQUEENSD;
    PIECES(*p, WHITE, PAWN  ) = 0x0000ff00ULL;
    for (i = A2; i <= H2; ++i) p->sqtopc[i] = PC(WHITE, PAWN);
    PIECES(*p, WHITE, KNIGHT) = 0x00000042ULL;
    p->sqtopc[B1] = PC(WHITE, KNIGHT);
    p->sqtopc[G1] = PC(WHITE, KNIGHT);
    PIECES(*p, WHITE, BISHOP) = 0x00000024ULL;
    p->sqtopc[C1] = PC(WHITE, BISHOP);
    p->sqtopc[F1] = PC(WHITE, BISHOP);
    PIECES(*p, WHITE, ROOK  ) = 0x00000081ULL;
    p->sqtopc[A1] = PC(WHITE, ROOK);
    p->sqtopc[H1] = PC(WHITE, ROOK);
    PIECES(*p, WHITE, QUEEN ) = 0x00000008ULL;
    p->sqtopc[D1] = PC(WHITE, QUEEN);
    PIECES(*p, WHITE, KING  ) = 0x00000010ULL;
    p->sqtopc[E1] = PC(WHITE, KING);
    PIECES(*p, BLACK, PAWN  ) = 0xff000000000000ULL;
    for (i = A7; i <= H7; ++i) p->sqtopc[i] = PC(BLACK, PAWN);
    PIECES(*p, BLACK, KNIGHT) = 0x4200000000000000ULL;
    p->sqtopc[B8] = PC(BLACK, KNIGHT);
    p->sqtopc[G8] = PC(BLACK, KNIGHT);
    PIECES(*p, BLACK, BISHOP) = 0x2400000000000000ULL;
    p->sqtopc[C8] = PC(BLACK, BISHOP);
    p->sqtopc[F8] = PC(BLACK, BISHOP);
    PIECES(*p, BLACK, ROOK  ) = 0x8100000000000000ULL;
    p->sqtopc[A8] = PC(BLACK, ROOK);
    p->sqtopc[H8] = PC(BLACK, ROOK);
    PIECES(*p, BLACK, QUEEN ) = 0x800000000000000ULL;
    p->sqtopc[D8] = PC(BLACK, QUEEN);
    PIECES(*p, BLACK, KING  ) = 0x1000000000000000ULL;
    p->sqtopc[E8] = PC(BLACK, KING);
    for (i = A3; i < A7; ++i) p->sqtopc[i] = EMPTY;
}
