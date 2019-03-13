import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    cup(3, 1)
    for i in range(0, 32):
        write('X')
    flush(1)
    cup(3, 16)
    flush(.5)
    csi('14P')
    flush(.5)
    write('Y')
    flush(.5)
    cup(4, 1)
