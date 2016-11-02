#ifndef POSITION__H_
#define POSITION__H_

#include "general.h"
#include "move.h"

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

extern void make_move(struct position *restrict p, move m, struct savepos *restrict sp);
extern void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp);
extern int read_fen(struct position * restrict pos, const char * const fen, int print);
extern void position_print(const uint8_t * const restrict sqtopc, FILE *ostream);
extern void full_position_print(const struct position *p);
extern int validate_position(const struct position * const restrict p);
extern void set_initial_position(struct position * restrict p);

#endif // POSITION__H_
