#include "move.h"
#include <stdio.h>
#include <assert.h>
#include "types.h"

const uint16_t _sm_translation[5] = {
    SM_PRM_NONE,
    SM_PRM_KNIGHT-1,
    SM_PRM_BISHOP-1,
    SM_PRM_ROOK-1,
    SM_PRM_QUEEN-1
};

#ifndef NDEBUG
smove_t SMALLMOVE(uint32_t from, uint32_t to, uint32_t prm, uint32_t ep, uint32_t csl) {
    assert(ep  == SM_TRUE || ep  == SM_FALSE);
    assert(csl == SM_TRUE || csl == SM_FALSE);
    assert(prm == SM_PRM_NONE   ||
           prm == SM_PRM_KNIGHT ||
           prm == SM_PRM_BISHOP ||
           prm == SM_PRM_ROOK   ||
           prm == SM_PRM_QUEEN);
    assert(ep == SM_FALSE  || (prm == SM_FALSE && csl == SM_FALSE));
    assert(prm == SM_FALSE || (ep  == SM_FALSE && csl == SM_FALSE));
    assert(csl == SM_FALSE || (ep  == SM_FALSE && prm == SM_FALSE));

    #if 0    
    const uint16_t flags = ((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3);    
    printf("to(%u), from(%u), prm(%u), ep(%u), csl(%u), flags(%u)\n",
           to, from, prm, ep, csl, flags);
    const uint16_t tmsk = to << 0;
    const uint16_t fmsk = from << 6;
    const uint16_t pmsk = _sm_translation[prm] << 12;
    const uint16_t gmsk = flags << 14;
    pbin(tmsk);
    pbin(fmsk);
    pbin(pmsk);
    pbin(gmsk);
    #endif
    
    return _SMALLMOVE(from, to, prm, ep, csl);
}
#endif

void smove_print(smove_t mv) {
    const uint32_t to    = SM_TO(mv);
    const uint32_t from  = SM_FROM(mv);
    const uint32_t prm   = SM_PROMO_PC(mv);
    const uint32_t flags = SM_FLAGS(mv);

    printf("from(%s) to(%s)", sq_to_str[from], sq_to_str[to]);
    switch (flags) {
    case SM_EP:
        printf(" e.p.");
        break;
    case SM_PROMO:
        printf(" promo(%c)", vpcs[PROMOPC[prm]]);
        break;
    case SM_CASTLE:
        printf(" castle");
    default:
        break;
    }
    printf("\n");
}
