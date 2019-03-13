import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    csi('H')
    csi('2J')
    print('This is the VT Test template.')

    csi('9;1H')
    write('line 9 ##########')

    csi('10;1H')
    csi('41m')
    write('##########')

    csi('20;1H')
    csi('43m')
    write('##########')

    csi('21;1H')
    csi('49m')
    write('line 21 ##########')

    csi('10;20r')

    csi('11;1H')
    for i in range(0, 15):
        csi('4{}m'.format(i%8))
        print('scroll line {}'.format(i))
        sys.stdout.flush()
        time.sleep(.25)

    csi('r')

