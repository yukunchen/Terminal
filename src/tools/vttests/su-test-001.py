import sys
import time # time.sleep is in seconds
from common import *

if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template. This line is going to disappear')

    for i in range(0, 10):
        write('\nThis is a longer line {}'.format(i))
        flush(.2)
    flush(1)
    write('\x0d')
    flush(1)
    csi('1S')
    flush(1)

