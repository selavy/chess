#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include "magic_tables.h"
#include <sys/types.h>
#include <unistd.h>

#define BOOLSTR(x) ((x)?"TRUE":"FALSE")
#define FLIP(side) ((side)^1)
#define COLS 8
#define RANKS 8
#define SQUARES 64
#define MAX_MOVES 256
#define MASK(x) ((uint64_t)1 << x)
#define SQ(c,r) ((r)*COLS+(c))
enum { WHITE=0, BLACK };
#define pawn_attacks(side, sq) ((side) == WHITE ? wpawn_attacks[sq] : bpawn_attacks[sq])
enum { PAWN=0, KNIGHT, BISHOP, ROOK, QUEEN, KING, NPIECES, EMPTY=(NPIECES*2) };
static const char *vpcs = "PNBRQKpnbrqk ";
#define PC(side, type) (((side)*NPIECES)+(type))
#define PIECES(p, side, type) (p).brd[PC(side, type)]
#define WHITE_ENPASSANT_SQUARES 0x00000000ff000000
#define BLACK_ENPASSANT_SQUARES 0x000000ff00000000
#define SECOND_RANK 0xff00ull
#define SEVENTH_RANK 0xff000000000000ull
#define RANK7(side) ((side) == WHITE ? SEVENTH_RANK : SECOND_RANK)
#define A_FILE 0x101010101010101ULL
#define H_FILE   0x8080808080808080ULL
#define RANK2(side) ((side) == WHITE ? SECOND_RANK : SEVENTH_RANK)
#define THIRD_RANK 0xff0000ULL
#define SIXTH_RANK 0xff0000000000ULL
#define RANK3(side) ((side) == WHITE ? THIRD_RANK : SIXTH_RANK)
enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};
const char *sq_to_str[64] = {
    "A1", "B1", "C1", "D1", "E1", "F1", "G1", "H1",
    "A2", "B2", "C2", "D2", "E2", "F2", "G2", "H2",
    "A3", "B3", "C3", "D3", "E3", "F3", "G3", "H3",
    "A4", "B4", "C4", "D4", "E4", "F4", "G4", "H4",
    "A5", "B5", "C5", "D5", "E5", "F5", "G5", "H5",
    "A6", "B6", "C6", "D6", "E6", "F6", "G6", "H6",
    "A7", "B7", "C7", "D7", "E7", "F7", "G7", "H7",
    "A8", "B8", "C8", "D8", "E8", "F8", "G8", "H8"
};
const char *piecestr(uint32_t piece) {
    switch (piece) {
    case PAWN             : return "WHITE PAWN";
    case KNIGHT           : return "WHITE KNIGHT";
    case BISHOP           : return "WHITE BISHOP";
    case ROOK             : return "WHITE ROOK";
    case QUEEN            : return "WHITE QUEEN";
    case KING             : return "WHITE KING";
    case PC(BLACK,PAWN)   : return "BLACK PAWN";
    case PC(BLACK,KNIGHT) : return "BLACK KNIGHT";
    case PC(BLACK,BISHOP) : return "BLACK BISHOP";
    case PC(BLACK,ROOK)   : return "BLACK ROOK";
    case PC(BLACK,QUEEN)  : return "BLACK QUEEN";
    case PC(BLACK,KING)   : return "BLACK KING";
    case EMPTY            : return "EMPTY";
    default               : return "UNKNOWN";
    }
}
// to   [0..63]    6 bits
// from [0..63]    6 bits
// piece [0..5*2]  4 bits
// capture [0..11] 4 bit
// promote [0..5]  3 bit
typedef uint32_t move;
#define MOVE(from, to, pc, cap, prm, ep)        \
    (((to) & 0x3f)           |                  \
     (((from ) & 0x3f) << 6) |                  \
     (((pc ) & 0x0f) << 12)  |                  \
     (((prm) & 0x07) << 16)  |                  \
     (((cap) & 0x0f) << 19)  |                  \
     (((ep ) & 0x01) << 23))  
