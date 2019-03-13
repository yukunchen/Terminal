import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    print('Scroll up right now. In a few secs we\'ll scroll back down')
    flush(5)
    cup(3, 0)
    print('This line should apear on line 3, below the other two lines.')
