#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define FLIP(side) ((side)^1)
#define COLS 8
#define RANKS 8
#define SQUARES 64
#define MAX_MOVES 256
#define MASK(x) ((uint64_t)1 << x)
#define SQ(c,r) ((r)*COLS+(c))
enum { WHITE=0, BLACK };
enum { PAWN=0, KNIGHT, BISHOP, ROOK, QUEEN, KING, NPIECES, EMPTY=(NPIECES*2) };
static char vpcs[] = {
    'P', 'N', 'B', 'R', 'Q', 'K',
    'p', 'n', 'b', 'r', 'q', 'k',
    ' '
};
#define PC(side, type) (((side)*NPIECES)+(type))
#define PIECES(p, side, type) (p).brd[PC(side, type)]
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
#define MOVE(to, from, pc, cap, prm)            \
    (((from) & 0x3f)         |                  \
     (((to ) & 0x3f) << 6)   |                  \
     (((pc ) & 0x0f) << 12)  |                  \
     (((prm) & 0x07) << 16)  |                  \
     (((cap) & 0x0f) << 19))
#define FROM(m) ((m) & 0x3f)
#define TO(m) (((m) >> 6) & 0x3f)
#define PIECE(m) (((m) >> 12) & 0xf)
#define PROMOTE(m) (((m) >> 16) & 0x7)
#define CAPTURE(m) (((m) >> 19) & 0xf)
#define NO_PROMOTION 0
#define NO_CAPTURE EMPTY
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
struct savepos {
    uint8_t halfmoves;
    uint8_t enpassant;
    uint8_t castle;
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

    *pcs &= ~from;
    if (promotion == NO_PROMOTION) {
        *pcs |= to;
    } else {
        pcs = &PIECES(*p, side, promotion);
    }
    if (capture != NO_CAPTURE) {
        assert(p->sqtopc[tosq] != EMPTY);
        assert((p->brd[p->sqtopc[tosq]] & to) != 0);
        p->brd[p->sqtopc[tosq]] &= ~to;
    }
    p->sqtopc[fromsq] = EMPTY;
    p->sqtopc[tosq] = pc;
    // REVISIT: improve
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
         
    p->wtm = FLIP(p->wtm);
    p->halfmoves = pc == PC(side,PAWN) ? 0 : ++p->halfmoves;
    ++p->nmoves;
}
/* struct position { */
/*     uint64_t brd[NPIECES*2];  // 8 * 12 = 96B */
/*     uint8_t  sqtopc[SQUARES]; // 1 * 64 = 64B */
/*     uint16_t nmoves;          // 2 *  1 =  2B */
/*     uint8_t  wtm;             // 1 *  1 =  1B */
/*     uint8_t  halfmoves;       // 1 *  1 =  1B */
/*     uint8_t  castle;          // 1 *  1 =  1B */
/*     uint8_t  enpassant;       // 1 *  1 =  1B */
/* };                            // Total:  164B */
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
    
    p->sqtopc[fromsq] = pc;
    p->sqtopc[tosq] = capture;
    if (promotion == NO_PROMOTION) {
        *pcs &= ~to;
        *pcs |= from;        
        if (capture != NO_CAPTURE) {
            p->brd[capture] |= to;
        }
    } else {
        assert(0); // TODO
    }
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
void set_initial_position(struct position * restrict pos) {
    int i;
    pos->wtm = WHITE;
    pos->nmoves = 0;
    pos->halfmoves = 0;
    PIECES(*pos, WHITE, PAWN  ) = 0x0000ff00ULL;
    for (i = A2; i <= H2; ++i) pos->sqtopc[i] = PC(WHITE, PAWN);
    PIECES(*pos, WHITE, KNIGHT) = 0x00000042ULL;
    pos->sqtopc[B1] = PC(WHITE, KNIGHT);
    pos->sqtopc[G1] = PC(WHITE, KNIGHT);
    PIECES(*pos, WHITE, BISHOP) = 0x00000024ULL;
    pos->sqtopc[C1] = PC(WHITE, BISHOP);
    pos->sqtopc[F1] = PC(WHITE, BISHOP);
    PIECES(*pos, WHITE, ROOK  ) = 0x00000081ULL;
    pos->sqtopc[A1] = PC(WHITE, ROOK);
    pos->sqtopc[H1] = PC(WHITE, ROOK);    
    PIECES(*pos, WHITE, QUEEN ) = 0x00000008ULL;
    pos->sqtopc[D1] = PC(WHITE, QUEEN);
    PIECES(*pos, WHITE, KING  ) = 0x00000010ULL;
    pos->sqtopc[E1] = PC(WHITE, KING);
    PIECES(*pos, BLACK, PAWN  ) = 0xff000000000000ULL;
    for (i = A7; i <= H7; ++i) pos->sqtopc[i] = PC(BLACK, PAWN);
    PIECES(*pos, BLACK, KNIGHT) = 0x4200000000000000ULL;
    pos->sqtopc[B8] = PC(BLACK, KNIGHT);
    pos->sqtopc[G8] = PC(BLACK, KNIGHT);
    PIECES(*pos, BLACK, BISHOP) = 0x2400000000000000ULL;
    pos->sqtopc[C8] = PC(BLACK, BISHOP);
    pos->sqtopc[F8] = PC(BLACK, BISHOP);    
    PIECES(*pos, BLACK, ROOK  ) = 0x8100000000000000ULL;
    pos->sqtopc[A8] = PC(BLACK, ROOK);
    pos->sqtopc[H8] = PC(BLACK, ROOK);
    PIECES(*pos, BLACK, QUEEN ) = 0x800000000000000ULL;
    pos->sqtopc[D8] = PC(BLACK, QUEEN);
    PIECES(*pos, BLACK, KING  ) = 0x1000000000000000ULL;
    pos->sqtopc[E8] = PC(BLACK, KING);
    for (i = A3; i < A7; ++i) pos->sqtopc[i] = EMPTY;
}
int validate_position(const struct position * restrict p) {
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
                found = 1;
                if (p->sqtopc[i] != pc) {
                    fprintf(stderr, "p->brd[%s] != p->sqtopc[%d] = %s\n",
                            piecestr(pc), i, piecestr(p->sqtopc[i]));
                        
                    return 3;
                }
            }
        }
        if (found == 0 && p->sqtopc[i] != EMPTY) {
            return 4;
        }
    }
    return 0;
}
int main(int argc, char **argv) {
    static struct position pos;
    static struct position tmp;
    static struct savepos sp;
    move m;
    
    set_initial_position(&pos);
    position_print(&pos.sqtopc[0]);
    assert(validate_position(&pos) == 0);
    memcpy(&tmp, &pos, sizeof(tmp));
    
    m = MOVE(SQ(4,3), SQ(4,1), PC(WHITE,PAWN), NO_CAPTURE, NO_PROMOTION);
    make_move(&pos, m, &sp);
    position_print(&pos.sqtopc[0]);
    assert(validate_position(&pos) == 0);
    
    undo_move(&pos, m, &sp);
    position_print(&pos.sqtopc[0]);
    assert(validate_position(&pos) == 0);
    assert(memcmp(&tmp, &pos, sizeof(tmp)) == 0);
    
    return 0;
}
