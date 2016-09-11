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
typedef uint16_t move;
// TODO(plesslie): make CSLMOVE(), EPMOVE()
extern const uint16_t _sm_translation[5];
#define SIMPLEMOVE(from, to) (((to) << 0) | ((from) << 6))
#define _MOVE(from, to, prm, ep, csl)              \
    (((to)    <<  0) |                                  \
     ((from)  <<  6) |                                  \
     (_sm_translation[prm] << 12) |                     \
     (((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3) << 14))

#ifdef NDEBUG
    #define MOVE _MOVE
#else
    move MOVE(uint32_t from, uint32_t to, uint32_t prm, uint32_t ep, uint32_t csl);
#endif

#define TO(m)       (((m) >>  0) & 0x3f)
#define FROM(m)     (((m) >>  6) & 0x3f)
#define PROMO_PC(m) ((((m) >> 12) & 0x03)+1)
#define FLAGS(m)    (((m) >> 14))

#define FLG_NONE   0
#define FLG_EP     1
#define FLG_PROMO  2
#define FLG_CASTLE 3

void move_print(move mv);

#endif // MOVE__H_
