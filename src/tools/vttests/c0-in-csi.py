import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')

    write('\x03')
    print('  This text should follow a \\x03')

    write('\x1b')
    write('\x03')
    write('[31m')
    print('  This text should not be red')
    csi('m')

    write('\x1b[')
    write('\x03')
    write('32m')
    print('  This text should not be green')
    sgr()

    write('\x1b[')
    write('\n')
    write('32m')
    print('  This text should not be green')
    sgr()

    write('\x1b[31;32;33')
    write('\x1b[')
    write('34m')
    print('  This text should not be blue')
    sgr()


    for i in range(0, 0x1f):
        write('\x1b')
        write(chr(i))
        print('[31m  This text should not be red ({:02X})'.format(i))
        sgr()

