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

    csi('2;1H')
    write('line 2 ##########')

    csi('3;1H')
    csi('41m')
    write('##########')

    csi('4;1H')
    csi('43m')
    write('##########')

    csi('5;1H')
    csi('49m')
    write('line 5 ##########')

    csi('3;4r')

    # csi('11;1H')
    # for i in range(0, 20):
    #     csi('4{}m'.format(i%8))
    #     print('scroll line {}'.format(i))
    #     time.sleep(.5)

    csi('r')
    sys.stdout.flush()
    time.sleep(2)
