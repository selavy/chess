#!/usr/bin/env python

import sys

if __name__ == '__main__':
    with open('output') as f:
        for line in f:
            if line.startswith('MOVE('):
                if line[11] != '4':
                    sys.stdout.write(line)
                if line[18] != '3':
                    sys.stdout.write(line)
                c1 = line[10]
                c2 = line[17]

                if c1 == 'A' and c2 != 'B':
                    sys.stdout.write(line)
                elif c1 == 'B' and c2 not in ('A','C'):
                    sys.stdout.write(line)
                elif c1 == 'C' and c2 not in ('B','D'):
                    sys.stdout.write(line)
                elif c1 == 'D' and c2 not in ('C','E'):
                    sys.stdout.write(line)
                elif c1 == 'E' and c2 not in ('D','F'):
                    sys.stdout.write(line)
                elif c1 == 'F' and c2 not in ('E','G'):
                    sys.stdout.write(line)
                elif c1 == 'G' and c2 not in ('F','H'):
                    sys.stdout.write(line)
                elif c1 == 'H' and c2 != 'G':
                    sys.stdout.write(line)

                if line[24:34] != 'BLACK PAWN':
                    sys.stdout.write(line)
                if line[47:57] != 'WHITE PAWN':
                    sys.stdout.write(line)


