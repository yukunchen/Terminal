import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/default-fgbg-000.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    sgr(32)
    write('[dark_green,default]')
    sgr(43)
    write('[dark_green,dark_yellow]')
    sgr(39)
    write('[default,dark_yellow]')

    sgr(0)
    write('\n')

    sgr(42)
    write('[default,dark_green]')
    sgr(33)
    write('[dark_yellow,dark_green]')
    sgr(49)
    write('[dark_yellow,default]')

    sgr(0)
    write('\n')

    sgr(47)
    write('a')
    sgr(0)
    write('b')
    sgr(39)
    sgr(49)
    write('c')

    sgr(0)
    write('\n')
