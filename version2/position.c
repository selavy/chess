#include "position.h"
#include <string.h>
#include <assert.h>
#include <inttypes.h>

int position_from_fen(struct position *restrict pos, const char *fen) {
    int rank;
    int file;
    char c;
    int halfmoves = 0;
    int nmoves = 0;

    pos->nmoves = 1;
    pos->wtm = WHITE;
    pos->halfmoves = 0;
    pos->castle = CSL_NONE;
    pos->enpassant = EP_NONE;
    memset(&pos->sqtopc[0], EMPTY, sizeof(pos->sqtopc[0]) * 64);
    memset(&pos->brd[0], 0, sizeof(pos->brd[0]) * NPIECES * 2);
    pos->side[WHITE] = 0ull;
    pos->side[BLACK] = 0ull;

    // piece placements
    for (rank = RANK_8; rank >= RANK_1; --rank) {
	for (file = FILE_A; file <= FILE_H; ++file) {
	    c = *fen++;
	    switch (c) {
	    case 0:
		return 1;
	    case 'P':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, PAWN);
		pos->brd[PIECE(WHITE, PAWN)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));
		break;		
	    case 'N':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, KNIGHT);
		pos->brd[PIECE(WHITE, KNIGHT)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'B':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, BISHOP);
		pos->brd[PIECE(WHITE, BISHOP)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'R':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, ROOK);
		pos->brd[PIECE(WHITE, ROOK)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'Q':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, QUEEN);
		pos->brd[PIECE(WHITE, QUEEN)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'K':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(WHITE, KING);
		pos->brd[PIECE(WHITE, KING)] |= MASK(SQUARE(file, rank));
		pos->side[WHITE] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'p':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, PAWN);
		pos->brd[PIECE(BLACK, PAWN)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));
		break;
	    case 'n':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, KNIGHT);
		pos->brd[PIECE(BLACK, KNIGHT)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'b':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, BISHOP);
		pos->brd[PIECE(BLACK, BISHOP)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'r':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, ROOK);
		pos->brd[PIECE(BLACK, ROOK)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'q':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, QUEEN);
		pos->brd[PIECE(BLACK, QUEEN)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    case 'k':
		pos->sqtopc[SQUARE(file, rank)] = PIECE(BLACK, KING);
		pos->brd[PIECE(BLACK, KING)] |= MASK(SQUARE(file, rank));
		pos->side[BLACK] |= MASK(SQUARE(file, rank));		
		break;		
	    default:
		if (c >= '1' && c <= '8') {
		    file += c - '0' - 1; // file will get incremented by for loop
		} else {
		    return 2;
		}
	    }
	}
	assert(rank == RANK_1 || *fen == '/');
	assert(rank != RANK_1 || *fen == ' ');
	++fen;
    }

    // active color
    c = *fen++;
    switch (c) {
    case 'w':
	pos->wtm = WHITE;
	break;
    case 'b':
	pos->wtm = BLACK;
	break;
    default:
	return 3;
    }

    if (*fen++ != ' ') {
	return 4;
    }

    // castling availability
    while ((c = *fen++) != ' ') {
	switch (c) {
	case 0:
	    return 5;
	case '-':
	    pos->castle = CSL_NONE;
	    break;
	case 'K':
	    pos->castle |= CSL_WKSIDE;
	    break;
	case 'Q':
	    pos->castle |= CSL_WQSIDE;
	    break;
	case 'k':
	    pos->castle |= CSL_BKSIDE;
	    break;
	case 'q':
	    pos->castle |= CSL_BQSIDE;
	    break;
	default:
	    return 6;
	}
    }

    // en passant
    c = *fen++;
    if (c == 0) {
	return 7;
    } else if (c == '-') {
	pos->enpassant = EP_NONE;
    } else if (c >= 'a' && c <= 'h') {
	file = c - 'a';
	c = *fen++;
	if (c >= '1' && c <= '8') {
	    rank = c - '1';
	    if (rank == RANK_3) {
		pos->enpassant = file;
	    } else if (rank == RANK_6) {
		pos->enpassant = file + 8;
	    } else {
		return 10;
	    }
	} else {
	    return 8;
	}
    } else {
	return 9;
    }

    if (*fen++ != ' ') {
	return 11;
    }

    // half moves
    while ((c = *fen++) != ' ') {
	if (c == 0) {
	    return 12;
	}
	if (c < '0' || c > '9') {
	    return 13;
	}
	halfmoves *= 10;
	halfmoves += c - '0';
    }
    pos->halfmoves = halfmoves;

    // full moves
    while ((c = *fen++)) {
	if (c < '0' || c > '9') {
	    return 14;
	}
	nmoves *= 10;
	nmoves += c - '0';
    }
    pos->nmoves = nmoves;
        
    return 0;
}

