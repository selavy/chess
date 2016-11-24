#ifndef PERFT__H_
#define PERFT__H_

#include <stdint.h>
#include "move.h"
#include "position.h"

int perft_test(const struct position *restrict pos,
	       int depth,
	       uint64_t *nodes,
	       uint64_t *captures,
	       uint64_t *eps,
	       uint64_t *castles,
	       uint64_t *promos,
	       uint64_t *checks,
	       uint64_t *mates);

#endif // PERFT__H_
