#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <assert.h>

enum { NO_CASTLE, CASTLE_KSIDE, CASTLE_QSIDE };
const char *CASTLESTR[] = {
    "NO CASTLE",
    "CASTLE KINGSIDE",
    "CASTLE QUEENSIDE"
};
enum { UNKNOWN, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
const char *PIECESTR[] = {
    "UNKNOWN",
    "PAWN",
    "KNIGHT",
    "BISHOP",
    "ROOK",
    "QUEEN",
    "KING"
};
enum { NO_CHECK, IS_CHECK };
enum { NO_CAPTURE, IS_CAPTURE };
const char *CAPTURESTR[] = {
    "NO CAPTURE",
    "CAPTURE"
};
struct move_t {
    int castle;
    int capture;
    int piece;
    int tosq;
    int fromrank;
    int fromcol;
    int check;
};
#define NO_SQ -1
const char *COLSTR[] = { "a", "b", "c", "d", "e", "f", "g", "h" };
const char *RANKSTR[] = { "1", "2", "3", "4", "5", "6", "7", "8" };
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
const char *sq_to_str[64] = {
    "A1", "B1", "C1", "D1", "E1", "F1", "G1", "H1",
    "A2", "B2", "C2", "D2", "E2", "F2", "G2", "H2",
    "A3", "B3", "C3", "D3", "E3", "F3", "G3", "H3",
    "A4", "B4", "C4", "D4", "E4", "F4", "G4", "H4",
    "A5", "B5", "C5", "D5", "E5", "F5", "G5", "H5",
    "A6", "B6", "C6", "D6", "E6", "F6", "G6", "H6",
    "A7", "B7", "C7", "D7", "E7", "F7", "G7", "H7",
    "A8", "B8", "C8", "D8", "E8", "F8", "G8", "H8"
};

void move_print(const struct move_t *m) {
    printf("MOVE: ");
    if (m->castle != NO_CASTLE) {
        printf("%s", CASTLESTR[m->castle]);
    } else {
        printf(" piece: %s,", PIECESTR[m->piece]);
        if (m->capture != NO_CAPTURE) {
            printf(" capture: %s,", CAPTURESTR[m->capture]);
        }
        if (m->fromrank != -1) {
            printf(" fromrank: %d,", m->fromrank + 1);
        }
        if (m->fromcol != -1) {
            printf(" fromcol: %s,", COLSTR[m->fromcol]);
        }
        printf(" tosq: %s,", sq_to_str[m->tosq]);
        if (m->check != NO_CHECK) {
            printf(" IS CHECK!");
        }
    }
    printf("\n");    
}

enum state_t {
    PC_OR_CSL,
    PC,
    CSL,
    SQ_OR_CAP_OR_FROM,
    CAPTURE,
    SQ_OR_CAP,
    MAYBE_FROM_COL,
    COLUMN,
    RANK,
    MAYBE_CHECK
};
const char *STATESTR[] = {
    "PC_OR_CSL",
    "CSL",    
    "PC",
    "SQ_OR_CAP_OR_FROM",
    "CAPTURE",    
    "SQ_OR_CAP",
    "MAYBE_FROM_COL",    
    "COLUMN",
    "RANK",
    "MAYBE_CHECK"
};
int parse(const char * const begin, const char * const end, struct move_t *m) {
    const char * cur = begin;
    int state = PC_OR_CSL;
    char c;
    int col;
    int rank;
    int fromcol = -1;
    int fromrank = -1;
    m->capture = NO_CAPTURE;
    m->check = NO_CHECK;

    do {
        c = *cur;

        if (state == PC_OR_CSL) {
            if (c == 'O' || c == '0') {
                state = CSL;
            } else {
                state = PC;
            }
        } else if (state == CSL) { // TERMINAL STATE
            if ((cur + 4) < end) {
                if (strncmp(cur, "0-0-0", 5) != 0 && strncmp(cur, "O-O-O", 5) != 0) {
                    fprintf(stderr, "Invalid castle string\n");
                    return 1;
                }
                m->castle = CASTLE_QSIDE;
            } else {
                if (strncmp(cur, "0-0", 3) != 0 && strncmp(cur, "O-O", 3) != 0) {
                    fprintf(stderr, "Invalid castle string\n");
                    return 1;
                }
                m->castle = CASTLE_KSIDE;
            }
            break;            
        } else if (state == PC) {
            m->castle = NO_CASTLE;
            switch (c) {
            case 'K': m->piece = KING;   ++cur; break;
            case 'Q': m->piece = QUEEN;  ++cur; break;
            case 'R': m->piece = ROOK;   ++cur; break;
            case 'B': m->piece = BISHOP; ++cur; break;
            case 'N': m->piece = KNIGHT; ++cur; break;
            default:  m->piece = PAWN;   break; // no 'P' so don't advance cur
            }
            state = SQ_OR_CAP_OR_FROM;
        } else if (state == SQ_OR_CAP_OR_FROM) {
            if (c == 'x') {
                state = CAPTURE;
            } else if (c >= '1' && c <= '8') {
                fromrank = c - '1';
                state = SQ_OR_CAP;
                ++cur;
            } else if (c >= 'a' && c <= 'h') {
                col = c - 'a';
                state = MAYBE_FROM_COL;
                ++cur;                
            } else {
                fprintf(stderr, "Invalid character: %c\n", c);
                return 1;
            }
        } else if (state == CAPTURE) {
            assert(c == 'x');
            m->capture = IS_CAPTURE;
            state = COLUMN;
            ++cur;
        } else if (state == SQ_OR_CAP) {
            if (c == 'x') {
                state = CAPTURE;
            } else if (c >= 'a' && c <= 'h') {
                state = COLUMN;
            } else {
                fprintf(stderr, "Invalid character: %c\n", c);
                return 1;
            }
        } else if (state == MAYBE_FROM_COL) {
            if (c == 'x') {
                fromcol = col;
                //DEBUG
                col = -1;
                //GUBED
                state = CAPTURE;
            } else if (c >= '1' && c <= '8') {
                state = RANK;
            } else if (c >= 'a' && c <= 'h') {
                fromcol = col;
                //DEBUG
                col = -1;
                //GUBED
                state = COLUMN;
            } else {
                fprintf(stderr, "Invalid character for column: %c\n", c);
                return 1;
            }
        } else if (state == COLUMN) {
            if (c >= 'a' && c <= 'h') {
                col = c - 'a';
                state = RANK;
                ++cur;
            } else {
                fprintf(stderr, "Invalid character for column: %c\n", c);
                return 1;
            }
        } else if (state == RANK) {
            if (c >= '1' && c <= '8') {
                rank = c - '1';
                ++cur;
                state = MAYBE_CHECK;
            } else {
                fprintf(stderr, "Invalid character for rank: %c\n", c);
                return 1;
            }
        } else if (state == MAYBE_CHECK) { // TERMINAL STATE
            if (c == '+') {
                m->check = IS_CHECK;
            }
            break;
        } else {
            fprintf(stderr, "Invalid state = %d\n", state);
            return 1;
        }
        
    } while (cur < end);

    m->fromrank = fromrank;
    m->fromcol = fromcol;
    m->tosq = (rank * 8) + col;

    return 0;
}

int main(int argc, char **argv) {
    #if 0
    const char *inputs[] = {
        "0-0",
        "0-0-0",
        "Qe4",
        "Ra4",
        "Bh6",
        "Ke8",
        "Bxh8",
        "Nc3",
        "Ngf3",
        "Ngxf3",
        "N1e2",
        "N1xe2"
    };
    const size_t ninputs = sizeof(inputs) / sizeof(inputs[0]);
    size_t i;
    size_t len;
    struct move_t move;

    for (i = 0; i < ninputs; ++i) {
        len = strlen(inputs[i]);
        printf("Input '%s'\n", inputs[i]);
        parse(inputs[i], inputs[i]+len, &move);
        move_print(&move);
    }
    #endif
    FILE *fp;
    char *line = 0;
    size_t len = 0;
    ssize_t read;
    const char *begin;
    const char *end;
    struct move_t move;
    
    if (argc < 2) {
        fputs("Usage: ./pgn_parser <file>\n", stderr);
        exit(EXIT_SUCCESS);
    }
    fp = fopen(argv[1], "r");    
    if (!fp) {
        fputs("Unable to open input file!", stderr);
        exit(EXIT_SUCCESS);
    }
    
    while ((read = getline(&line, &len, fp)) > 0) {
        if (line[0] == '[') { // meta data
            continue;
        }
        printf("Found line: %s", line);
        // assume that moves will never wrap across lines...
        if (*line == '\n') continue;

        
        begin = line;
        do {
            end = begin;
            while (*end && *end != ' ' && *end != '\n') ++end;
            if (*end == 0) {
                fprintf(stderr, "Invalid line!!! '%s'\n", line);
                abort();
            }

            while (*begin >= '0' && *begin <= '9') {
                ++begin;
            }
            if (*begin == '.') {
                ++begin;
            }
            if (*begin == '-') { // end of game
                goto finished;
            }
            printf("Found move: '%.*s'\n",(int) (end-begin), begin);
            parse(begin, end, &move);
            move_print(&move);
            begin = end + 1;
        } while (*end != '\n');
    }

 finished:
    free(line);
    fclose(fp);

    return 0;
}
