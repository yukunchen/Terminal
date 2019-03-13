import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')

    sgr(31)
    write(' 1 ')

    sgr(7) # reverse
    write(' 2 ')

    sgr(44) # Set the background, see that the FG has changed
    write(' 3 ')

    sgr(27) # reverse-reset, see that the text is red-on-blue
    write(' 4 ')

    sgr_n([38,2,255,22,255]) # rgb:MAGENTA fg
    write(' 5 ')

    sgr(7)
    write(' 6 ')

    sgr(0)
    write(' 7 ')

    sgr(31)
    write(' 8 ')

    sgr(7)
    write(' 9 ')

    sgr(44)
    write(' A ')

    sgr(27)
    write(' B ')

    sgr(0)
    write(' C ')

    # sgr(31)
    # write(' D ')

    # sgr(31)
    # write(' E ')

