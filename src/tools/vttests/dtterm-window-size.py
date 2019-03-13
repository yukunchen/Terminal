import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print('This is the VT Test template.')
    csi('8;30;80t')
    print('8;30;80t')
    time.sleep(1)
    print('filler')
    # exit(0)
    csi('8;40;80t')
    print('8;40;80t')
    time.sleep(1)
    print('filler')
    # exit(0)
    csi('8;30;120t')
    print('8;30;120t')
    time.sleep(1)
    print('filler')
    csi('8;40;150t')
    print('8;40;150t')
    time.sleep(1)
    print('filler')

    print('8;0;0t')
    time.sleep(2)
    print('filler')

