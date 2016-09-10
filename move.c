#include "move.h"
#include <stdio.h>
#include <assert.h>
#include "types.h"

#ifndef NDEBUG
smove_t SMALLMOVE(int to, int from, int prm, int ep, int csl) {
    assert(ep  == SM_TRUE || ep  == SM_FALSE);
    assert(csl == SM_TRUE || csl == SM_FALSE);
    assert(prm == SM_PRM_NONE ||
           prm == SM_PRM_KNIGHT ||
           prm == SM_PRM_BISHOP ||
           prm == SM_PRM_ROOK ||
           prm == SM_PRM_QUEEN);
    assert(ep == 0 || (prm == 0 && csl == 0));
    assert(prm == 0 || (ep == 0 && csl == 0));
    assert(csl == 0 || (ep == 0 && prm == 0));

    return _SMALLMOVE(to, from, prm, ep, csl);
}
#endif

void move_print(move m) {
    printf("MOVE(from=%s, to=%s, pc=%c, prm=%d, cap=%c, ep=%s)",
           sq_to_str[FROM(m)], sq_to_str[TO(m)],
           vpcs[PIECE(m)], PROMOTE(m), vpcs[CAPTURE(m)], BOOLSTR(ENPASSANT(m)));
}

void mprnt(move m) {
    printf("%s%s\t", sq_to_small[FROM(m)], sq_to_small[TO(m)]);
}

int is_castle(move m) {
    uint32_t pc = PIECE(m);
    uint32_t from = FROM(m);
    uint32_t to = TO(m);
    if (pc == PC(WHITE,KING)) {
        return from == E1 && (to == C1 || to == G1);
    } else if (pc == PC(BLACK,KING)) {
        return from == E8 && (to == C8 || to == G8);
    } else {
        return 0;
    }
}
