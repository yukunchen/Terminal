import sys
import time
# $ python ~/vttests/osc-colors.py
def osc(seq):
    sys.stdout.write('\x1b]{}\x07'.format(seq))

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

def print_sample():
    for i in range(31, 37):
        csi('{}m'.format(i))
        print 'new text'

if __name__ == '__main__':
    print('This is the VT Test template.')

    osc('4;1;rgb:ff/00/ff')
    print_sample()

    osc('4;2;rgb:ff/00/ff')
    print_sample()

    osc('4;3;rgb:ff/00/ff')
    print_sample()

    osc('4;4;rgb:ff/00/ff')
    # csi('31m')
    # print 'MAGENTA'

