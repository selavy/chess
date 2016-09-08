#include "read_fen.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "types.h"

int read_fen(struct position * restrict pos, const char * const fen, int print) {
    int row;
    int col;
    const char *p = fen;
    int color;
    char c;

    // init position
    pos->nmoves = 0;
    pos->wtm = WHITE;
    pos->halfmoves = 0;
    pos->castle = 0;
    pos->enpassant = 0;
    for (row = 0; row < 64; ++row) {
        pos->sqtopc[row] = EMPTY;
    }
    memset(&pos->brd[0], 0, sizeof(pos->brd[0]) * NPIECES*2);

    for (row = 7; row >= 0; --row) {
        col = 0;
        do {
            c = *p++;
            if (c == 0) { // end-of-input
                return 1;
            } else if (c >= '1' && c <= '9') {
                col += c - '0';
            } else if (c != '/') {
                if (c >= 'a') {
                    color = BLACK;
                    c += 'A' - 'a';
                } else {
                    color = WHITE;
                }
                switch (c) {
                case 'P':
                    pos->sqtopc[row*8+col] = PC(color,PAWN);
                    PIECES(*pos, color, PAWN) |= MASK(SQ(col,row));
                    break;
                case 'N':
                    pos->sqtopc[row*8+col] = PC(color,KNIGHT);
                    PIECES(*pos, color, KNIGHT) |= MASK(SQ(col,row));                    
                    break;
                case 'B':
                    pos->sqtopc[row*8+col] = PC(color,BISHOP);
                    PIECES(*pos, color, BISHOP) |= MASK(SQ(col,row));                    
                    break;
                case 'R':
                    pos->sqtopc[row*8+col] = PC(color,ROOK);
                    PIECES(*pos, color, ROOK) |= MASK(SQ(col,row));                    
                    break;
                case 'Q':
                    pos->sqtopc[row*8+col] = PC(color,QUEEN);
                    PIECES(*pos, color, QUEEN) |= MASK(SQ(col,row));                    
                    break;
                case 'K':
                    pos->sqtopc[row*8+col] = PC(color,KING);
                    PIECES(*pos, color, KING) |= MASK(SQ(col,row));                    
                    break;
                default:
                    return 1;
                    break;
                }
                ++col;
            }
        } while (col < 8);
    }

    ++p;
    pos->wtm = *p == 'w' ? WHITE : BLACK;
    p++;
    pos->castle = 0;

    ++p;
    while (*p && *p != ' ') {
        switch (*p) {
        case 'K':
            pos->castle |= WKINGSD;
            break;
        case 'Q':
            pos->castle |= WQUEENSD;
            break;
        case 'k':
            pos->castle |= BKINGSD;
            break;
        case 'q':
            pos->castle |= BQUEENSD;
            break;
        case '-':
            break;
        default:
            fprintf(stderr, "Unexpected character: '%c'\n", *p);
            return 1;
        }
        ++p;
    }
    ++p;

    if (!*p) {
        fprintf(stderr, "Unexpected end of input!\n");
        return 1;
    } else if (*p == '-') {
        pos->enpassant = NO_ENPASSANT;
    } else {
        int sq = 0;
        if (*p >= 'a' && *p <= 'h') {
            sq = *p - 'a';
        } else {
            fprintf(stderr, "Unexpected character in enpassant square: '%c'\n", *p);
            return 1;
        }

        ++p;
        if (*p >= '1' && *p <= '8') {
            sq += (*p - '1') * 8;
        } else {
            fprintf(stderr, "Unexpected character in enpassant square: '%c'\n", *p);
            return 1;
        }

        if ((sq >= A3 && sq <= H3) || (sq >= A6 && sq <= H6)) {
            // I store the square the pawn moved to, FEN gives the square behind the pawn
            if (pos->wtm == WHITE) {
                pos->enpassant = sq + 8 - 23;
            } else {
                pos->enpassant = sq - 8 - 23;
            }
        } else {
            fprintf(stderr, "Invalid enpassant square: %d\n", sq);
            return 1;
        }
    }
    ++p;

    if (*p) {
        int halfmoves = 0;
        while (*p && *p != ' ') {
            if (*p >= '0' && *p <= '9') {
                halfmoves *= 10;
                halfmoves += *p - '0';
            } else {
                fprintf(stderr, "Unexpected character in halfmoves: '%c'\n", *p);
                return 1;
            }
        }
        pos->halfmoves = halfmoves;

        ++p;
        int fullmoves = 0;
        while (*p && *p != ' ') {
            if (*p >= '0' && *p <= '9') {
                fullmoves *= 10;
                fullmoves += *p - '0';
            } else {
                fprintf(stderr, "Unexpected character in fullmoves: '%c'\n", *p);
                return 1;
            }
        }
        pos->nmoves = fullmoves;
    }

    if (print) {
        full_position_print(pos);
    }
    assert(validate_position(pos) == 0);

    return 0;
}

