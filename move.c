#include "move.h"
#include <stdio.h>
#include <assert.h>
#include "types.h"

const uint16_t _sm_translation[5] = {
    MV_PRM_NONE,
    MV_PRM_KNIGHT-1,
    MV_PRM_BISHOP-1,
    MV_PRM_ROOK-1,
    MV_PRM_QUEEN-1
};

#ifndef NDEBUG
move MOVE(uint32_t from, uint32_t to, uint32_t prm, uint32_t ep, uint32_t csl) {
    assert(ep  == MV_TRUE || ep  == MV_FALSE);
    assert(csl == MV_TRUE || csl == MV_FALSE);
    assert(prm == MV_PRM_NONE   ||
           prm == MV_PRM_KNIGHT ||
           prm == MV_PRM_BISHOP ||
           prm == MV_PRM_ROOK   ||
           prm == MV_PRM_QUEEN);
    assert(ep == MV_FALSE  || (prm == MV_FALSE && csl == MV_FALSE));
    assert(prm == MV_FALSE || (ep  == MV_FALSE && csl == MV_FALSE));
    assert(csl == MV_FALSE || (ep  == MV_FALSE && prm == MV_FALSE));

    #if 0    
    const uint16_t flags = ((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3);    
    printf("to(%u), from(%u), prm(%u), ep(%u), csl(%u), flags(%u)\n",
           to, from, prm, ep, csl, flags);
    const uint16_t tmsk = to << 0;
    const uint16_t fmsk = from << 6;
    const uint16_t pmsk = _sm_translation[prm] << 12;
    const uint16_t gmsk = flags << 14;
    #endif
    
    return _MOVE(from, to, prm, ep, csl);
}
#endif
