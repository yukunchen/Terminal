import sys
import time

def esc(seq):
    sys.stdout.write('\x1b{}'.format(seq))

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print('This is the VT Test template.')
    csi('H')
    csi('2J')
    csi ('?25l')
    # csi('!p') # Soft Reset
    esc('c') # hard reset
    print('Visible?')
    sys.stdin.read(1)
    # time.sleep(1)
