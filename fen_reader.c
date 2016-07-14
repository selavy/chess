#include "fen_reader.h"
#include "types.h"
#include <stdio.h>

// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2

enum state {
    ranks,
    active_color,
    castling,
    en_passant,
    halfmove_count,
    fullmove_count
};

int fen_reader(struct fen_position *pos, const char * restrict fen) {
    enum state state = ranks;
    int rank = 7;
    int col = 0;
    //    char color;
    char c;
    int i;
    int v;
    
    while (*fen) {
        c = *fen++;
        
        switch (state) {
        case ranks:
            if (c == '/') {
                rank--;
                if (rank < 0) {
                    fputs("Too many ranks given", stderr);
                    return 1;
                }
                col = 0;
            } else if (c == ' ') {
                state = active_color;
            } else if (c >= 'a' && c <= 'z') {
                pos->board[SQUARE(col++, rank)] = c;
            } else if (c >= '1' && c <= '8') {
                // TODO: fast path for 8
                v = c - '0';
                if (col + v > 8) {
                    fprintf(stderr, "Invalid number of columns: %d, %d\n", col, v);
                    return 1;
                }
                for (i = 0; i < v; ++i) {
                    pos->board[SQUARE(col++, rank)] = ' ';
                }
            } else if (c >= 'A' && c <= 'Z') {
                pos->board[SQUARE(col++, rank)] = c;
            } else {
                fprintf(stderr, "Invalid character: '%c'\n", c);
                return 1;
            }
            break;
        case active_color:
        case castling:
        case en_passant:
        case halfmove_count:
        case fullmove_count:
            break;
        }
    }
    
    return 0;
}
