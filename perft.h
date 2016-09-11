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

extern void reset_counts();

extern uint64_t perft(int depth, struct position * const restrict pos, move pmove);

extern uint64_t perft_bulk(int depth, struct position * const restrict pos);

extern int perft_count_test();

extern void test_perft();

extern uint64_t perft_ex(int depth, struct position *const restrict pos, smove_t pmove, int cap);

extern void test_new_perft();

#endif // PERFT__H_
