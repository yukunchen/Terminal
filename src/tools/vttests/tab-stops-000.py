import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    alt_buffer()
    clear_all()
    print('This is the VT Test template.')

    print('##### Test 1: current tab stops #####')
    for i in range(0, 9):
        write('{}'.format(i))
        flush(0.025)
        ht()
    flush(1)
    write('\n')

    print('##### Test 2: clear tab stops #####')
    tbc()
    for i in range(0, 9):
        write('{}'.format(i))
        flush(0.025)
        ht()
    flush(1)
    write('\n')

    esc('c')
    print('##### Test 3: default tab stops #####')
    for i in range(0, 9):
        write('{}'.format(i))
        flush(0.025)
        ht()
    write('\n')
    write('\n')
    flush(1)
    csi('120C') # to the right
    for i in range(0, 9):
        write('{}'.format(i))
        flush(.5)
        cbt()
    write('\n')

    flush(2)

    print('##### Test 4: default tab stops, with BS #####')
    for i in range(0, 9):
        write('{}'.format(i))
        flush(0.025)
        ht()
    write('\n')
    write('\n')
    flush(1)
    csi('120C') # to the right
    for i in range(0, 9):
        write('{}'.format(i))
        write('\x08')
        flush(.5)
        cbt()
    write('\n')


    print('##### Test 5: autowrap on? #####')
    csi('?7h')
    for i in range(0, 9):
        write('{}'.format(i))
        flush(0.025)
        ht()
    write('\n')
    write('\n')
    flush(1)
    csi('120C') # to the right
    for i in range(0, 9):
        write('{}'.format(i))
        flush(.5)
        cbt()
    write('\n')

    print('##### Test 6: autowrap off? #####')
    csi('?7l')
    for i in range(0, 9):
        write('{}'.format(i))
        flush(0.025)
        ht()
    write('\n')
    write('\n')
    flush(1)
    csi('120C') # to the right
    for i in range(0, 9):
        write('{}'.format(i))
        flush(.5)
        cbt()
    write('\n')

    flush(5)

    main_buffer()

