import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    cup(5, 5)
    write('V The cursor is right below here')
    cup(6, 5)
    flush(1)
    csi('2J')
    flush(2)
    write('right where we left it')
