import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/margin-scrollback-001.py
if __name__ == '__main__':
    clear_all()
    cup(20, 0)
    write('[')
    cup(20, 80)
    write(']')
    for i in range(0, 12):
        write('\nBottom line {}'.format(i))

    top_margin = 0
    cup(top_margin, 1)
    margins(top_margin, 10)

    print('This is the VT Test template.')
    _max=150
    timeout = .001

    for i in range(0, _max):
        write('Loading {}...\n'.format(i))
        flush(timeout)
        write('\tLoaded {}\n'.format(i))
        csi('?25l')
        esc('7')
        percent=float(i)/float(_max)

        cup(20, int(percent*78)+2)
        write('#')
        esc('8')
        csi('?25h')
        flush(timeout)

        if i == 70:
            flush(10)

    margins()
