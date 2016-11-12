#include "move.h"
#include <assert.h>
#include <stdio.h>

const char *sq_to_str[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};
const char *const visual_pcs = "NBRQPKnbrqpk ";
const char *const ep_targets[16] = { "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
				     "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6" };

// #ifndef NDEBUG
// move MOVE(uint32_t from, uint32_t to, uint32_t prm, uint32_t ep, uint32_t csl) {
//     assert(ep  == MV_TRUE || ep  == MV_FALSE);
//     assert(csl == MV_TRUE || csl == MV_FALSE);
//     assert(prm == MV_PRM_NONE   ||
//            prm == MV_PRM_KNIGHT ||
//            prm == MV_PRM_BISHOP ||
//            prm == MV_PRM_ROOK   ||
//            prm == MV_PRM_QUEEN);
//     assert(ep == MV_FALSE  || (prm == MV_FALSE && csl == MV_FALSE));
//     assert(prm == MV_FALSE || (ep  == MV_FALSE && csl == MV_FALSE));
//     assert(csl == MV_FALSE || (ep  == MV_FALSE && prm == MV_FALSE));
// 
//     #if 0    
//     const uint16_t flags = ((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3);    
//     printf("to(%u), from(%u), prm(%u), ep(%u), csl(%u), flags(%u)\n",
//            to, from, prm, ep, csl, flags);
//     const uint16_t tmsk = to << 0;
//     const uint16_t fmsk = from << 6;
//     const uint16_t pmsk = _sm_translation[prm] << 12;
//     const uint16_t gmsk = flags << 14;
//     #endif
//     
//     return _MOVE(from, to, prm, ep, csl);
// }
// #endif

void move_print(move mv) {
    const uint32_t to    = TO(mv);
    const uint32_t from  = FROM(mv);
    const uint32_t prm   = PROMO_PC(mv);
    const uint32_t flags = FLAGS(mv);

    printf("from(%s) to(%s)", sq_to_str[from], sq_to_str[to]);
    switch (flags) {
    case FLG_EP:
        printf(" e.p.");
        break;
    case FLG_PROMO:
        printf(" promo(%c)", visual_pcs[prm]);
        break;
    case FLG_CASTLE:
        printf(" castle");
    default:
        break;
    }
    printf("\n");
}


