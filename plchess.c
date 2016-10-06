#define _GNU_SOURCE
#include "plchess.h"
#include <inttypes.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "move.h"
#include "movegen.h"

// --- Macros ---
/* #define BOOLSTR(x) ((x)?"TRUE":"FALSE") */
/* #define FLIP(side) ((side)^1) */
/* #define COLS 8 */
/* #define RANKS 8 */
/* #define SQUARES 64 */
/* #define MAX_MOVES 256 */
/* #define MASK(x) ((uint64_t)1 << x) */
/* #define SQ(c,r) ((r)*COLS+(c)) */
/* #define FORWARD_ONE_RANK(wtm, sq) ((wtm) == WHITE ? (sq) + 8 : (sq) - 8) */
/* #define BACKWARD_ONE_RANK(wtm, sq) ((wtm) == WHITE ? (sq) - 8 : (sq) + 8) */
/* #define pawn_attacks(side, sq) ((side) == WHITE ? wpawn_attacks[sq] : bpawn_attacks[sq]) */
/* #define PC(side, type) (((side)*NPIECES)+(type)) */
/* #define PIECES(p, side, type) (p).brd[PC(side, type)] */
/* #define WHITE_ENPASSANT_SQUARES 0x00000000ff000000 */
/* #define BLACK_ENPASSANT_SQUARES 0x000000ff00000000 */
/* #define EP_SQUARES(side) ((side)==WHITE ? WHITE_ENPASSANT_SQUARES : BLACK_ENPASSANT_SQUARES) */
/* #define SECOND_RANK 0xff00ull */
/* #define SEVENTH_RANK 0xff000000000000ull */
/* #define RANK7(side) ((side) == WHITE ? SEVENTH_RANK : SECOND_RANK) */
/* #define A_FILE 0x101010101010101ULL */
/* #define H_FILE   0x8080808080808080ULL */
/* #define RANK2(side) ((side) == WHITE ? SECOND_RANK : SEVENTH_RANK) */
/* #define THIRD_RANK 0xff0000ULL */
/* #define SIXTH_RANK 0xff0000000000ULL */
/* #define RANK3(side) ((side) == WHITE ? THIRD_RANK : SIXTH_RANK) */
/* #define NO_PROMOTION 0 */
/* #define NO_CAPTURE EMPTY */
/* #define NO_ENPASSANT 0 */
/* #define FULLSIDE(b, s) ((b).brd[(s)*NPIECES+PAWN]|(b).brd[(s)*NPIECES+KNIGHT]|(b).brd[(s)*NPIECES+BISHOP]|(b).brd[(s)*NPIECES+ROOK]|(b).brd[(s)*NPIECES+QUEEN]|(b).brd[(s)*NPIECES+KING]) */
/* #define SIDESTR(side) ((side)==WHITE?"WHITE":"BLACK") */
/* #define CSL(side) ((side) == WHITE ? (WKINGSD|WQUEENSD) : (BKINGSD|BQUEENSD)) */

/* // --- Enums --- */
/* enum { WHITE=0, BLACK }; */

/* enum { PAWN=0, KNIGHT, BISHOP, ROOK, QUEEN, KING, NPIECES, EMPTY=(NPIECES*2) }; */

/* enum { */
/*     A1, B1, C1, D1, E1, F1, G1, H1, */
/*     A2, B2, C2, D2, E2, F2, G2, H2, */
/*     A3, B3, C3, D3, E3, F3, G3, H3, */
/*     A4, B4, C4, D4, E4, F4, G4, H4, */
/*     A5, B5, C5, D5, E5, F5, G5, H5, */
/*     A6, B6, C6, D6, E6, F6, G6, H6, */
/*     A7, B7, C7, D7, E7, F7, G7, H7, */
/*     A8, B8, C8, D8, E8, F8, G8, H8, */
/* }; */

/* enum { */
/*     WKINGSD  = (1<<0), WQUEENSD = (1<<1), */
/*     BKINGSD  = (1<<2), BQUEENSD = (1<<3), */
/* }; */

