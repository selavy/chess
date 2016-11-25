#ifndef MOVEGEN__H_
#define MOVEGEN__H_

#include <stdint.h>
#include "move.h"
#include "position.h"

extern int attacks(const struct position *const restrict pos, uint8_t side, int square);
extern int generate_legal_moves(const struct position *const restrict pos, move *restrict moves);
extern int in_check(const struct position * const restrict pos, uint8_t side);
extern int legal_move(const struct position *const restrict pos, move m);
/* void generate_evasions(); */
/* void generate_captures(); */

#endif // MOVEGEN__H_
