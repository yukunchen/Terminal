import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    sgr(7)
    write("This is the titlebar - should be reversed")
    csi('K')
    write('\n')
    flush(1)
    sgr(27)
    write('This is some text ')
    esc('7') # push the cursor
    write('of the body of the file')
    write('\n')
    write('\n')
    print('')
    flush(1)
    sgr_n([48, 5, 250])
    print('This is the footer')
    sgr_n([39, 49])
    esc('8')
    flush(1)
    write('This text should be normal colored')

    flush(5)