#define FROM(m)      (((m) >>  0) & 0x3f)
#define TO(m)        (((m) >>  6) & 0x3f)
#define PIECE(m)     (((m) >> 12) & 0x0f)
#define PROMOTE(m)   (((m) >> 16) & 0x07)
#define CAPTURE(m)   (((m) >> 19) & 0x0f)
#define ENPASSANT(m) (((m) >> 23) & 0x01)
void move_print(move m) {
    printf("MOVE(from=%s, to=%s, pc=%s, prm=%d, cap=%s, ep=%s)",
           sq_to_str[FROM(m)], sq_to_str[TO(m)],
           piecestr(PIECE(m)), PROMOTE(m), piecestr(CAPTURE(m)), BOOLSTR(ENPASSANT(m)));
}
#define NO_PROMOTION 0
#define NO_CAPTURE EMPTY
#define NO_ENPASSANT 0
enum {
    WKINGSD  = (1<<0), WQUEENSD = (1<<1),
    BKINGSD  = (1<<2), BQUEENSD = (1<<3),
};
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
struct savepos {
    uint8_t halfmoves;
    uint8_t enpassant;
    uint8_t castle;
    uint8_t was_ep;
};
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
    if (pc == PC(WHITE,KING) && tosq == C1) {
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
        return;
    } else if (pc == PC(WHITE,KING) && tosq == G1) {
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
        return;
    } else if (pc == PC(BLACK,KING) && tosq == C8) {
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
        return;
    } else if (pc == PC(BLACK,KING) && tosq == G8) {
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
        return;
    }

    *pcs &= ~from;
    if (promotion == NO_PROMOTION) {
        *pcs |= to;
    } else { // TODO: promotions
        assert(0);
        pcs = &PIECES(*p, side, promotion-1);
        // TODO: implement
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
                p->sqtopc[sq] = EMPTY;
                p->brd[PC(BLACK,PAWN)] &= ~MASK(sq);
            } else {
                assert(tosq >= A3 && tosq <= H3);
                int sq = tosq + 8;
                assert(p->enpassant != NO_ENPASSANT);
                assert(sq == 23 + p->enpassant);
                assert(p->sqtopc[sq] == PC(WHITE,PAWN));
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
    p->sqtopc[tosq] = pc;

    // REVISIT: improve
    // update castling flags if needed
    if (p->castle != 0) {
        if (pc == PC(WHITE,ROOK) && fromsq == A1) {
            p->castle &= ~WQUEENSD;
        } else if (pc == PC(WHITE,ROOK) && fromsq == H1) {
            p->castle &= ~WKINGSD;
        } else if (pc == PC(BLACK,ROOK) && fromsq == A8) {
            p->castle &= ~BQUEENSD;
        } else if (pc == PC(BLACK,ROOK) && fromsq == H8) {
            p->castle &= ~BKINGSD;
        } else if (pc == PC(WHITE,KING)) {
            p->castle &= ~(WQUEENSD | WKINGSD);
        } else if (pc == PC(BLACK,KING)) {
            p->castle &= ~(BQUEENSD | BKINGSD);
        }
    }

    p->enpassant = NO_ENPASSANT;
    if (capture == NO_CAPTURE) {
        if (pc == PC(WHITE,PAWN) && (to & WHITE_ENPASSANT_SQUARES) != 0 && (from & RANK2(side)) != 0) {
            p->enpassant = tosq - 23;
        } else if (pc == PC(BLACK,PAWN) && (to & BLACK_ENPASSANT_SQUARES) != 0 && (from & RANK2(side)) != 0) {
            p->enpassant = tosq - 23;
        }
    }
}
void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp) {
    uint32_t pc = PIECE(m);
    uint32_t fromsq = FROM(m);
    uint32_t tosq = TO(m);
    uint32_t capture = CAPTURE(m);
    uint32_t promotion = PROMOTE(m);
    uint8_t side = FLIP(p->wtm);
    uint64_t from = MASK(fromsq);
    uint64_t to = MASK(tosq);
    uint64_t * restrict pcs = &p->brd[pc];

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
    // TODO: generate attack square data
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
    if ((pawn_attacks(side, square) & pcs) != 0) {
        return 1;
    }

    #if 0
    if ((rook_attacks[square] & (Rooks(side) | Queens(side)))
        && (RookAttacks(square,
                        OccupiedSquares) & (Rooks(side) | Queens(side))))
        return 1;
    if ((bishop_attacks[square] & (Bishops(side) | Queens(side)))
        && (BishopAttacks(square,
                          OccupiedSquares) & (Bishops(side) | Queens(side))))
        return 1;
    if (KnightAttacks(square) & Knights(side))
        return 1;
    if (PawnAttacks(Flip(side), square) & Pawns(side))
        return 1;
    if (KingAttacks(square) & Kings(side))
        return 1;
    return 0;
    #endif
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

    // knight moves
    pcs = PIECES(*pos, side, KNIGHT);
    if (pcs != 0) {
        pc = PC(side, KNIGHT);
        for (i = 0; i < 64 && pcs; ++i, pcs >>= 1) {
            if ((pcs & 0x01) != 0) {
                posmoves = knight_attacks[i] & ~same;
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
    pc = PC(side, KING);
    for (i = 0; i < 64 && (pcs & 0x01) == 0; ++i, pcs >>= 1);
    assert(i < 64 && i >= 0);
    posmoves = king_attacks[i] & ~same;
    if (posmoves != 0) {
        for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
            if (posmoves & 0x1) {
                moves[nmove++] = MOVE(sq, i, pc, pos->sqtopc[sq], 0, 0);
            }
        }
        pcs &= ~msk;
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
    // TODO: promotion
    uint32_t fromsq;
    pc = PC(side, PAWN);
    pcs = PIECES(*pos, side, PAWN);

    // forward 1 square
    posmoves = side == WHITE ? pcs << 8 : pcs >> 8;
    posmoves &= ~occupied;
    for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
        if ((posmoves & 0x01) != 0) {
            fromsq = side == WHITE ? sq - 8 : sq + 8;
            // TODO: if on last rank, generate promotions
            moves[nmove++] = MOVE(sq, fromsq, pc, EMPTY, 0, 0);
        }
    }

    // forward 2 squares
    posmoves = pcs & RANK2(side);
    posmoves = side == WHITE ? posmoves << 16 : posmoves >> 16;
    posmoves &= ~occupied;
    for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
        if ((posmoves & 0x01) != 0) {
            fromsq = side == WHITE ? sq - 16 : sq + 16;
            // TODO: can i do this with the bitmasks?
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
            // TODO: if on last rank, generate promotions
            moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], 0, 0);
        }
    }

    // capture right
    posmoves = pcs & ~H_FILE;
    posmoves = side == WHITE ? posmoves << 9 : posmoves >> 7;
    posmoves &= contra;
    for (sq = 0; posmoves; ++sq, posmoves >>= 1) {
        if ((posmoves & 0x01) != 0) {
            fromsq = side == WHITE ? sq - 9 : sq + 7;
            // TODO: if on last rank, generate promotions
            moves[nmove++] = MOVE(sq, fromsq, pc, pos->sqtopc[sq], 0, 0);
        }
    }

    // en passant
    // TODO: improve
    if (pos->enpassant != NO_ENPASSANT) {
        uint32_t epsq = pos->enpassant + 23;
        assert(pos->sqtopc[epsq] == PC(contraside, PAWN));
        if (epsq != 24 && epsq != 32) {
            // try capture left
            fromsq = epsq - 1;
            if (pos->sqtopc[fromsq] == PC(side,PAWN)) {
                sq = side == WHITE ? fromsq + 9 : fromsq - 7;
                if (pos->sqtopc[sq] == EMPTY) {
                    moves[nmove++] = MOVE(sq, fromsq, pc, PC(contraside,PAWN), 0, 1);
                }
            }
        }
        if (epsq != 31 && epsq != 39) {
            // try capture right
            fromsq = epsq + 1;
            if (pos->sqtopc[fromsq] == PC(side,PAWN)) {
                sq = side == WHITE ? fromsq + 7 : fromsq - 9;                
                if (pos->sqtopc[sq] == EMPTY) {
                    moves[nmove++] = MOVE(sq, fromsq, pc, PC(contraside,PAWN), 0, 1);
                }
            }
        }
    }

    // TODO: castling

    return nmove;
}
void set_initial_position(struct position * restrict p) {
    int i;
    p->wtm = WHITE;
    p->nmoves = 0;
    p->halfmoves = 0;
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
    for (i = 0; i < SQUARES; ++i) {
        msk = MASK(i);
        found = 0;
        for (pc = PC(WHITE,PAWN); pc <= PC(BLACK,KING); ++pc) {
            if ((p->brd[pc] & msk) != 0) {
                if (p->sqtopc[i] != pc) {
                    fprintf(stderr, "p->brd[%s] != p->sqtopc[%d] = %s, found = %d\n",
                            piecestr(pc), i, piecestr(p->sqtopc[i]), found);

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
int compare_positions(const struct position * const restrict p1,
                      const struct position * const restrict p2) {
    // check sqtopc
    int i;
    for (i = 0; i < 64; ++i) {
        if (p1->sqtopc[i] != p2->sqtopc[i]) {
            fprintf(stderr, "[sqtopc] %s != %s on %s\n",
                    piecestr(p1->sqtopc[i]), piecestr(p2->sqtopc[i]),
                    sq_to_str[i]);
            goto failure;
        }
    }

    // check brd
    for (i = PC(WHITE,PAWN); i <= PC(BLACK,KING); ++i) {
        if (p1->brd[i] != p2->brd[i]) {
            fprintf(stderr, "[brd] 0x%016" PRIx64 " != 0x%016" PRIx64 " for %s\n",
                    p1->brd[i], p2->brd[i], piecestr(i));
            goto failure;
        }
    }

    if (p1->nmoves != p2->nmoves) {
        fprintf(stderr, "[nomves] %u != %u\n", p1->nmoves, p2->nmoves);
        goto failure;
    }

    if (p1->wtm != p2->wtm) {
        fprintf(stderr, "[wtm] %u != %u\n", p1->wtm, p2->wtm);
        goto failure;
    }

    if (p1->halfmoves != p2->halfmoves) {
        fprintf(stderr, "[halfmoves] %u != %u\n",
                p1->halfmoves, p2->halfmoves);
        goto failure;
    }

    if (p1->castle != p2->castle) {
        fprintf(stderr, "[castle] %u != %u\n",
                p1->castle, p2->castle);
        goto failure;
    }

    if (p1->enpassant != p2->enpassant) {
        fprintf(stderr, "[enpassant[ %u != %u\n",
                p1->enpassant, p2->enpassant);
        goto failure;
    }

    return 0;

 failure:
    position_print(&p1->sqtopc[0]);
    position_print(&p2->sqtopc[0]);
    return 1;
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
void read_position_from_file(FILE* fp, struct position * restrict p, move * restrict m) {
    int i, j, sq;
    for (i = 0; i < NPIECES*2; ++i) {
        p->brd[i] = 0ULL;
    }
    for (i = 0; i < 8; ++i) {
        assert(fscanf(fp, "---------------------------------\n") == 0);
        for (j = 0; j < 8; ++j) {
            assert(fgetc(fp) == '|');
            assert(fgetc(fp) == ' ');
            sq = (7-i)*8+j;
            p->sqtopc[sq] = int_to_piece(fgetc(fp));
            if (p->sqtopc[sq] != EMPTY) {
                p->brd[p->sqtopc[sq]] |= MASK(sq);
            }
            assert(fgetc(fp) == ' ');
        }
        assert(fgetc(fp) == '|');
        assert(fgetc(fp) == '\n');
    }
    assert(fscanf(fp, "---------------------------------\n") == 0);

    char wtm = 'W';
    int hf = 0, cstl = 0, ep = 0;
    assert(fscanf(fp, "%c %d %d %d\n", &wtm, &hf, &cstl, &ep) == 4);

    p->wtm = wtm == 'W' ? WHITE : BLACK;
    p->halfmoves = (uint8_t)hf;
    p->castle = cstl;
    p->enpassant = ep;
    p->nmoves = 0; // TODO: fix me

    char pc=0, c1=0, c2=0, cap=0, prm=0;
    int r1 = 0, r2 = 0;
    assert(fscanf(fp, "%c %c%d %c%d %c %c\n",
                  &pc, &c1, &r1, &c2, &r2, &cap, &prm) == 7);
    int from = (r1 - 1) * 8 + (c1 - 'A');
    int to = (r2 - 1) * 8 + (c2 - 'A');
    if (prm == '_') {
        prm = NO_PROMOTION;
    } else if (prm == 'P' || prm == 'p') {
        prm = PAWN;
    } else if (prm == 'N' || prm == 'n') {
        prm = KNIGHT;
    } else if (prm == 'B' || prm == 'b') {
        prm = BISHOP;
    } else if (prm == 'Q' || prm == 'q') {
        prm = QUEEN;
    } else if (prm == 'K' || prm == 'k') {
        prm = KING;
    } else {
        assert(0);
    }
    *m = MOVE(to, from, int_to_piece(pc), int_to_piece(cap), prm, 0);
}
static uint64_t checkcnt = 0;
static uint64_t capturecnt = 0;
static uint64_t enpassants = 0;
uint64_t perft_ex(int depth, struct position * const restrict pos, move pmove, int ply) {
    uint32_t i;
    uint32_t nmoves;
    uint64_t nodes = 0;
    uint64_t cnt;    
    move moves[MAX_MOVES];
    struct savepos sp;
    if (in_check(pos, FLIP(pos->wtm))) {
        return 0;
    }
    if (depth == 0) {
#define COUNTERS
#ifdef COUNTERS        
        if (in_check(pos, pos->wtm)) {
            ++checkcnt;
        }
        if (CAPTURE(pmove) != NO_CAPTURE) {
            ++capturecnt;
            if (ENPASSANT(pmove) != 0) {
                ++enpassants;
            }
        }
#endif        
        return 1;
    }

    nmoves = generate_moves(pos, &moves[0]);
    for (i = 0; i < nmoves; ++i) {
        make_move(pos, moves[i], &sp);
        cnt = perft_ex(depth - 1, pos, moves[i], ply + 1);
        nodes += cnt;
        undo_move(pos, moves[i], &sp);
        if (ply == 0) {
            move_print(moves[i]);
            printf(" %" PRIu64 "\n", cnt);
        }        
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
    printf("'%c'\n", *p);
    pos->wtm = *p == 'w' ? WHITE : BLACK;
    p++;

    // TODO: castle rights
    // TODO: enpassant square
    // TODO: halfmoves
    // TODO: fullmove number

    position_print(&pos->sqtopc[0]);
    printf("%s\n", pos->wtm == WHITE ? "WHITE":"BLACK");
    
    assert(validate_position(pos) == 0);

    return 0;
}

int main(int argc, char **argv) {
    printf("Perft:\n");
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    static struct position pos;
    uint64_t res;
    for (int depth = 6; depth < 7; ++depth) {
        checkcnt = 0;
        capturecnt = 0;
        enpassants = 0;
        if (read_fen(&pos, fen) != 0) {
            fputs("Failed to read FEN for position!", stderr);
            break;
        }
        res = perft_ex(depth, &pos, 0, 0);
        printf("Perft(%u) = %" PRIu64 ", "
               "Check Count = %" PRIu64 ", "
               "Capture Count = %" PRIu64 ", "
               "Enpassant Count = %" PRIu64 "\n"
               , depth, res, checkcnt, capturecnt, enpassants);
    }

    return 0;
}
