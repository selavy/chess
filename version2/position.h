#ifndef POSITION__H_
#define POSITION__H_

#include <stdio.h>
#include <stdint.h>

#define WHITE 0
#define BLACK 1
enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NPIECES };

struct position {
    uint64_t brd[NPIECES*2]; // 8 * 12 = 96B
    uint8_t  sqtopc[64];     // 1 * 64 = 64B
    uint16_t nmoves;         // 2 *  1 =  2B
    uint8_t  wtm;            // 1 *  1 =  1B
    uint8_t  halfmoves;      // 1 *  1 =  1B
    uint8_t  castle;         // 1 *  1 =  1B
    uint8_t  enpassant;      // 1 *  1 =  1B
};                           // Total:  164B

struct savepos {
    uint8_t halfmoves;
    uint8_t enpassant;
    uint8_t castle;
    uint8_t was_ep;
    uint8_t captured_pc; // EMPTY if no capture
};

extern int read_fen(struct position *restrict pos, const char *fen);
extern void position_print(FILE *ostream, struct position *restrict pos); 

#endif // POSITION__H_
