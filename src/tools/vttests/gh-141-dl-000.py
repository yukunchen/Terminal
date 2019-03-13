import sys
import time # time.sleep is in seconds
def write(s):
    sys.stdout.write(s)

def esc(seq):
    write('\x1b{}'.format(seq))

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def cup(r=0, c=0):
    csi('H') if (r==0 and c==0) else csi('{};{}H'.format(r, c))

def cupxy(x=0, y=0):
    cup(y+1, x+1)

def margins(top=0, bottom=0):
    csi('{};{}r'.format(top, bottom))

def clear_all():
    cupxy(0,0)
    csi('2J')

def sgr(code=0):
    csi('{}m').format(code)

def flush(timeout=0):
    sys.stdout.flush()
    time.sleep(timeout)

if __name__ == '__main__':
    clear_all()
    # print('This is the VT Test template.')
    for i in range(1,9):
        print('line {}'.format(i))

    # margins(0, 4)
    csi('4r')
    cup()
    flush(1)
    csi('M')
    flush(1)
    margins()
