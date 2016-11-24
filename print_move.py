#!/usr/bin/env python

from __future__ import print_function
import sys

sq_to_str = [
    'a1', 'b1', 'c1', 'd1', 'e1', 'f1', 'g1', 'h1',
    'a2', 'b2', 'c2', 'd2', 'e2', 'f2', 'g2', 'h2',
    'a3', 'b3', 'c3', 'd3', 'e3', 'f3', 'g3', 'h3',
    'a4', 'b4', 'c4', 'd4', 'e4', 'f4', 'g4', 'h4',
    'a5', 'b5', 'c5', 'd5', 'e5', 'f5', 'g5', 'h5',
    'a6', 'b6', 'c6', 'd6', 'e6', 'f6', 'g6', 'h6',
    'a7', 'b7', 'c7', 'd7', 'e7', 'f7', 'g7', 'h7',
    'a8', 'b8', 'c8', 'd8', 'e8', 'f8', 'g8', 'h8'
]

def interp_flags(flags):
    if flags == 0:
        return 'NONE'
    elif flags == 1:
        return 'enpassant'
    elif flags == 2:
        return 'promo'
    elif flags == 3:
        return 'castle'
    else:
        return 'unknown'


def interp_promo(flags, pc):
    if flags != 2:
        return 'none'
    visual_pcs = "NBRQPKnbrqpk "
    if pc >= 0 and pc < len(visual_pcs):
        return visual_pcs[pc]
    else:
        return 'unknown'

if __name__ == '__main__':
    if len(sys.argv) == 1:
        print("Usage: {} <move>".format(sys.argv[0]))
        sys.exit(0)

    move = int(sys.argv[1])
    print(move)

    tosq = ((move >>  0) & 0x3f)
    fromsq = ((move >>  6) & 0x3f)
    promopc = ((move >> 12) & 0x03)
    flags = ((move >> 14))

    print("From : {}".format(sq_to_str[fromsq]))
    print("To   : {}".format(sq_to_str[tosq]))    
    print("Flags: {}".format(interp_flags(flags)))
    print("Promo: {}".format(interp_promo(flags, promopc)))
