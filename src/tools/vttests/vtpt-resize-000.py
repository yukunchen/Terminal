import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/vtpt-resize-000.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    print('This is the main buffer. Gonna print a 10x10 box to prove it')
    for y in range(0, 20):
        for x in range(0, 20):
            write('M')
        csi('1000C')
        write('\bR') # dont do it at the actual EOL because that'll wrap weird
        write('\r')
        write('\n')

    print('Switching to alt buffer...')
    flush(1)

    csi('?1049h')
    print('You now have 5 seconds to futz with this')

    for i in range(0, 256):
        sgr_n([48, 5, i])
        write('[]')
    print('')
    csi('1000C')
    write('\bR\r\n')
    csi('1000C')
    write('\bR\r\n')
    csi('1000C')
    write('\bR\r\n')
    flush(5)
    print('That was a lie, I\'m gonna give you a second more')
    for i in range(0, 256):
        sgr_n([48, 5, i])
        write('[]')
    sgr(0)
    print('')
    csi('1000C')
    write('\bR\r\n')
    csi('1000C')
    write('\bR\r\n')
    csi('1000C')
    write('\bR\r\n')
    flush(5)
    print('switching back to main...')
    flush(1)
    csi('?1049l')
