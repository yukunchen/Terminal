import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    sgr(0)
    print('Writing a bunch of lines with default attr\'s, they should all be all default colored.')
    flush(.5)
    for i in range(0, 5000):
        print('line {}'.format(i))

    flush(1)

    # Test 2: default on black
    write('\n')
    write('Now we\'ll do the same, but in ')
    sgr(40)
    write('Default on black\n')
    flush(1)
    for i in range(0, 5000):
        print('black line {}'.format(i))

    flush(1)

    sgr(0)
    # Test 3: crazy colors
    write('\n')
    write('Now we\'ll do the same, but in ')
    sgr(45)
    sgr(32)
    write('CRAZY COLORS\n')
    flush(1)
    for i in range(0, 5000):
        print('crazy line {}'.format(i))

    print('Done')

