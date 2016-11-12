#ifndef POSITION__H_
#define POSITION__H_

#include <stdio.h>
#include <stdint.h>
#include "move.h"

// `nmoves'    - number of full moves, incremented after black's move
// `halfmoves' - number of halfmoves since the last capture or pawn advance (like in FEN)
//               used for 50 move rule
// `enpassant' - is the target square behind the pawn (like in FEN)
//               -1    = no enpassant
//               0..7  = a3..h3
//               8..15 = a6..h6
struct position {
    uint64_t brd[NPIECES*2];
    uint8_t  sqtopc[64];
    uint16_t nmoves;
    uint8_t  wtm;
    uint8_t  halfmoves;
    uint8_t  castle;
    uint8_t  enpassant;
};

struct savepos {
    uint8_t halfmoves;
    uint8_t enpassant;
    uint8_t castle;
    uint8_t was_ep;
    uint8_t captured_pc; // EMPTY if no capture
};

extern int position_from_fen(struct position *restrict pos, const char *fen);
extern void position_print(FILE *os, struct position *restrict pos); 

#endif // POSITION__H_
