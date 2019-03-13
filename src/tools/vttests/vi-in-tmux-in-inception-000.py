import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    cup(2, 5)
    # Bright white on black
    sgr(37)
    sgr(1)
    sgr(40)
    write('<Filename>')
    flush(.5)
    csi('15X')
    flush(.5)
    csi('15C')
    flush(.5)
    write('buffers')
    flush(.5)
    sgr(0)
    flush(.5)
    write('\n')
