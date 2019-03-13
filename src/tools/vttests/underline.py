import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')

    sgr(4);
    print('This text should be underlined')
    sgr(24)
    print('This text should not be')

