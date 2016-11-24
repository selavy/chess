#ifndef GENERAL__H_
#define GENERAL__H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

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
#define CSL(side) ((side) == WHITE ? (WKINGSD|WQUEENSD) : (BKINGSD|BQUEENSD))

// --- Enums ---
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

// --- Global Data ---
extern const char *vpcs;
extern const char *sq_to_str[64];

#endif // GENERAL__H_