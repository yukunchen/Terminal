import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/repeatChar000.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')

    cup(1, 1)
    write('A')
    write('\n')

    write('B')
    write('\n')

    write('C')
    flush(1)
    cup(1, 2)
    write('D')
    flush(1)
    csi('b')
    cup(2, 2)
    csi('5b')
    flush(1)
    sgr(31)
    write('E')
    flush(.5)
    csi('500b')
    flush(.5)
    csi('5b')
    flush(.5)
    write('foo')
    flush(.5)
    sgr(32) # If you don't SGR here, then 'o' will get repeated. If you do, then it won't get repeated.
    csi('5b')
    cup(4, 1)
    sgr(0)
    write('foo\n')
    csi('5b')
    write('bar\t')
    flush(.25)
    csi('5b')
    write('baz ')
    flush(.25)
    csi('5b')
    write('x')
    write('\r\n')
    write(u'\u2602')
    csi('b')
    cup(10, 1)
