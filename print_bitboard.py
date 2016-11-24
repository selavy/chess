#!/usr/bin/env python

from __future__ import print_function
import sys

if __name__ == '__main__':
    if len(sys.argv) == 1:
        print("Usage: {} <bitboard>".format(sys.argv[0]))
        sys.exit(0)

    bbrd = int(sys.argv[1])
    print(bbrd)

    sys.stdout.write('+---+---+---+---+---+---+---+---+\n')
    for rank in range(7, -1, -1):
        for file in range(8):
            sq = rank * 8 + file
            pc = '*' if (bbrd & (1 << sq)) else ' '
            sys.stdout.write('| {} '.format(pc))
        sys.stdout.write('|\n+---+---+---+---+---+---+---+---+\n')

