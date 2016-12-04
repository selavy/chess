#include "eval.h"

/*extern*/ int eval(const struct position *restrict const pos) {
    #define number_of_pieces(bb) __builtin_popcountll(bb)    
    int rval = 0;
    
    const int wpawns = number_of_pieces(PIECES(*pos, WHITE, PAWN));
    const int wknights = number_of_pieces(PIECES(*pos, WHITE, KNIGHT));
    const int wbishops = number_of_pieces(PIECES(*pos, WHITE, BISHOP));
    const int wrooks = number_of_pieces(PIECES(*pos, WHITE, ROOK));
    const int wqueens = number_of_pieces(PIECES(*pos, WHITE, QUEEN));
    const int bpawns = number_of_pieces(PIECES(*pos, BLACK, PAWN));
    const int bknights = number_of_pieces(PIECES(*pos, BLACK, KNIGHT));
    const int bbishops = number_of_pieces(PIECES(*pos, BLACK, BISHOP));
    const int brooks = number_of_pieces(PIECES(*pos, BLACK, ROOK));
    const int bqueens = number_of_pieces(PIECES(*pos, BLACK, QUEEN));

    // basic material calculation
    rval += wpawns + (wknights*3) + (wbishops*3) + (wrooks*5) + (wqueens*8);
    rval -= bpawns + (bknights*3) + (bbishops*3) + (brooks*5) + (bqueens*8);

    return rval;
}
