#ifndef MOVE__H_
#define MOVE__H_

#include <stdint.h>

#define SM_TRUE       1
#define SM_FALSE      0
#define SM_PRM_NONE   (SM_FALSE)
#define SM_PRM_KNIGHT 1
#define SM_PRM_BISHOP 2
#define SM_PRM_ROOK   3
#define SM_PRM_QUEEN  4
typedef uint16_t smove_t;
// TODO(plesslie): make CSLMOVE(), EPMOVE()
extern const uint16_t _sm_translation[5];
#define SIMPLEMOVE(from, to) (((to) << 0) | ((from) << 6))
#define _SMALLMOVE(from, to, prm, ep, csl)              \
    (((to)    <<  0) |                                  \
     ((from)  <<  6) |                                  \
     (_sm_translation[prm] << 12) |                     \
     (((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3) << 14))

#ifdef NDEBUG
    #define SMALLMOVE _SMALLMOVE
#else
    smove_t SMALLMOVE(uint32_t from, uint32_t to, uint32_t prm, uint32_t ep, uint32_t csl);
#endif

#define SM_TO(m)       (((m) >>  0) & 0x3f)
#define SM_FROM(m)     (((m) >>  6) & 0x3f)
#define SM_PROMO_PC(m) ((((m) >> 12) & 0x03)+1)
#define SM_FLAGS(m)    (((m) >> 14))

#define SM_NONE   0
#define SM_EP     1
#define SM_PROMO  2
#define SM_CASTLE 3

void smove_print(smove_t mv);

#endif // MOVE__H_
