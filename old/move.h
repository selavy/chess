#ifndef MOVE__H_
#define MOVE__H_

#include "general.h"

#define MV_TRUE       1
#define MV_FALSE      0
enum { MV_PRM_NONE, MV_PRM_KNIGHT, MV_PRM_BISHOP, MV_PRM_ROOK, MV_PRM_QUEEN };
#define SIMPLEMOVE(from, to) (((to) << 0) | ((from) << 6))
#define _MOVE(from, to, prm, ep, csl)              \
    (((to)    <<  0) |                                  \
     ((from)  <<  6) |                                  \
     (_sm_translation[prm] << 12) |                     \
     (((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3) << 14))
#define TO(m)       (((m) >>  0) & 0x3f)
#define FROM(m)     (((m) >>  6) & 0x3f)
#define PROMO_PC(m) ((((m) >> 12) & 0x03)+1)
#define FLAGS(m)    (((m) >> 14))
#define FLG_NONE   0
#define FLG_EP     1
#define FLG_PROMO  2
#define FLG_CASTLE 3

typedef uint16_t move;

extern const uint16_t _sm_translation[5];
extern const uint32_t PROMOPC[5];

#ifdef NDEBUG
    #define MOVE _MOVE
#else
    extern move MOVE(uint32_t from, uint32_t to, uint32_t prm, uint32_t ep, uint32_t csl);
#endif
extern void move_print(move mv);

#endif // MOVE__H_
