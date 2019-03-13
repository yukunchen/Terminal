import sys
import time

def osc(seq):
    sys.stdout.write('\x1b]{}\x07'.format(seq))

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print('This is the VT Test template.')
    print('Testing Setting the title - should be "Hello World"')
    osc('2;Hello World')
    time.sleep(1)
    osc('1;Hello World')
    time.sleep(1)
    osc('0;Hello World')
    time.sleep(1)
    print('Done')

