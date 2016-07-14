#ifndef FEN_READER__H_
#define FEN_READER__H_

struct fen_position {
    char board[64];
    char color;
    int enpassant;
    int halfmoves;
    int fullmoves;
};

int fen_reader(struct fen_position *pos, const char * restrict fen);

#endif // FEN_READER__H_
