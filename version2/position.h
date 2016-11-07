#ifndef POSITION__H_
#define POSITION__H_

#include <stdio.h>
#include <stdint.h>

#define WHITE 0
#define BLACK 1
#define FLIP(color) ((color)^1)
enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NPIECES, EMPTY=(NPIECES*2) };
#define PIECE(color, type) ((NPIECES*(color))+(type))
#define CSL_NONE   (0)
#define CSL_WQSIDE (1 << 0)
#define CSL_WKSIDE (1 << 1)
#define CSL_BQSIDE (1 << 2)
#define CSL_BKSIDE (1 << 3)
#define CSL_ALL    (CSL_WQSIDE|CSL_WKSIDE|CSL_BQSIDE|CSL_BKSIDE)
#define EP_NONE 16
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
#define SQUARE(file, rank) (((rank)*8)+(file))
#define MASK(sq) ((uint64_t)1 << (sq))
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
const char *EP_TARGETS[] = { "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
			     "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6" };

struct position {
    uint64_t brd[NPIECES*2]; // 8 * 12 = 96B
    uint8_t  sqtopc[64];     // 1 * 64 = 64B
    // number of full moves, incremented after black's move
    uint16_t nmoves;         // 2 *  1 =  2B
    uint8_t  wtm;            // 1 *  1 =  1B
    // number of halfmoves since the last capture or pawn advance (like in FEN)
    // used for 50 move rule
    uint8_t  halfmoves;      // 1 *  1 =  1B
    uint8_t  castle;         // 1 *  1 =  1B
    // enpassant is the target square behind the pawn (like in FEN)
    // -1    = no enpassant
    // 0..7  = a3..h3
    // 8..15 = a6..h6
    uint8_t  enpassant;      // 1 *  1 =  1B
};                           // Total:  166B

struct savepos {
    // number of halfmoves since the last capture or pawn advance (like in FEN)
    // used for 50 move rule
    uint8_t halfmoves;
    // enpassant is the target square behind the pawn (like in FEN)    
    uint8_t enpassant;
    uint8_t castle;
    // was the last move an enpassant
    uint8_t was_ep;
    uint8_t captured_pc; // EMPTY if no capture
};

extern int position_from_fen(struct position *restrict pos, const char *fen);
extern void position_print(FILE *os, struct position *restrict pos); 

#endif // POSITION__H_
