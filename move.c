#include "move.h"
#include <stdio.h>
#include "types.h"

void move_print(move m) {
    printf("MOVE(from=%s, to=%s, pc=%c, prm=%d, cap=%c, ep=%s)",
           sq_to_str[FROM(m)], sq_to_str[TO(m)],
           vpcs[PIECE(m)], PROMOTE(m), vpcs[CAPTURE(m)], BOOLSTR(ENPASSANT(m)));
}

void mprnt(move m) {
    printf("%s%s\t", sq_to_small[FROM(m)], sq_to_small[TO(m)]);
}
