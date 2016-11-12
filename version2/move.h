#ifndef MOVE__H_
#define MOVE__H_

#include <stdint.h>

#define WHITE 0
#define BLACK 1
#define COLORSTR(x) ((x) == WHITE ? "WHITE":"BLACK")
#define FLIP(color) ((color)^1)
#define MAX_MOVES 256
#define CSL_NONE   (0)
#define CSL_WQSIDE (1 << 0)
#define CSL_WKSIDE (1 << 1)
#define CSL_BQSIDE (1 << 2)
#define CSL_BKSIDE (1 << 3)
#define CSL_ALL    (CSL_WQSIDE|CSL_WKSIDE|CSL_BQSIDE|CSL_BKSIDE)
#define EP_NONE 16
#define SQUARE(file, rank) (((rank)*8)+(file))
#define MASK(sq) ((uint64_t)1 << (sq))
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};
enum { KNIGHT, BISHOP, ROOK, QUEEN, PAWN, KING, NPIECES, EMPTY=(NPIECES*2) };
#define PIECECOLOR(pc) ((pc) <= KING ? WHITE:BLACK)
#define PIECE(color, type) ((NPIECES*(color))+(type))

// moves are 16 bits
// 6 bits for `to'
// 6 bits for `from'
// 2 bits for `promo' - 0 = KNIGHT, 1 = BISHOP, 2 = ROOK, 3 = QUEEN
// 2 bits for flags - NONE, EP, PROMO, CASTLE
#define _SIMPLEMOVE(from, to) (((to) << 0) | ((from) << 6))
#define _EP_CAPTURE(from, to)				\
    (((to)    <<  0) |					\
     ((from)  <<  6) |					\
     (1 << 14))
#define _PROMOTION(from, to, prm)		        \
    (((to)    <<  0) |					\
     ((from)  <<  6) |					\
     ((prm)   << 12) |					\
     (2 << 14))
#define _CASTLE(from, to)				\
    (((to)    <<  0) |					\
     ((from)  <<  6) |					\
     (3 << 14))

// #define _MOVE(from, to, prm, ep, csl)			
//     (((to)    <<  0) |					
//      ((from)  <<  6) |					
//      ((prm)   << 12) |					
//      (((!!(ep))*1 + (!!(prm))*2 + (!!(csl))*3) << 14))

#define TO(m)       (((m) >>  0) & 0x3f)
#define FROM(m)     (((m) >>  6) & 0x3f)
#define PROMO_PC(m) (((m) >> 12) & 0x03)
#define FLAGS(m)    (((m) >> 14))
#define FLG_NONE   0
#define FLG_EP     1
#define FLG_PROMO  2
#define FLG_CASTLE 3

extern const char *sq_to_str[64];
extern const char *const visual_pcs;
extern const char *const ep_targets[16];

typedef uint16_t move;

#ifndef NDEBUG
extern move SIMPLEMOVE(uint32_t from, uint32_t to);
#else
#define SIMPLEMOVE _SIMPLEMOVE
#define EP_CAPTURE _EP_CAPTURE
#define PROMOTION  _PROMOTION
#define CASTLE     _CASTLE
#endif

extern void move_print(move mv);

#endif // MOVE__H_
