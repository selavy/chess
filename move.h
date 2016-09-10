#ifndef MOVE__H_
#define MOVE__H_

#include <stdint.h>

// TODO: could do move in 16-bits - definitely makes harder to debug and more implicit knowledge
//     +6 bits for to square
//     +6 bits for from square
//     // determine if promotion from flags
//     +2 bits for promotion piece { KNIGHT=0, BISHOP=1, ROOK=2, QUEEN=3 } 
//     // know capture from sqtopc array, need captured piece in savepos struct
//     +2 bits for { NORMAL=0, EN_PASSANT=1, PROMOTION=2, CASTLE=3 }       

// to        [0..63 ] 6 bits  {A1...H6}
// from      [0..63 ] 6 bits  {A1...H6}
// piece     [0..5*2] 4 bits  { WPAWN...WKING,BPAWN...BKING }
// capture   [0..11 ] 4 bit - 0 = no capture,   { WPAWN...WKING,BPAWN...BKING }
// promote   [0..5  ] 3 bit - 0 = no promotion, { KNIGHT, BISHOP, ROOK, QUEEN }
// enpassant [0..1  ] 1 bit - 0 = no en passant
typedef uint32_t move;
#define MOVE(to, from, pc, cap, prm, ep)        \
    ((((from ) & 0x3f) <<  0) |                 \
     (((to   ) & 0x3f) <<  6) |                 \
     (((pc   ) & 0x0f) << 12) |                 \
     (((prm  ) & 0x07) << 16) |                 \
     (((cap  ) & 0x0f) << 19) |                 \
     (((ep   ) & 0x01) << 23))  
#define FROM(m)      (((m) >>  0) & 0x3f)
#define TO(m)        (((m) >>  6) & 0x3f)
#define PIECE(m)     (((m) >> 12) & 0x0f)
#define PROMOTE(m)   (((m) >> 16) & 0x07)
#define CAPTURE(m)   (((m) >> 19) & 0x0f)
#define ENPASSANT(m) (((m) >> 23) & 0x01)

extern void move_print(move m);

extern void mprnt(move m);

extern int is_castle(move m);

#define SM_TRUE       1
#define SM_FALSE      0
#define SM_PRM_NONE   (SM_FALSE)
#define SM_PRM_KNIGHT 1
#define SM_PRM_BISHOP 2
#define SM_PRM_ROOK   3
#define SM_PRM_QUEEN  4
typedef uint16_t smove_t;
// TODO(plesslie): don't actually need to AND off the other bits...
extern const uint16_t _sm_translation[5];
#define _SMALLMOVE(to, from, prm, ep, csl)              \
    (((to)    <<  0) |                                  \
     ((from)  <<  6) |                                  \
     (_sm_translation[prm] << 12) |                     \
     (((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3) << 14))

#ifdef NDEBUG
    #define SMALLMOVE _SMALLMOVE
#else
    smove_t SMALLMOVE(uint32_t to, uint32_t from, uint32_t prm, uint32_t ep, uint32_t csl);
#endif

#define SM_TO(m)       (((m) >>  0) & 0x3f)
#define SM_FROM(m)     (((m) >>  6) & 0x3f)
#define SM_PROMO_PC(m) ((((m) >> 12) & 0x03)+1)
#define SM_FLAGS(m)    (((m) >> 14))

#define SM_NONE   0
#define SM_EP     1
#define SM_PROMO  2
#define SM_CASTLE 3

#endif // MOVE__H_
