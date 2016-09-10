#include "types.h"
#include <stdio.h>
#include <assert.h>

const char *vpcs = "PNBRQKpnbrqk ";

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

const char *sq_to_small[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

const uint32_t PROMOPC[4] = {
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN
};

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

