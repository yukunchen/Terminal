import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/stbm-reset-cursor.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')

    margins(4, 8)
    print('foofoofoofoo')
    flush(.25)
    cup(5, 5)
    print('bar')
    flush(.25)
    margins()
    print('baz')
    flush(.25)

