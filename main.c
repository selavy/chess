#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include "types.h"
#include "move.h"

struct position {
    uint64_t brd[NPIECES*2];  // 8 * 12 = 96B
    uint8_t  sqtopc[SQUARES]; // 1 * 64 = 64B
    uint16_t nmoves;          // 2 *  1 =  2B
    uint8_t  wtm;             // 1 *  1 =  1B
    uint8_t  halfmoves;       // 1 *  1 =  1B
    uint8_t  castle;          // 1 *  1 =  1B
    uint8_t  enpassant;       // 1 *  1 =  1B
};                            // Total:  164B
#define FULLSIDE(b, s) ((b).brd[(s)*NPIECES+PAWN]|(b).brd[(s)*NPIECES+KNIGHT]|(b).brd[(s)*NPIECES+BISHOP]|(b).brd[(s)*NPIECES+ROOK]|(b).brd[(s)*NPIECES+QUEEN]|(b).brd[(s)*NPIECES+KING])
void position_print(const uint8_t * const restrict sqtopc) {
    char v;
    int sq, r, c;
    fputs("---------------------------------\n", stdout);
    for (r = (RANKS - 1); r >= 0; --r) {
        for (c = 0; c < COLS; ++c) {
            sq = SQ(c, r);
            v = vpcs[sqtopc[sq]];
            fprintf(stdout, "| %c ", v);
        }
        fputs("|\n---------------------------------\n", stdout);
    }
}
void full_position_print(const struct position *p) {
    position_print(&p->sqtopc[0]);
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
struct savepos {
    uint8_t halfmoves;
    uint8_t enpassant;
    uint8_t castle;
    uint8_t was_ep;
};
int is_castle(move m) {
    uint32_t pc = PIECE(m);
    uint32_t from = FROM(m);
    uint32_t to = TO(m);
    if (pc == PC(WHITE,KING)) {
        return from == E1 && (to == C1 || to == G1);
    } else if (pc == PC(BLACK,KING)) {
        return from == E8 && (to == C8 || to == G8);
    } else {
        return 0;
    }
}
// `validate_position': verify both sides have a king and
// that the sqtopc array matches the bitboards.
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
                    fprintf(stderr, "p->brd[%c] != p->sqtopc[%d] = %c, found = %d\n",
                            vpcs[pc], i, vpcs[p->sqtopc[i]], found);
                    return 3;
                }
                found = 1;
            }
        }
        if (found == 0 && p->sqtopc[i] != EMPTY) {
            return 4;
        }
    }
    return 0;
}
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
// return 1 if `side`s king is attacked
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

uint8_t int_to_piece(int c) {
    switch (c) {
    case 'P': return PC(WHITE,PAWN);
    case 'N': return PC(WHITE,KNIGHT);
    case 'B': return PC(WHITE,BISHOP);
    case 'R': return PC(WHITE,ROOK);
    case 'Q': return PC(WHITE,QUEEN);
    case 'K': return PC(WHITE,KING);
    case 'p': return PC(BLACK,PAWN);
    case 'n': return PC(BLACK,KNIGHT);
    case 'b': return PC(BLACK,BISHOP);
    case 'r': return PC(BLACK,ROOK);
    case 'q': return PC(BLACK,QUEEN);
    case 'k': return PC(BLACK,KING);
    case ' ': return EMPTY;
    case '_': return EMPTY;
    default:
        assert(0);
        return EMPTY;
    }
}
static uint64_t checks = 0;
static uint64_t captures = 0;
static uint64_t enpassants = 0;
static uint64_t castles = 0;
static uint64_t promotions = 0;
static uint64_t checkmates = 0;