void position_print(FILE *os, struct position *restrict pos) {
    fprintf(os, "+---+---+---+---+---+---+---+---+\n");
    for (int rank = RANK_8; rank >= RANK_1; --rank) {
	for (int file = FILE_A; file <= FILE_H; ++file) {
	    fprintf(os, "| %c ", visual_pcs[pos->sqtopc[SQUARE(file, rank)]]);
	}
	fprintf(os, "|\n+---+---+---+---+---+---+---+---+\n");
    }
    fprintf(os, "%s to move\n", pos->wtm == WHITE ? "White" : "Black");
    if ((pos->castle & CSL_WKSIDE) != 0) {
	fprintf(os, "K");
    }
    if ((pos->castle & CSL_WQSIDE) != 0) {
	fprintf(os, "Q");
    }
    if ((pos->castle & CSL_BKSIDE) != 0) {
	fprintf(os, "k");
    }
    if ((pos->castle & CSL_BQSIDE) != 0) {
	fprintf(os, "q");
    }
    fprintf(os, "\n");
    fprintf(os, "Half moves: %d\n", pos->halfmoves);
    fprintf(os, "Full moves: %d\n", pos->nmoves);
    if (pos->enpassant == EP_NONE) {
	fprintf(os, "En Passant target: none\n");
    } else {
	fprintf(os, "En Passant target: %s\n", ep_targets[pos->enpassant]);
    }
}

int validate_position(struct position *restrict const pos) {
    int pc;
    int color;
    int contra;
    int sq;
    int white_kings = 0;
    int black_kings = 0;
    
    for (sq = A1; sq <= H8; ++sq) {
	if (pos->sqtopc[sq] == EMPTY) {
	    if ((pos->side[WHITE] & MASK(sq)) != 0) {
		printf("validate_position: sqtopc empty on %s, but white bit board is not empty\n", sq_to_str[sq]);
		return 1;
	    }
	    if ((pos->side[BLACK] & MASK(sq)) != 0) {
		printf("validate_position: sqtopc empty on %s, but black bit board is not empty\n", sq_to_str[sq]);
		return 2;
	    }
	} else {
	    pc = pos->sqtopc[sq];
	    color = PIECECOLOR(pc);
	    contra = FLIP(color);

	    // check full side bit boards
	    if ((pos->side[color] & MASK(sq)) == 0) {
		// full side board doesn't have this square occupied
		printf("validate_position: sqtopc has %c on %s, but %s bit board is empty\n",
		       visual_pcs[pc], sq_to_str[sq], COLORSTR(color));
		return 3;
	    }
	    if ((pos->side[contra] & MASK(sq)) != 0) {
		// other side's full board has this square occupied
		printf("validate_position: sqtopc has %c on %s, but %s bit board is not empty\n",
		       visual_pcs[pc], sq_to_str[sq], COLORSTR(color));
		printf("%" PRIu64 "\n", pos->side[contra]);
		return 4;
	    }

	    // check piece bit boards
	    if ((pos->brd[pc] & MASK(sq)) == 0) {
		printf("validate_position: sqtoc has %c on %s, but bit board does not\n",
		       visual_pcs[pc], sq_to_str[sq]);
		return 5;
	    }

	    if (pc == PIECE(WHITE, KING)) {
		++white_kings;
	    }
	    if (pc == PIECE(BLACK, KING)) {
		++black_kings;
	    }
	}
    }

    for (pc = PIECE(WHITE, KNIGHT); pc <= PIECE(BLACK, KING); ++pc) {
	for (sq = A1; sq <= H8; ++sq) {
	    if ((pos->brd[pc] & MASK(sq)) != 0) {
		if (pos->sqtopc[sq] != pc) {
		    printf("validate_position: pos->brd[%c] has a piece on %s, sqtopc has %c\n",
			   visual_pcs[pc], sq_to_str[sq], visual_pcs[pos->sqtopc[sq]]);
		    return 6;
		}
	    }
	}
    }

    if (white_kings != 1) {
	printf("validate_position: %d white kings found!\n", white_kings);
	return 7;
    }
    if (black_kings != 1) {
	printf("validate_position: %d black kings found!\n", black_kings);	
	return 8;
    }

    return 0;
}