/* // --- Structs --- */
/* struct position { */
/*     uint64_t brd[NPIECES*2];  // 8 * 12 = 96B */
/*     uint8_t  sqtopc[SQUARES]; // 1 * 64 = 64B */
/*     uint16_t nmoves;          // 2 *  1 =  2B */
/*     uint8_t  wtm;             // 1 *  1 =  1B */
/*     uint8_t  halfmoves;       // 1 *  1 =  1B */
/*     uint8_t  castle;          // 1 *  1 =  1B */
/*     uint8_t  enpassant;       // 1 *  1 =  1B */
/* };                            // Total:  164B */

/* struct savepos { */
/*     uint8_t halfmoves; */
/*     uint8_t enpassant; */
/*     uint8_t castle; */
/*     uint8_t was_ep; */
/* }; */

/* struct savepos { */
/*     uint8_t halfmoves; */
/*     uint8_t enpassant; */
/*     uint8_t castle; */
/*     uint8_t was_ep; */
/*     uint8_t captured_pc; // EMPTY if no capture */
/* }; */


// --- Global Data ---
/* const char *vpcs = "PNBRQKpnbrqk "; */

/* const uint32_t PROMOPC[5] = { 0, KNIGHT, BISHOP, ROOK, QUEEN }; */

// --- Interface Functions ---
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

// --- Static Function Prototypes ---
//static void make_move(struct position *restrict p, move m, struct savepos *restrict sp);
//static uint32_t gen_legal_moves(const struct position *const restrict pos, move *restrict moves);
static int read_fen(struct position * restrict pos, const char * const fen, int print);

// --- Static Function Definitions ---

static int read_fen(struct position * restrict pos, const char * const fen, int print) {
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

//================================================================================
// Tests
//================================================================================

// --- Test Structures ---
struct expected_t {
    uint64_t nodes;
    uint64_t captures;    
    uint64_t enpassants;
    uint64_t castles;    
    uint64_t promotions;    
    uint64_t checks;    
    uint64_t checkmates;
};

// --- Global Test Data ---
uint64_t checks     = 0;
uint64_t captures   = 0;
uint64_t enpassants = 0;
uint64_t castles    = 0;
uint64_t promotions = 0;
uint64_t checkmates = 0;

// --- Static Test Function Prototypes ---
static void reset_counts();
static int perft_count_test();
static int perft_count_test_ex(const char *name, const char *fen, const struct expected_t *expected, int max_depth);
static int perft_bulk_test_ex(const char *name, const char *fen, const uint64_t *expected, int max_depth);
static uint64_t perft(int depth, struct position *const restrict pos, move pmove, int cap);
static int test_make_move_ex(const char *fen, const move *moves);
static int test_undo_move_ex(const char *fen, const move *moves);
static int test_move_creation();
static int position_cmp(const struct position *restrict l, const struct position *restrict r);

// --- Test Functions ---
static void reset_counts() {
    checks     = 0;
    captures   = 0;
    enpassants = 0;
    castles    = 0;
    promotions = 0;
    checkmates = 0;
}

void test_perft(int argc, char **argv) {
    if (perft_count_test() != 0) {
        fputs("FAILURE!!\n", stderr);
    } else {
        fputs("Success.\n", stdout);
    }    
}

void test_deep_perft(int argc, char **argv) {
    const int max_depth = argc == 2 ? 8 : atoi(argv[2]);
    int depth;
    uint64_t nodes;
    struct position pos;
    struct timespec start, end;
    uint64_t nanos;

    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return;
    }    

    for (depth = 0; depth < max_depth; ++depth) {
        reset_counts();
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        nodes = perft(depth, &pos, 0, EMPTY);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        nanos = (((end.tv_sec * 1000000000ull) + end.tv_nsec) -
                 ((start.tv_sec   * 1000000000ull) + start.tv_nsec));
        
        printf("Perft(%u): Nodes(%" PRIu64 ") Captures(%" PRIu64 ") "
               "E.p.(%" PRIu64 ") Castles(%" PRIu64 ") "
               "Promotions(%" PRIu64 ") Checks(%" PRIu64 ") "
               "Checkmates(%" PRIu64 "). Nanos = %" PRIu64 "\n",
               depth, nodes, captures, enpassants, castles, promotions,
               checks, checkmates, nanos);
    }
}

