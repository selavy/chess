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
#include "movegen.h"

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
