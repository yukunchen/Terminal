import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')

    for line in range(0, 9000):
        for col in range(0, 80):
            csi('48;5;{}m'.format(col+line % 256))
            csi('38;5;{}m'.format(col-line % 256))
            write('X')
        write('\n')