static int perft_count_test() {
    int ret;
    
    const char *starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    #define start_position_depth 6
    const struct expected_t start_position_expected[start_position_depth] = {
        { .nodes=1        , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=20       , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=400      , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=8902     , .captures=34     , .enpassants=0   , .castles=0, .promotions=0, .checks=12    , .checkmates=0 },
        { .nodes=197281   , .captures=1576   , .enpassants=0   , .castles=0, .promotions=0, .checks=469   , .checkmates=0 },
        { .nodes=4865609  , .captures=82719  , .enpassants=258 , .castles=0, .promotions=0, .checks=27351 , .checkmates=0 },
        /* { .nodes=119060324, .captures=2812008, .enpassants=5248, .castles=0, .promotions=0, .checks=809099, .checkmates=0 },         */
    };
    
    if ((ret = perft_count_test_ex("starting position", starting_position, &start_position_expected[0], start_position_depth)) != 0) {
        return ret;
    }

    const char *kiwi_pete_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    #define kiwi_depth 5
    const struct expected_t kiwi_expected[kiwi_depth] = {
        { .nodes=1, .captures=0, .enpassants=0, .castles=0, .promotions=0, .checks=0, .checkmates=0 },        
        { .nodes=48, .captures=8, .enpassants=0, .castles=2, .promotions=0, .checks=0, .checkmates=0 },
        { .nodes=2039, .captures=351, .enpassants=1, .castles=91, .promotions=0, .checks=3, .checkmates=0 },
        { .nodes=97862, .captures=17102, .enpassants=45, .castles=3162, .promotions=0, .checks=993, .checkmates=0 },
        { .nodes=4085603, .captures=757163, .enpassants=1929, .castles=128013, .promotions=15172, .checks=25523, .checkmates=0 },
        /* { .nodes=193690690, .captures=35043416, .enpassants=73365, .castles=4993637, .promotions=8392, .checks=3309887, .checkmates=0 }, */
    };
    if ((ret = perft_count_test_ex("kiwi pete", kiwi_pete_fen, &kiwi_expected[0], kiwi_depth)) != 0) {
        return ret;
    }

    const char *position3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    #define pos3_depth 5
    const struct expected_t pos3_exp[pos3_depth] = {
        { .nodes=1, .captures=0, .enpassants=0, .castles=0, .promotions=0, .checks=0, .checkmates=0 },
        { .nodes=14, .captures=1, .enpassants=0, .castles=0, .promotions=0, .checks=2, .checkmates=0 },
        { .nodes=191, .captures=14, .enpassants=0, .castles=0, .promotions=0, .checks=10, .checkmates=0 },
        { .nodes=2812, .captures=209, .enpassants=2, .castles=0, .promotions=0, .checks=267, .checkmates=0 },
        { .nodes=43238, .captures=3348, .enpassants=123, .castles=0, .promotions=0, .checks=1680, .checkmates=0 },                
    };
    if ((ret = perft_count_test_ex("position #3", position3, &pos3_exp[0], pos3_depth)) != 0) {
        return ret;
    }

    const char *pos4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -";
    #define pos4_depth 5
    const struct expected_t pos4_exp[pos4_depth] = {
        { .nodes=1, .captures=0, .enpassants=0, .castles=0, .promotions=0, .checks=0, .checkmates=0 },
        { .nodes=6, .captures=0, .enpassants=0, .castles=0, .promotions=0, .checks=0, .checkmates=0 },
        { .nodes=264, .captures=87, .enpassants=0, .castles=6, .promotions=48, .checks=10, .checkmates=0 },
        { .nodes=9467, .captures=1021, .enpassants=4, .castles=0, .promotions=120, .checks=38, .checkmates=0 },
        { .nodes=422333, .captures=131393, .enpassants=0, .castles=7795, .promotions=60032, .checks=15492, .checkmates=0 },                        
    };
    if ((ret = perft_count_test_ex("position #4", pos4, &pos4_exp[0], pos4_depth)) != 0) {
        return ret;
    }

    const char *pos5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -";
    #define pos5_depth 5
    uint64_t pos5_exp[pos5_depth] = {
        1,
        44,
        1486,
        62379,
        2103487
    };
    if ((ret = perft_bulk_test_ex("position #5", pos5, &pos5_exp[0], pos5_depth)) != 0) {
        return ret;
    }

    const char* pos6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -";
    #define pos6_depth 5
    uint64_t pos6_exp[pos6_depth] = {
        1,
        46,
        2079,
        89890,
        3894594
    };
    if ((ret = perft_bulk_test_ex("position #6", pos6, &pos6_exp[0], pos6_depth)) != 0) {
        return ret;
    }    
    
    return 0;
}

