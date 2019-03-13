import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')

    write('ABCDEFGHIJK')
    flush(.5)
    csi('2D')
    flush(.5)
    csi('P')
    flush(.5)
    write('\n')
    print('The J should be deleted')
    print('Also, the entire line should be one color')
