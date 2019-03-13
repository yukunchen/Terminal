import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    for i in range(0, 120):
        write('{}'.format(i % 10))

    flush(1)
    write('\r\n')
    write('f')
    flush(1)
    write('\r \r')
    flush(1)

    cup(2, 120)
    flush(1)
    write(' \x08')
    flush(1)

    write('\x08 \x08')
    flush(1)
    write('\n\n')


