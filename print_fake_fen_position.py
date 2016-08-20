#!/usr/bin/env python

from __future__ import print_function
import sys

SQ_TO_STR = [
    "A1", "B1", "C1", "D1", "E1", "F1", "G1", "H1",
    "A2", "B2", "C2", "D2", "E2", "F2", "G2", "H2",
    "A3", "B3", "C3", "D3", "E3", "F3", "G3", "H3",
    "A4", "B4", "C4", "D4", "E4", "F4", "G4", "H4",
    "A5", "B5", "C5", "D5", "E5", "F5", "G5", "H5",
    "A6", "B6", "C6", "D6", "E6", "F6", "G6", "H6",
    "A7", "B7", "C7", "D7", "E7", "F7", "G7", "H7",
    "A8", "B8", "C8", "D8", "E8", "F8", "G8", "H8",
]

def sq_to_num(col, row):
    return row*8 + col

def print_fen_position(f, line):
    f.write("---------------------------------\n") 
    for row in range(8):
        for col in range(8):
            f.write("| {} ".format(line[sq_to_num(col, 7 - row)]))
        f.write("|\n---------------------------------\n")
    f.write(line[65:72])
    
    c = 72
    move1 = 0
    while 1:
        if line[c] == ',':
            break
        else:
            move1 *= 10
            move1 += int(line[c])
        c += 1
    move2 = 0
    c += 1
    while 1:
        if line[c] == ')':
            break
        else:
            move2 *= 10
            move2 += int(line[c])
        c += 1

    f.write('{},{})'.format(SQ_TO_STR[move1], SQ_TO_STR[move2]))
    f.write('\n\n')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: {} <diff file>".format(sys.argv[0]))
        sys.exit(0)

    with open(sys.argv[1]) as f:
        for line in f:
            if line[0] not in ('<', '>'):
                continue
            line = line[2:]
            #print(line)
            print_fen_position(sys.stdout, line)
