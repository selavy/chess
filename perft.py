#!/usr/bin/env python

from __future__ import print_function
import sys
import pprint
from collections import namedtuple

Move = namedtuple('Move', ['pc', 'fromsq', 'tosq', 'cap', 'promo'])
EMPTY = ' '
WPAWN = 'P'
WKNIGHT = 'N'
WBISHOP = 'B'
WROOK = 'R'
WQUEEN = 'Q'
WKING = 'K'
BPAWN = 'p'
BKNIGHT = 'n'
BBISHOP = 'b'
BROOK = 'r'
BQUEEN = 'q'
BKING = 'k'
WHITE_TO_MOVE = True
BLACK_TO_MOVE = False
WHITE_PIECES_PROMO = [WKNIGHT,WBISHOP,WROOK,WQUEEN]
BLACK_PIECES_PROMO = [BKNIGHT,BBISHOP,BROOK,BQUEEN]
def get_promo_piece(wtm):
    return WHITE_PIECES_PROMO if wtm else BLACK_PIECES_PROMO
SQ_TO_STR = [
    "A1", "B1", "C1", "D1", "E1", "F1", "G1", "H1",
    "A2", "B2", "C2", "D2", "E2", "F2", "G2", "H2",
    "A3", "B3", "C3", "D3", "E3", "F3", "G3", "H3",
    "A4", "B4", "C4", "D4", "E4", "F4", "G4", "H4",
    "A5", "B5", "C5", "D5", "E5", "F5", "G5", "H5",
    "A6", "B6", "C6", "D6", "E6", "F6", "G6", "H6",
    "A7", "B7", "C7", "D7", "E7", "F7", "G7", "H7",
    "A8", "B8", "C8", "D8", "E8", "F8", "G8", "H8"
]
def is_white(p):
    return p in (WPAWN,WKNIGHT,WBISHOP,WQUEEN,WKING)
def is_black(p):
    return p in (BPAWN,BKNIGHT,BBISHOP,BQUEEN,BKING)
def is_pawn(p, wtm):
    if wtm:
        return p == WPAWN
    else:
        return p == BPAWN
def is_knight(p, wtm):
    if wtm:
        return p == WKNIGHT
    else:
        return p == BKNIGHT
def is_bishop(p, wtm):
    if wtm:
        return p == WBISHOP
    else:
        return p == BBISHOP
def is_rook(p, wtm):
    if wtm:
        return p == WROOK
    else:
        return p == BROOK
def is_queen(p, wtm):
    if wtm:
        return p == WQUEEN
    else:
        return p == BQUEEN
def is_king(p, wtm):
    if wtm:
        return p == WKING
    else:
        return p == BKING
def is_opponent(p, wtm):
    if wtm:
        return is_black(p)
    else:
        return is_white(p)
def is_second_rank(sq, wtm):
    if wtm:
        return sq >= 8 and sq <= 15
    else:
        return sq >= 48 and sq <= 55
def col_to_num(c):
    return ord(c) - ord('A')
def row_to_num(r):
    return r - 1
def sq_to_num(col, row):
    return row*8 + col
def print_board(f, board):
    f.write("---------------------------------\n") 
    for row in range(8):
        for col in range(8):
            f.write("| {} ".format(board[sq_to_num(col, 7 - row)]))
        f.write("|\n---------------------------------\n")
def set_starting_position(board):
    for i in range(8, 16):
        board[i] = WPAWN
    for i in range(48, 56):
        board[i] = BPAWN
    board[0] = WROOK
    board[1] = WKNIGHT
    board[2] = WBISHOP
    board[3] = WQUEEN
    board[4] = WKING
    board[5] = WBISHOP
    board[6] = WKNIGHT
    board[7] = WROOK
    board[56] = BROOK
    board[57] = BKNIGHT
    board[58] = BBISHOP
    board[59] = BQUEEN
    board[60] = BKING
    board[61] = BBISHOP
    board[62] = BKNIGHT
    board[63] = BROOK

def sq_to_rc(i):
    return (i / 8, i % 8)
    
