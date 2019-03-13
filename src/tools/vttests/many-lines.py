import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print('\
Line 00\n\
Line 01\n\
Line 02\n\
Line 03\n\
Line 04\n\
Line 05\n\
Line 06\n\
Line 07\n\
Line 08\n\
Line 09\n\
Line 10\n\
Line 11\n\
Line 12\n\
Line 13\n\
Line 14\n\
Line 15\n\
Line 16\n\
Line 17\n\
Line 18\n\
Line 19\n\
Line 20\n\
Line 21\n\
Line 22\n\
Line 23\n\
Line 24\n\
Line 25\n\
Line 26\n\
Line 27\n\
Line 28\n\
Line 29\n\
Line 30\n\
Line 31\n\
Line 32\n\
Line 33\n\
Line 34\n\
Line 35\n\
Line 36\n\
Line 37\n\
Line 38\n\
Line 39\n\
')
