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

# $ python ~/vttests/gh-170-cuu-in-margins-000.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    margins(8, 16)
    cup(12, 0)
    csi('99B')
    write('BOTTOM')
    flush(1)
    write('\r')
    do_all_at_once = False
    if do_all_at_once:
        csi('99A')
        flush(.5)
    else:
        for i in range(0, 24):
            csi('A')
            flush(.05)
    write('TOP')
    flush(.25)
    cup(4, 4)
    write('lol peace out margins')
    flush(.25)
    csi('99B')
    write('Oh no I\'m trapped')
    flush(.25)
    for i in range(0, 24):
        esc('M')
        flush(.05)
    flush(1)
    write('reverse is trapped too')
    flush(2)
    margins()
