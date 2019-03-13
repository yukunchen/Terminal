import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/margin-scrollback-000.py
if __name__ == '__main__':
    clear_all()

    cup(0, 1)
    margins(0, 7)

    print('This is the VT Test template.')

    for i in range(0, 10):
        write(('Line {}'.format(i))*i)
        write('\n')
        flush(.25)

    margins()
