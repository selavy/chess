#define _GNU_SOURCE
#include "plchess.h"
#include <inttypes.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "magic_tables.h"

// --- Macros ---
#define BOOLSTR(x) ((x)?"TRUE":"FALSE")
#define FLIP(side) ((side)^1)
#define COLS 8
#define RANKS 8
#define SQUARES 64
#define MAX_MOVES 256
#define MASK(x) ((uint64_t)1 << x)
#define SQ(c,r) ((r)*COLS+(c))
#define FORWARD_ONE_RANK(wtm, sq) ((wtm) == WHITE ? (sq) + 8 : (sq) - 8)
#define BACKWARD_ONE_RANK(wtm, sq) ((wtm) == WHITE ? (sq) - 8 : (sq) + 8)
#define pawn_attacks(side, sq) ((side) == WHITE ? wpawn_attacks[sq] : bpawn_attacks[sq])
#define PC(side, type) (((side)*NPIECES)+(type))
#define PIECES(p, side, type) (p).brd[PC(side, type)]
#define WHITE_ENPASSANT_SQUARES 0x00000000ff000000
#define BLACK_ENPASSANT_SQUARES 0x000000ff00000000
#define EP_SQUARES(side) ((side)==WHITE ? WHITE_ENPASSANT_SQUARES : BLACK_ENPASSANT_SQUARES)
#define SECOND_RANK 0xff00ull
#define SEVENTH_RANK 0xff000000000000ull
#define RANK7(side) ((side) == WHITE ? SEVENTH_RANK : SECOND_RANK)
#define A_FILE 0x101010101010101ULL
#define H_FILE   0x8080808080808080ULL
#define RANK2(side) ((side) == WHITE ? SECOND_RANK : SEVENTH_RANK)
#define THIRD_RANK 0xff0000ULL
#define SIXTH_RANK 0xff0000000000ULL
#define RANK3(side) ((side) == WHITE ? THIRD_RANK : SIXTH_RANK)
#define NO_PROMOTION 0
#define NO_CAPTURE EMPTY
#define NO_ENPASSANT 0
#define FULLSIDE(b, s) ((b).brd[(s)*NPIECES+PAWN]|(b).brd[(s)*NPIECES+KNIGHT]|(b).brd[(s)*NPIECES+BISHOP]|(b).brd[(s)*NPIECES+ROOK]|(b).brd[(s)*NPIECES+QUEEN]|(b).brd[(s)*NPIECES+KING])
#define SIDESTR(side) ((side)==WHITE?"WHITE":"BLACK")
#define MV_TRUE       1
#define MV_FALSE      0
#define MV_PRM_NONE   (MV_FALSE)
#define MV_PRM_KNIGHT 1
#define MV_PRM_BISHOP 2
#define MV_PRM_ROOK   3
#define MV_PRM_QUEEN  4
#define SIMPLEMOVE(from, to) (((to) << 0) | ((from) << 6))
#define _MOVE(from, to, prm, ep, csl)              \
    (((to)    <<  0) |                                  \
     ((from)  <<  6) |                                  \
     (_sm_translation[prm] << 12) |                     \
     (((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3) << 14))
#define TO(m)       (((m) >>  0) & 0x3f)
#define FROM(m)     (((m) >>  6) & 0x3f)
#define PROMO_PC(m) ((((m) >> 12) & 0x03)+1)
#define FLAGS(m)    (((m) >> 14))
#define FLG_NONE   0
#define FLG_EP     1
#define FLG_PROMO  2
#define FLG_CASTLE 3
#define CSL(side) ((side) == WHITE ? (WKINGSD|WQUEENSD) : (BKINGSD|BQUEENSD))

/* // --- Enums --- */
enum {
    WHITE=0, BLACK
};

enum {
    PAWN=0, KNIGHT, BISHOP, ROOK, QUEEN, KING, NPIECES, EMPTY=(NPIECES*2)
};

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

enum {
    WKINGSD  = (1<<0), WQUEENSD = (1<<1),
    BKINGSD  = (1<<2), BQUEENSD = (1<<3),
};

enum xboard_state { IDLE, SETUP, PLAYING };

// --- Types ---
typedef uint16_t move;

struct xboard_settings {
    int state;
    FILE *log;
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
    uint8_t was_ep;
    uint8_t captured_pc; // EMPTY if no capture
};

// --- Global Data ---
static const uint16_t _sm_translation[5] = {
    MV_PRM_NONE,
    MV_PRM_KNIGHT-1,
    MV_PRM_BISHOP-1,
    MV_PRM_ROOK-1,
    MV_PRM_QUEEN-1
};

const char *vpcs = "PNBRQKpnbrqk ";

const char *sq_to_str[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

const uint32_t PROMOPC[5] = { 0, KNIGHT, BISHOP, ROOK, QUEEN };

// --- Static Function Prototypes ---
static uint32_t gen_legal_moves(const struct position *const restrict pos, move *restrict moves);
static void make_move(struct position *restrict p, move m, struct savepos *restrict sp);
static void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp);
static uint32_t generate_moves(const struct position *const restrict pos, move *restrict moves);
static int read_fen(struct position * restrict pos, const char * const fen, int print);
static int attacks(const struct position * const restrict pos, uint8_t side, int square);
static int in_check(const struct position * const restrict pos, uint8_t side);
static int xboard_settings_init(struct xboard_settings *settings, const char *log);
static int xboard_settings_finalize(struct xboard_settings *settings);
static int handle_xboard_input(const char * const line, size_t bytes, struct xboard_settings *settings);
static void sighandler(int signum);
static void move_print(move mv);
static const char * xboard_move_print(move mv);
#ifdef NDEBUG
    #define MOVE _MOVE
#else
    static move MOVE(uint32_t from, uint32_t to, uint32_t prm, uint32_t ep, uint32_t csl);
#endif
static void position_print(const uint8_t * const restrict sqtopc, FILE *ostream);
static void full_position_print(const struct position *p);
static int validate_position(const struct position * const restrict p);
static void set_initial_position(struct position * restrict p);

// --- Interface Functions ---




void xboard_main() {
    FILE *istream;
    char *line = 0;
    size_t len = 0;
    ssize_t read = 0;
    struct xboard_settings settings;

    // TODO(plesslie): until I implement actual move selection algo
    srand(0);

    // xboard sends SIGINT when the opponent moves - can use to wake
    // up from pondering once that is implemented
    signal(SIGINT, &sighandler);

    if (xboard_settings_init(&settings, "/tmp/output.txt") != 0) {
	fprintf(stderr, "Failed to initialize xboard settings...\n");
	exit(EXIT_FAILURE);
    }

    istream = fdopen(STDIN_FILENO, "rb");
    if (istream == 0) {
	exit(EXIT_FAILURE);
    }
    // turn off i/o buffering
    setbuf(stdout , NULL);
    setbuf(istream, NULL);

    fprintf(settings.log, "Starting up myengine...\n");
    while ((read = getline(&line, &len, istream)) > 0) {
        line[read-1] = 0;
        if (handle_xboard_input(line, read-1, &settings) != 0) {
            break;
        }
    }

    printf("Bye.\n");
    free(line);
    fclose(istream);

    if (xboard_settings_finalize(&settings) != 0) {
	fprintf(stderr, "Failed to finalize xboard settings...\n");
    }
}

// --- Static Function Definitions ---
static uint32_t gen_legal_moves(const struct position *const restrict pos, move *restrict moves) {
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

static void make_move(struct position *restrict p, move m, struct savepos *restrict sp) {
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

static void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp) {
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

static uint32_t generate_moves(const struct position *const restrict pos, move *restrict moves) {
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

// returns 1 if a piece from `side` attacks `square`
static int attacks(const struct position * const restrict pos, uint8_t side, int square) {
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

static int in_check(const struct position * const restrict pos, uint8_t side) {
    // find `side`'s king
    uint64_t kings = pos->brd[PC(side,KING)];
    int kingloc = 0;
    assert(kings != 0); // there should be a king...
    for (; (kings & ((uint64_t)1 << kingloc)) == 0; ++kingloc);
    // check if the other side attacks the king location
    return attacks(pos, FLIP(side), kingloc);
}

static int xboard_settings_init(struct xboard_settings *settings, const char *log) {
    settings->state = IDLE;
    if (log == 0) {
	log = "/tmp/output.txt";
    }
    settings->log = fopen(log, "w");
    if (!settings->log) {
	return 1;
    }
    setbuf(settings->log, NULL);
    return 0;
}

static int xboard_settings_finalize(struct xboard_settings *settings) {
    if (settings->log) {
	fclose(settings->log);
    }
    return 0;
}

#define XSTRNCMP(X,Y) strncmp(X, Y, strlen(Y))
static int handle_xboard_input(const char * const line, size_t bytes, struct xboard_settings *settings) {
    static struct position position;
    static move moves[MAX_MOVES];
    static struct savepos sp;

    fprintf(settings->log, "Received command: '%s'\n", line);
    if (settings->state == IDLE) {
	// TODO:
	settings->state = SETUP;
    }
    if (settings->state == SETUP) {
        if (strcmp(line, "xboard") == 0) {
	    set_initial_position(&position);
        } else if (XSTRNCMP(line, "protover") == 0) {
            if (line[9] != '2') {
                fprintf(stderr, "Unrecognized protocol version:\n");
                return 1;
            }
            printf("feature myname=\"experiment\"\n");
            printf("feature reuse=0\n");
            printf("feature analyze=0\n");
	    printf("feature time=0\n");
            printf("feature done=1\n");
        } else if (strcmp(line, "new") == 0) {
            // no-op
        } else if (strcmp(line, "random") == 0) {
            // no-op
        } else if (XSTRNCMP(line, "level") == 0) {
            // TODO(plesslie): parse time control
        } else if (strcmp(line, "post") == 0) {
            // TODO(plesslie):
            // turn on thinking/pondering output
            // thinking output should be in the format "ply score time nodes pv"
            // E.g. "9 156 1084 48000 Nf3 Nc6 Nc3 Nf6" ==> 
            // "9 ply, score=1.56, time = 10.84 seconds, nodes=48000, PV = "Nf3 Nc6 Nc3 Nf6""
        } else if (XSTRNCMP(line, "accepted") == 0) {
            // no-op
        } else if (strcmp(line, "hard") == 0) {
	    settings->state = PLAYING;
        } else if (strcmp(line, "white") == 0) {
            // TODO(plesslie): setup side
        } else if (strcmp(line, "black") == 0) {
            // TODO(plesslie): setup side
        } else if (XSTRNCMP(line, "time") == 0) {
            // no-op
        } else if (XSTRNCMP(line, "otim") == 0) {
            // no-op
        } else if (strcmp(line, "force") == 0) {
            // no-op
            // stop thinking about the current position
        } else {
            printf("Error (unknown command): %.*s\n", (int)bytes, line);
            return 1;
        }
    } else if (settings->state == PLAYING) {
	if (strcmp(line, "go") == 0) {
	    // no-op
	} else if (strcmp(line, "white") == 0) {
	    return 0;
	} else if (bytes == 4 || bytes == 5) { 	// read move from input
	    const uint32_t from = (line[1]-'1')*8 + (line[0]-'a');
	    const uint32_t to   = (line[3]-'1')*8 + (line[2]-'a');
	    fprintf(settings->log, "Parsed move as %u -> %u, %s -> %s\n",
		    to, from, sq_to_str[from], sq_to_str[to]);
	    move m;
	    if (bytes == 4) {
		m = SIMPLEMOVE(from, to);
	    } else { // promotion
		uint32_t prm;
		switch (line[4]) {
		case 'q': prm = MV_PRM_QUEEN ; break;
		case 'n': prm = MV_PRM_KNIGHT; break;
		case 'b': prm = MV_PRM_BISHOP; break;
		case 'r': prm = MV_PRM_ROOK  ; break;
		default:
		    printf("Invalid move: %s\n", line);
		    return 1;
		}
		m = MOVE(from, to, prm, MV_FALSE, MV_FALSE);
	    }
		    
	    fprintf(settings->log, "Position before:\n");
	    position_print(position.sqtopc, settings->log);
	    make_move(&position, m, &sp);
	    fprintf(settings->log, "Position after:\n");
	    position_print(position.sqtopc, settings->log);
	} else {
	    printf("Error (bad move): %s\n", line);
	    return 1;
	}

	const uint32_t nmoves = gen_legal_moves(&position, &moves[0]);
	fprintf(settings->log, "Found %d legal moves\n", nmoves);
	int zz = nmoves < 10 ? nmoves : 10;
	for (int ii = 0; ii < zz; ++ii) {
	    fprintf(settings->log, "\t%s\n", xboard_move_print(moves[ii]));
	}
	if (nmoves == 0) {
	    // TODO(plesslie): send correct end of game state
	    printf("resign\n");
	} else {
	    const uint32_t r = rand() % nmoves;
	    make_move(&position, moves[r], &sp);
	    const char *movestr = xboard_move_print(moves[r]);

	    fprintf(settings->log, "Trying to make move: %s\n", movestr);
	    printf("move %s\n", movestr);
	}
    }
    
    return 0;
}

static void sighandler(int signum) {
    // mask sigint signal until i get pondering
}

static void move_print(move mv) {
    const uint32_t to    = TO(mv);
    const uint32_t from  = FROM(mv);
    const uint32_t prm   = PROMO_PC(mv);
    const uint32_t flags = FLAGS(mv);

    printf("from(%s) to(%s)", sq_to_str[from], sq_to_str[to]);
    switch (flags) {
    case FLG_EP:
        printf(" e.p.");
        break;
    case FLG_PROMO:
        printf(" promo(%c)", vpcs[PROMOPC[prm]]);
        break;
    case FLG_CASTLE:
        printf(" castle");
    default:
        break;
    }
    printf("\n");
}

static const char * xboard_move_print(move mv) {
	// max move length is "e7e8q", most moves are "e7e8"
	static char buffer[6];
	const char *pcs = " nbrq";
	const uint32_t to = TO(mv);
	const uint32_t from = FROM(mv);
	const uint32_t flags = FLAGS(mv);

	sprintf(&buffer[0], "%s%s", sq_to_str[from], sq_to_str[to]);
	if (flags == FLG_PROMO) {
		buffer[5] = pcs[PROMOPC[PROMO_PC(mv)]];
		buffer[6] = 0;
	} else {
		buffer[5] = 0;
	}
	return &buffer[0];
}

#ifndef NDEBUG
static move MOVE(uint32_t from, uint32_t to, uint32_t prm, uint32_t ep, uint32_t csl) {
    assert(ep  == MV_TRUE || ep  == MV_FALSE);
    assert(csl == MV_TRUE || csl == MV_FALSE);
    assert(prm == MV_PRM_NONE   ||
           prm == MV_PRM_KNIGHT ||
           prm == MV_PRM_BISHOP ||
           prm == MV_PRM_ROOK   ||
           prm == MV_PRM_QUEEN);
    assert(ep == MV_FALSE  || (prm == MV_FALSE && csl == MV_FALSE));
    assert(prm == MV_FALSE || (ep  == MV_FALSE && csl == MV_FALSE));
    assert(csl == MV_FALSE || (ep  == MV_FALSE && prm == MV_FALSE));

    #if 0    
    const uint16_t flags = ((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3);    
    printf("to(%u), from(%u), prm(%u), ep(%u), csl(%u), flags(%u)\n",
           to, from, prm, ep, csl, flags);
    const uint16_t tmsk = to << 0;
    const uint16_t fmsk = from << 6;
    const uint16_t pmsk = _sm_translation[prm] << 12;
    const uint16_t gmsk = flags << 14;
    #endif
    
    return _MOVE(from, to, prm, ep, csl);
}
#endif

static void position_print(const uint8_t * const restrict sqtopc, FILE *ostream) {
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

static void full_position_print(const struct position *p) {
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

static int validate_position(const struct position * const restrict p) {
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

static void set_initial_position(struct position * restrict p) {
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
