import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    sgr(31)
    print('This is some red text')
    flush(1)
    alt_buffer()
    sgr(31)
    print('This is some red text')
    flush(1)
    set_color(1, 0, 255, 255)
    print('This is some more red text, but it should be cyan now')
    flush(2)
    main_buffer()
    print('This is more red text in the main buffer')
