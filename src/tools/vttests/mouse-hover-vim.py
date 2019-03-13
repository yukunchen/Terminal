import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    csi('?1049h')
    esc('=')
    print('This is the VT Test template.')
    print('Press Ctrl+C to exit')
    try:
        # enable mouse modes
        csi('?1000h') # mouse mode
        csi('?1h')
        # csi('?1049h')
        # csi('?1006h') # sgr mode
        # csi('?1003h') # "any-event" mode, for hover tracking

        exit_requested = False
        while not exit_requested:
            in_text = sys.stdin.read()
            if 'q' == in_text:
                exit_requested = True
                break
            escaped = in_text.replace('\x1b', '\\x1b')
            print(escaped)
            write('\n')
    finally:
        # turn off mouse modes
        # csi('?1003l')
        # csi('?1006l')
        csi('?1000l')
        csi('?1l')
        csi('?1049l')
        esc('>')

