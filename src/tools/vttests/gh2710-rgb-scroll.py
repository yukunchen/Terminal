import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    csi('H')
    csi('2J')

    csi('38;5;137m')
    csi('48;5;230m')
    for i in range(1, 10):
        write('This is line {}'.format(i))
        csi('K')
        # write('          ')
        write('\n')
    csi('0m')

    csi('48;2;100;255;100m')

    csi('3;8r')
    csi('3;H')
    csi('L')
    csi('r')
    csi('15;H')
