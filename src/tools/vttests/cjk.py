# -*- coding: utf-8 -*-
import sys
import time

def csi(seq):
    sys.stdout.write(u'\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print(u'This is the VT Test template.')

    write(u'12')
    csi(u'1D')
    write(u'X')
    print('\n')

    write(u'縱')
    csi(u'1D')
    write(u'X')
    print('\n')

    write(u'縱')
    csi(u'2D')
    write(u'X')
    print('\n')