def generate_moves(board, wtm, ledger):
    moves = []
    pawns = []
    knights = []
    bishops = []
    rooks = []
    queens = []
    kings = []
    for i in range(64):
        if board[i] != EMPTY:
            if is_pawn(board[i], wtm):
                pawns.append(sq_to_rc(i))
            elif is_knight(board[i], wtm):
                knights.append(sq_to_rc(i))
            elif is_bishop(board[i], wtm):
                bishops.append(sq_to_rc(i))
            elif is_rook(board[i], wtm):
                rooks.append(sq_to_rc(i))
            elif is_queen(board[i], wtm):
                queens.append(sq_to_rc(i))
            elif is_king(board[i], wtm):
                kings.append(sq_to_rc(i))

    def create_move(fromsq, tosq):
        moves.append(Move(pc=board[fromsq],
                          fromsq=fromsq,
                          tosq=tosq,
                          cap=board[tosq],
                          promo=EMPTY))

    def create_pawn_moves(fromsq, tosq, do_promos):
        def add_move(promo):
            moves.append(Move(pc=board[fromsq],
                              fromsq=fromsq,
                              tosq=tosq,
                              cap=board[tosq],
                              promo=promo))
        if do_promos:
            for p in get_promo_piece(wtm):
                add_move(p)
        else:
            add_move(EMPTY)
        
    # knight moves
    KNIGHT_OFFS = [(-1,-2),(-2,-1),(-2,1),(-1,2),(1,2),(2,1),(2,-1),(1,-2)]
    for row, col in knights:
        fromsq = sq_to_num(col, row)        
        for off in KNIGHT_OFFS:
            r = row + off[0]
            c = col + off[1]
            if r < 0 or r > 7 or c < 0 or c > 7:
                continue
            tosq = sq_to_num(c, r)
            if board[tosq] == EMPTY or is_opponent(board[tosq], wtm):
                create_move(fromsq, tosq)
                
    # bishop moves
    BISHOP_OFFS = [(-1,-1),(-1,1),(1,-1),(1,1)]
    for row, col in bishops:
        fromsq = sq_to_num(col, row)        
        for off in BISHOP_OFFS:
            for i in range(1, 8):
                r = row + i*off[0]
                c = col + i*off[1]
                if r < 0 or r > 7 or c < 0 or c > 7:
                    break
                tosq = sq_to_num(c, r)
                if board[tosq] == EMPTY:
                    create_move(fromsq, tosq)
                elif is_opponent(board[tosq], wtm):
                    create_move(fromsq, tosq)
                    break
                else: # own piece
                    break

    # rook moves
    ROOK_OFFS = [(1,0),(-1,0),(0,1),(0,-1)]
    for row, col in rooks:
        fromsq = sq_to_num(col, row)        
        for off in ROOK_OFFS:
            for i in range(1, 8):
                r = row + i*off[0]
                c = col + i*off[1]
                if r < 0 or r > 7 or c < 0 or c > 7:
                    break
                tosq = sq_to_num(c, r)
                if board[tosq] == EMPTY:
                    create_move(fromsq, tosq)
                elif is_opponent(board[tosq], wtm):
                    create_move(fromsq, tosq)
                    break
                else: # own piece
                    break
                    
    # queen moves
    QUEEN_OFFS = [(1,0),(-1,0),(0,1),(0,-1),   # Rook offsets
                  (-1,-1),(-1,1),(1,-1),(1,1)] # Bishop offsets
    for row, col in queens:
        fromsq = sq_to_num(col, row)        
        for off in QUEEN_OFFS:
            for i in range(1, 8):
                r = row + i*off[0]
                c = col + i*off[1]
                if r < 0 or r > 7 or c < 0 or c > 7:
                    break
                tosq = sq_to_num(c, r)
                if board[tosq] == EMPTY:
                    create_move(fromsq, tosq)
                elif is_opponent(board[tosq], wtm):
                    create_move(fromsq, tosq)
                    break
                else: # own piece
                    break

    # king moves
    KING_OFFS = [(1,0),(-1,0),(0,1),(0,-1),
                 (-1,-1),(-1,1),(1,-1),(1,1)]
    for row, col in kings:
        fromsq = sq_to_num(col, row)        
        for off in KING_OFFS:
            r = row + off[0]
            c = col + off[1]
            if r < 0 or r > 7 or c < 0 or c > 7:
                continue
            tosq = sq_to_num(c, r)
            if board[tosq] == EMPTY:
                create_move(fromsq, tosq)
            elif is_opponent(board[tosq], wtm):
                create_move(fromsq, tosq)                
                
    # pawn moves
    for row, col in pawns:
        sq = sq_to_num(col, row)
        if wtm:
            do_promotions = row == 6
            if row == 1 and board[sq+8] == EMPTY and board[sq+16] == EMPTY: # forward two
                create_pawn_moves(fromsq=sq, tosq=sq+16, do_promos=False)
            if board[sq+8] == EMPTY: # forward one
                create_pawn_moves(fromsq=sq, tosq=sq+8, do_promos=do_promotions)
            if col != 0 and is_opponent(board[sq+9], wtm): # capture right
                create_pawn_moves(fromsq=sq, tosq=sq+9, do_promos=do_promotions)
            if col != 7 and is_opponent(board[sq+7], wtm): # capture left
                create_pawn_moves(fromsq=sq, tosq=sq+7, do_promos=do_promotions)
        else:
            do_promotions = row == 1
            if row == 6 and board[sq-8] == EMPTY and board[sq-16] == EMPTY: # forward two
                # pawn_moves.append(sq-16)
                create_pawn_moves(fromsq=sq, tosq=sq-16, do_promos=False)
            if board[sq-8] == EMPTY: # forward one
                # pawn_moves.append(sq-8)
                create_pawn_moves(fromsq=sq, tosq=sq-8, do_promos=do_promotions)
            if col != 0 and is_opponent(board[sq-9], wtm): # capture left
                create_pawn_moves(fromsq=sq, tosq=sq-9, do_promos=do_promotions)
            if col != 7 and is_opponent(board[sq-7], wtm): # capture right
                create_pawn_moves(fromsq=sq, tosq=sq-7, do_promos=do_promotions)
                
    return moves

def make_move(board, move):
    board[move.fromsq] = EMPTY
    if move.promo != EMPTY:
        board[move.tosq] = move.pc
    else:
        board[move.tosq] = move.promo

def undo_move(board, move):
    board[move.tosq] = move.cap
    board[move.fromsq] = move.pc
        
def perft(board, wtm, ledger, depth):
    if depth == 0:
        return 1
    moves = generate_moves(board, wtm, ledger)
    if depth == 1:
        return len(moves)

    nodes = 0
    tmpledger = ledger[:] # deep copy
    tmpboard = board[:] # deep copy    
    wtm ^= True # flip whose move it is
    for move in moves:
        tmpledger.append(move)
        make_move(tmpboard, move)
        nodes += perft(tmpboard, wtm, tmpledger, depth - 1)
        undo_move(tmpboard, move)
        tmpledger.pop()

    return nodes

if __name__ == '__main__':
    for i in range(4):
        board = [EMPTY]*64
        set_starting_position(board)
        wtm = True
        ledger = []     
        print("Perft #{}: {}".format(i, perft(board, wtm, ledger, i)))
        