static int perft_count_test_ex(const char *name, const char *fen, const struct expected_t *expected, int max_depth) {
    int depth;
    uint64_t nodes;
    struct position pos;
    const struct expected_t *e;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }

    printf("Running test for %s\n", name);
    for (depth = 0; depth < max_depth; ++depth) {
        printf("Beginning depth %d...", depth);
        reset_counts();
        nodes = perft(depth, &pos, 0, EMPTY);
        e = &expected[depth];
        if (nodes != e->nodes) {
            printf("Failed nodes!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->nodes, nodes);            
            return 1;
        } else if (captures != e->captures) {
            printf("Failed captures!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->captures, captures);
            return 2;
        } else if (enpassants != e->enpassants) {
            printf("Failed enpassants!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->enpassants, enpassants);            
            return 3;
        } else if (castles != e->castles) {
            printf("Failed castles!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->castles, castles);            
            return 4;
        } else if (promotions != e->promotions) {
            printf("Failed promotions!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->promotions, promotions);            
            return 5;
        } else if (checks != e->checks) {
            printf("Failed checks!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->checks, checks);            
            return 6;
        } else if (checkmates != e->checkmates) {
            printf("Failed checkmates!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->checkmates, checkmates);            
            return 7;
        } else {
            printf("Passed.\n");
        }
    }

    return 0;
}

static int perft_bulk_test_ex(const char *name, const char *fen, const uint64_t *expected, int max_depth) {
    int depth;
    uint64_t nodes;
    struct position pos;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }

    printf("Running test for %s\n", name);
    for (depth = 0; depth < max_depth; ++depth) {
        printf("Beginning depth %d...", depth);
        reset_counts();
        nodes = perft(depth, &pos, 0, EMPTY);
        if (nodes != expected[depth]) {
            printf("Failed nodes!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   expected[depth], nodes);
            return 1;
        } else {
            printf("Passed.\n");
        }
    }

    return 0;
}

static uint64_t perft(int depth, struct position *const restrict pos, move pmove, int cap) {
    uint32_t i;
    uint32_t nmoves;
    uint64_t nodes = 0;
    struct savepos sp;
    move moves[MAX_MOVES];
    #if 0
    struct position tmp;
    #endif
    
    if (in_check(pos, FLIP(pos->wtm))) {
        return 0;
    }
    if (depth == 0) {
        if (pmove != 0) {
            if (in_check(pos, pos->wtm) != 0) {
                ++checks;
            }
            if (cap != EMPTY) {
                ++captures;
            }
            if (FLAGS(pmove) == FLG_CASTLE) {
                ++castles;
            } else if (FLAGS(pmove) == FLG_EP) {
                ++enpassants;
                ++captures; // e.p. don't set the sp->captured_pc flag
            } else if (FLAGS(pmove) == FLG_PROMO) {
                ++promotions;
            }
        }
        return 1;
    }

    #if 0
    memcpy(&tmp, pos, sizeof(tmp));
    #endif
    nmoves = generate_moves(pos, &moves[0]);
    for (i = 0; i < nmoves; ++i) {
        make_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
        nodes += perft(depth - 1, pos, moves[i], sp.captured_pc);
        undo_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
        #if 0
        if (memcmp(&tmp, pos, sizeof(tmp)) != 0) {
            printf("Original position:\n");
            full_position_print(&tmp);
            printf("\n\nAfter undo move:\n");
            full_position_print(pos);
            printf("\nMove:\n");
            move_print(moves[i]);
            assert(0);
        }
        #endif
    }

    return nodes;
}

void test_make_move(int argc, char **argv) {
    if (test_move_creation() != 0) {
        return;
    }

    printf("Running make move tests...\n");
    const char *start_pos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";    
    move start_pos_moves[] = {
        MOVE(A2, A3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(A2, A4, MV_FALSE, MV_FALSE, MV_FALSE),        
        MOVE(B2, B3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B2, B4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(C2, C3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(C2, C4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(D2, D3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(D2, D4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(E2, E3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(E2, E4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(F2, F3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(F2, F4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G2, G3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G2, G4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(H2, H3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(H2, H4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B1, A3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B1, C3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G1, F3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G1, H3, MV_FALSE, MV_FALSE, MV_FALSE),
        0
    };
    if (test_make_move_ex(start_pos_fen, &start_pos_moves[0]) != 0) {
        printf("Failed test for moves from starting position!\n");
        return;
    }

    const char *kiwi_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    move kiwi_moves[] = {
        MOVE(E2, A6, MV_FALSE, MV_FALSE, MV_FALSE), // bishop captures bishop
        MOVE(G2, H3, MV_FALSE, MV_FALSE, MV_FALSE), // pawn captures pawn
        MOVE(D2, G5, MV_FALSE, MV_FALSE, MV_FALSE), // bishop move
        MOVE(E1, G1, MV_FALSE, MV_FALSE, MV_TRUE ), // castle king side
        MOVE(E1, C1, MV_FALSE, MV_FALSE, MV_TRUE ), // castle queen side
        MOVE(F3, H3, MV_FALSE, MV_FALSE, MV_FALSE), // queen captures pawn
        MOVE(H1, F1, MV_FALSE, MV_FALSE, MV_FALSE), // rook move
        MOVE(E1, F1, MV_FALSE, MV_FALSE, MV_FALSE), // king move (not castling)
        0
    };
    if (test_make_move_ex(kiwi_fen, &kiwi_moves[0]) != 0) {
        printf("Failed test for moves from kiwi position!\n");
        return;
    }

    const char *ep_fen = "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6";
    move ep_moves[] = {
        MOVE(E5, D6, MV_FALSE, MV_TRUE, MV_FALSE),
        0
    };
    if (test_make_move_ex(ep_fen, &ep_moves[0]) != 0) {
        printf("Failed test for moves from e.p. position!\n");
        return;
    }

    const char *promo_fen = "8/1Pp5/7r/8/KR1p1p1k/8/4P1P1/8 w - -";
    move promo_moves[] = {
        MOVE(B7, B8, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE), // knight promo
        MOVE(B7, B8, MV_PRM_BISHOP, MV_FALSE, MV_FALSE), // bishop promo
        MOVE(B7, B8, MV_PRM_ROOK  , MV_FALSE, MV_FALSE), // rook promo
        MOVE(B7, B8, MV_PRM_QUEEN , MV_FALSE, MV_FALSE), // queen promo
        0
    };
    if (test_make_move_ex(promo_fen, &promo_moves[0]) != 0) {
        printf("Failed test for moves from promo position!\n");
        return;
    }
    
    printf("Succeeded.\n");
}

void test_undo_move(int argc, char **argv) {
    printf("Running undo move tests...\n");
    const char *start_pos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";    
    move start_pos_moves[] = {
        MOVE(A2, A3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(A2, A4, MV_FALSE, MV_FALSE, MV_FALSE),        
        MOVE(B2, B3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B2, B4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(C2, C3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(C2, C4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(D2, D3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(D2, D4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(E2, E3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(E2, E4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(F2, F3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(F2, F4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G2, G3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G2, G4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(H2, H3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(H2, H4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B1, A3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B1, C3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G1, F3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G1, H3, MV_FALSE, MV_FALSE, MV_FALSE),
        0
    };
    if (test_undo_move_ex(start_pos_fen, &start_pos_moves[0]) != 0) {
        printf("Failed test for moves from starting position!\n");
        return;
    }

    const char *kiwi_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    move kiwi_moves[] = {
        MOVE(E2, A6, MV_FALSE, MV_FALSE, MV_FALSE), // bishop captures bishop
        MOVE(G2, H3, MV_FALSE, MV_FALSE, MV_FALSE), // pawn captures pawn
        MOVE(D2, G5, MV_FALSE, MV_FALSE, MV_FALSE), // bishop move
        MOVE(E1, G1, MV_FALSE, MV_FALSE, MV_TRUE ), // castle king side
        MOVE(E1, C1, MV_FALSE, MV_FALSE, MV_TRUE ), // castle queen side
        MOVE(F3, H3, MV_FALSE, MV_FALSE, MV_FALSE), // queen captures pawn
        MOVE(H1, F1, MV_FALSE, MV_FALSE, MV_FALSE), // rook move
        MOVE(E1, F1, MV_FALSE, MV_FALSE, MV_FALSE), // king move (not castling)
        0
    };
    if (test_undo_move_ex(kiwi_fen, &kiwi_moves[0]) != 0) {
        printf("Failed test for moves from kiwi position!\n");
        return;
    }

    const char *ep_fen = "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6";
    move ep_moves[] = {
        MOVE(E5, D6, MV_FALSE, MV_TRUE, MV_FALSE),
        0
    };
    if (test_undo_move_ex(ep_fen, &ep_moves[0]) != 0) {
        printf("Failed test for moves from e.p. position!\n");
        return;
    }

    const char *promo_fen = "8/1Pp5/7r/8/KR1p1p1k/8/4P1P1/8 w - -";
    move promo_moves[] = {
        MOVE(B7, B8, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE), // knight promo
        MOVE(B7, B8, MV_PRM_BISHOP, MV_FALSE, MV_FALSE), // bishop promo
        MOVE(B7, B8, MV_PRM_ROOK  , MV_FALSE, MV_FALSE), // rook promo
        MOVE(B7, B8, MV_PRM_QUEEN , MV_FALSE, MV_FALSE), // queen promo
        0
    };
    if (test_undo_move_ex(promo_fen, &promo_moves[0]) != 0) {
        printf("Failed test for moves from promo position!\n");
        return;
    }

    printf("Success.\n");
}

static int test_make_move_ex(const char *fen, const move *moves) {
    struct position pos;
    struct position tmp;
    struct savepos sp;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }
    memcpy(&tmp, &pos, sizeof(tmp));

    const move *m = moves;
    while (*m) {
        #ifdef EXTRA_INFO
        printf("Testing: "); move_print(*m);
        #endif
        
        make_move(&pos, *m, &sp);
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

static int test_undo_move_ex(const char *fen, const move *moves) {
    struct position pos;
    struct position tmp;
    struct savepos sp;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }
    memcpy(&tmp, &pos, sizeof(tmp));

    const move *m = moves;
    while (*m) {
        #ifdef EXTRA_INFO
        printf("Testing: "); move_print(*m);
        #endif
        
        make_move(&pos, *m, &sp);
        if (validate_position(&pos) != 0) {
            fputs("Failed to make move!\n", stderr);
            return 1;
        }
        
        // restore pos
        undo_move(&pos, *m, &sp);
        if (validate_position(&pos) != 0) {
            fputs("validate_position failed after calling undo move!\n", stderr);
            return 1;
        }
        if (position_cmp(&pos, &tmp) != 0) {
            fputs("position_cmp failed after undo_move()\n", stderr);
            return 1;            
        }
        if (memcmp(&pos, &tmp, sizeof(tmp)) != 0) {
            fputs("memcmp failed after undo_move()\n", stderr);
            return 1;
        }
        ++m;
    }

    return 0;    
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
        { E2, E4, MV_FALSE     , MV_FALSE, MV_FALSE }, // regular case
        { E6, D5, MV_FALSE     , MV_TRUE , MV_FALSE }, // ep case
        { E8, E7, MV_PRM_QUEEN , MV_FALSE, MV_FALSE }, // prm case - white queen
        { E8, E7, MV_PRM_ROOK  , MV_FALSE, MV_FALSE }, // prm case - white rook
        { E8, E7, MV_PRM_BISHOP, MV_FALSE, MV_FALSE }, // prm case - white bishop
        { E8, E7, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE }, // prm case - white knight
        { E1, E2, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE }, // prm case - black knight
        { G1, E1, MV_FALSE     , MV_FALSE, MV_TRUE  },
        { A1, H8, MV_FALSE     , MV_FALSE, MV_FALSE },
        { A1, B1, MV_FALSE     , MV_FALSE, MV_FALSE }
    };

    printf("Testing move creation...\n");
    for (i = 0; i < (sizeof(tests)/sizeof(tests[0])); ++i) {
        const move mv = MOVE(tests[i].from,
                                     tests[i].to,
                                     tests[i].prm,
                                     tests[i].ep,
                                     tests[i].csl);
        const uint32_t to    = TO(mv);
        const uint32_t from  = FROM(mv);
        const uint32_t prm   = PROMO_PC(mv);
        const uint32_t flags = FLAGS(mv);

        if (to != tests[i].to) {
            printf("to(%u) != tests[i].to(%u)\n", to, tests[i].to);
            ret = 1;
        } else if (from != tests[i].from) {
            printf("from(%u) != tests[i].from(%u)\n", from, tests[i].from);
            ret = 1;
        } else if (tests[i].prm != MV_FALSE && flags != FLG_PROMO) {
            printf("prm != FALSE && flags(%u) != FLG_PROMO\n", flags);
            ret = 1;
        } else if (tests[i].prm != MV_FALSE && prm != tests[i].prm) {
            printf("prm(%u) != tests[i].prm(%u)\n", prm, tests[i].prm);
            ret = 1;
        } else if (tests[i].ep == MV_TRUE && flags != FLG_EP) {
            printf("ep != FALSE && flags(%u) != FLG_EP\n", flags);
            ret = 1;
        } else if (tests[i].csl == MV_TRUE && flags != FLG_CASTLE) {
            printf("csl != FALSE && flags(%u) != FLG_CASTLE\n", flags);
            ret = 1;
        } else {
            ret = 0;
        }

        if (ret != 0) {
            printf("Failed on test case: (frm=%s, to=%s, prm=%u, ep=%u, csl=%u) sm = 0x%04" PRIx32 "\n",
                   sq_to_str[tests[i].from], sq_to_str[tests[i].to], tests[i].prm,
                   tests[i].ep, tests[i].csl, mv);
            return ret;
        }
                
    }
    printf("Success.\n");
    return 0;
}

static int position_cmp(const struct position *restrict l, const struct position *restrict r) {
    int i;
    for (i = PC(WHITE,PAWN); i <= PC(BLACK,KING); ++i) {
        if (l->brd[i] != r->brd[i]) {
            fprintf(stderr, "l->brd[%c] != r->brd[%c] 0x%08" PRIX64 " != 0x%08" PRIX64 "\n",
                    vpcs[i], vpcs[i], l->brd[i], r->brd[i]);
            return 1;
        }
    }

    for (i = 0; i < 64; ++i) {
        if (l->sqtopc[i] != r->sqtopc[i]) {
            fprintf(stderr, "(l->sqtopc[%s]=%c) != (r->sqtopc[%s]=%c)\n",
                    sq_to_str[i], vpcs[l->sqtopc[i]],
                    sq_to_str[i], vpcs[r->sqtopc[i]]);
            return 2;
        }
    }

    if (l->nmoves != r->nmoves) {
        fprintf(stderr, "l->nmoves(%u) != r->nmoves(%u)\n", l->nmoves, r->nmoves);
        return 3;
    }
    if (l->wtm != r->wtm) {
        fprintf(stderr, "l->wtm(%s) != r->wtm(%s)\n", SIDESTR(l->wtm), SIDESTR(r->wtm));
        return 4;
    }
    if (l->halfmoves != r->halfmoves) {
        fprintf(stderr, "l->halfmoves(%u) != r->halfmoves(%u)\n", l->halfmoves, r->halfmoves);
        return 5;
    }
    if (l->castle != r->castle) {
        fprintf(stderr, "l->castle(%u) != r->castle(%u)\n", l->castle, r->castle);
        return 6;
    }
    if (l->enpassant != r->enpassant) {
        fprintf(stderr, "l->enpassant(%u) != r->enpassant(%u)\n", l->enpassant, r->enpassant);
        return 7;
    }

    return 0;
}
