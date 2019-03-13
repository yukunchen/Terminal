import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/vtrender-default-bold.py
if __name__ == '__main__':
    # clear_all()

    print('This is the VT Test template.')
    sgr(32)
    print('reference dark green')
    sgr(92)
    print('reference bright green')
    sgr(0)
    write('This text is default colored')
    sgr(1)
    write('this text is bold')
    sgr(39)
    write('Unbright, bold;')
    sgr(22)
    write('Unbright Unbold;')
    print('')

    sgr(0)
    write('This text is default colored')
    sgr(1)
    sgr(32)
    write('this text is bold, green')
    sgr(39)
    write('Color gets reset, boldness stays;')
    print('')

    sgr(0)
    write('This text is default colored;')
    sgr(1)
    sgr(32)
    write('this text is bold, green;')
    sgr(22)
    write('Color loses brightness, boldness gone;')
    print('')

    sgr(0)
    write('This text is default colored')
    sgr(92)
    write('this text is bright, green')
    sgr(22)
    write('Color gets reset, boldness stays')
    print('')

    sgr(0)
    write('default color;')
    sgr(32)
    write('this text is dark, green;')
    sgr(1)
    write('bright green;')
    sgr(22)
    write('darkgreen;')

    print('')