uint64_t perft_ex(int depth, struct position * const restrict pos, move pmove, int ply) {
    uint32_t i;
    uint32_t nmoves;
    uint64_t nodes = 0;
    struct savepos sp;
    move moves[MAX_MOVES];
    
    if (in_check(pos, FLIP(pos->wtm))) {
        return 0;
    }
    if (depth == 0) {
//#define COUNTERS
#ifdef COUNTERS        
        if (pmove != 0) {
            if (in_check(pos, pos->wtm) != 0) {
                ++checks;
            }
            if (is_castle(pmove) != 0) {
                ++castles;
            }
            if (CAPTURE(pmove) != NO_CAPTURE) {
                ++captures;
                if (ENPASSANT(pmove) != 0) {
                    ++enpassants;
                }
            }
            if (PROMOTE(pmove) != NO_PROMOTION) {
                ++promotions;
            }
        }
#endif // ~COUNTERS
        return 1;
    }
    
    nmoves = generate_moves(pos, &moves[0]);
    for (i = 0; i < nmoves; ++i) {
        make_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
        nodes += perft_ex(depth - 1, pos, moves[i], ply + 1);
        undo_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
    }

    return nodes;
}
uint64_t perft(int depth) {
    static struct position pos;
    set_initial_position(&pos);
    return perft_ex(depth, &pos, 0, 0);
}
int read_fen(struct position * restrict pos, const char * const fen) {
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
                    printf("BAD\n");
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
            exit(EXIT_FAILURE);
        }
        ++p;
    }
    ++p;

    if (!*p) {
        fprintf(stderr, "Unexpected end of input!\n");
        exit(EXIT_FAILURE);
    } else if (*p == '-') {
        pos->enpassant = NO_ENPASSANT;
    } else {
        int sq = 0;
        if (*p >= 'a' && *p <= 'h') {
            sq = *p - 'a';
        } else {
            fprintf(stderr, "Unexpected character in enpassant square: '%c'\n", *p);
            exit(EXIT_FAILURE);
        }

        ++p;
        if (*p >= '1' && *p <= '8') {
            sq += (*p - '1') * 8;
        } else {
            fprintf(stderr, "Unexpected character in enpassant square: '%c'\n", *p);
            exit(EXIT_FAILURE);            
        }

        if ((sq >= A3 && sq <= H3) || (sq >= A6 && sq <= H6)) {
            // I store the square the pawn moved to, FEN gives the square behind the pawn
            if (pos->wtm == WHITE) {
                pos->enpassant = sq + 8 - 23;
            } else {
                pos->enpassant = sq - 8 - 23;
            }
        } else {
            fprintf(stderr, "Invalid enpassant square: %d\n", sq);
            exit(EXIT_FAILURE);
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
                exit(EXIT_FAILURE);
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
                exit(EXIT_FAILURE);
            }
        }
        pos->nmoves = fullmoves;
    }

    full_position_print(pos);
    assert(validate_position(pos) == 0);

    return 0;
}

int main(int argc, char **argv) {
    int depth;
    uint64_t nodes;
    static struct position pos;

#ifdef NDEBUG
    printf("Built in `release' mode\n");
#else
    printf("Built in `debug' mode\n");
#endif

    #define DEPTH 8
    
    // starting position:
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

    // kiwi pete position:
    /* const char *fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"; */

    // position #3
    //const char *fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";

    // position #4
    //const char *fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -";

    // position #5
    //const char* fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -";

    // position #6
    //const char* fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -";
    
    printf("Perft:\n");
    if (read_fen(&pos, fen) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        exit(EXIT_FAILURE);
    }

    for (depth = DEPTH; depth < DEPTH+1; ++depth) {
        checks = 0;
        captures = 0;
        enpassants = 0;
        castles = 0;
        checkmates = 0;
        promotions = 0;
        
        nodes = perft_ex(depth, &pos, 0, 0);
        printf("Perft(%u): Nodes=%" PRIu64 ", Captures=%" PRIu64 ", E.p.=%" PRIu64
                ", Castles=%" PRIu64 ", Promotions=%" PRIu64
                ", Checks=%" PRIu64 ", Checkmates=%" PRIu64 "\n",
                depth, nodes, captures, enpassants, castles, promotions,
                checks, checkmates);
    }

    return 0;
}
