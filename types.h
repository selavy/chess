#ifndef TYPES__H_
#define TYPES__H_

#include <stdio.h>
#include <inttypes.h>
#include "magic_tables.h"

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
#define CSL(side) ((side) == WHITE ? (WKINGSD|WQUEENSD) : (BKINGSD|BQUEENSD))
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

extern const char *vpcs;
extern const char *sq_to_str[64];
extern const char *sq_to_small[64];
extern const uint32_t PROMOPC[5];
extern void position_print(const uint8_t * const restrict sqtopc, FILE *ostream);
extern void full_position_print(const struct position *p);
extern int validate_position(const struct position * const restrict p);
extern void set_initial_position(struct position * restrict p);

// TODO: delete
extern int position_cmp(const struct position *restrict l, const struct position *restrict r);
extern void pbbrd(uint64_t bb);

#endif // TYPES__H_
