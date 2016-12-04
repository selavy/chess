#ifndef SEARCH__H_
#define SEARCH__H_

#include <stdlib.h>
#include "move.h"
#include "position.h"
#include "movegen.h"

extern move search(const struct position *restrict const position);

#endif // SEARC__H_
