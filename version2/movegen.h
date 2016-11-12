#ifndef MOVEGEN__H_
#define MOVEGEN__H_

#include <stdint.h>
#include "move.h"
#include "position.h"

extern int generate_legal_moves(const struct position *const restrict pos, move *restrict moves);
/* void generate_evasions(); */
/* void generate_captures(); */

#endif // MOVEGEN__H_
