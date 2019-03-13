import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    cup(29, 1)
    write('This is where we expect the text')
    flush(.5)
    cup()
    flush(.5)
    csi('26B')
    flush(.5)
    margins(2, 28)
    cup(30, 12)
    flush(.5)
    write('\r')
    flush(.5)
    flush(.5)
    csi('A')
    flush(.5)
    write('WRONG')

    margins()
