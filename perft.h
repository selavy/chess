#ifndef PERFT__H_
#define PERFT__H_

#include "move.h"

struct position;

extern uint64_t checks;
extern uint64_t captures;
extern uint64_t enpassants;
extern uint64_t castles;
extern uint64_t promotions;
extern uint64_t checkmates;

extern uint64_t perft(int depth, struct position * const restrict pos, move pmove, int ply);

#endif // PERFT__H_
