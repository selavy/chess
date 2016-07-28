#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
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
void move_print(move m) {
    printf("MOVE(%s, %s, %s, %d, %d)\n",
           sq_to_str[FROM(m)], sq_to_str[TO(m)],
           piecestr(PIECE(m)), PROMOTE(m), CAPTURE(m));
}
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
void debug_position_print(struct position * restrict p) {
    int i;
    printf("BitBoards:\n");
    for (i = 0; i < NPIECES*2; ++i) {
        printf("%" PRIu64 "\n", p->brd[i]);
    }
    printf("nmoves = %u\n", p->nmoves);
    printf("wtm = %u\n", p->wtm);
    printf("halfmoves = %u\n", p->halfmoves);
    printf("castle = %u\n", p->castle);
    printf("enpassant = %u\n", p->enpassant);
}
struct savepos {
    uint8_t halfmoves;
    uint8_t enpassant;
    uint8_t castle;
    uint8_t was_ep;
};
void make_move(struct position * restrict p, move m, struct savepos * restrict sp) {
    // TODO: castling
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
    } else {
        assert(0);
        pcs = &PIECES(*p, side, promotion-1);
        // TODO: implement
    }
    if (capture != NO_CAPTURE) {
        if (pc == PC(side, PAWN) && p->sqtopc[tosq] == EMPTY) { // e.p.
            sp->was_ep = 1;
            if (side == WHITE) {
                assert(tosq >= A6 && tosq <= H6);
                int sq = tosq << 8;
                assert(p->sqtopc[sq] == PC(BLACK,PAWN));
                p->sqtopc[sq] = EMPTY;
                p->brd[PC(BLACK,PAWN)] &= ~MASK(sq);
            } else {
                assert(tosq >= A3 && tosq <= H3);
                int sq = tosq >> 8;
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
}
void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp) {
    // TODO: undo castling
    // TODO: undo enpassant
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
    p->sqtopc[tosq] = capture;
    *pcs |= from;
    *pcs &= ~to;    
    if (sp->was_ep != 0) { // e.p.
        if (side == WHITE) {
            assert(capture == PC(BLACK,PAWN));
            int sq = tosq << 8;
            p->sqtopc[sq] = PC(BLACK,PAWN);
            p->brd[PC(BLACK,PAWN)] |= MASK(sq);
        } else {
            assert(capture == PC(WHITE,PAWN));
            int sq = tosq >> 8;
            p->sqtopc[sq] = PC(WHITE,PAWN);
            p->brd[PC(WHITE,PAWN)] |= MASK(sq);            
        }
    } else {
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

    char wtm;
    int hf, cstl, ep;
    assert(fscanf(fp, "%c %d %d %d\n", &wtm, &hf, &cstl, &ep) == 4);

    p->wtm = wtm == 'W' ? WHITE : BLACK;
    p->halfmoves = (uint8_t)hf;
    p->castle = cstl;
    p->enpassant = ep;
    p->nmoves = 0; // TODO: fix me

    char pc, c1, c2, cap, prm;
    int r1, r2;
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
    *m = MOVE(to, from, int_to_piece(pc), int_to_piece(cap), prm);
}

int main(int argc, char **argv) {
    static struct position pos;
    static struct position tmp;
    static struct savepos sp;
    move m;    

    //#if 0 
    FILE *fp = fopen("test_cases.txt", "r");
    if (!fp) {
        fputs("Unable to open \"test_cases.txt\"\n", stderr);
        exit(1);
    }

    int c;
    while ((c = fgetc(fp)) != EOF) {
        ungetc(c, fp);
        
        read_position_from_file(fp, &pos, &m);
        //position_print(&pos.sqtopc[0]);
        assert(validate_position(&pos) == 0);
        memcpy(&tmp, &pos, sizeof(tmp));

        make_move(&pos, m, &sp);
        position_print(&pos.sqtopc[0]);
        move_print(m);                        
        assert(validate_position(&pos) == 0);

        undo_move(&pos, m, &sp);
        position_print(&pos.sqtopc[0]);
        assert(validate_position(&pos) == 0);
        assert(memcmp(&tmp, &pos, sizeof(tmp)) == 0);
    }
    
    fclose(fp);
    //#endif 

    #if 0
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
    #endif
    
    return 0;
}
