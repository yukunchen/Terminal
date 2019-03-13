import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    alt_buffer()
    print('This is the VT Test template.')

    bottom_margin = 29

    sgr(44)
    write('This is above the top margin')
    csi('K')

    cup(bottom_margin+1, 1)
    sgr(41)
    write('This is below the bottom margin')
    csi('K')

    margins(3, bottom_margin)
    cup(3, 1)
    sgr()

    for i in range(0, 32):
        # print('Line {}'.format(i))
        print('Line {}a'.format(i))
        # write('Line {}b'.format(i))
        # csi('2B')
        # write('\r')
        if i > 15:
            flush(.25)
            csi('S')
            flush(.25)
        flush(.06125)

    flush(1)
    cup(3, 1)
    for i in range(0, 10):
        cup(3, 1)
        flush(.25)
        write('negative {}'.format(i))
        flush(.25)
        csi('T')
        # esc('M')
        # csi('2A')
        # write('\r')
        flush(.25)

    flush(5)

    margins()
    main_buffer()

