import sys
import time # time.sleep is in seconds
from common import *

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    print('Press Ctrl+C to exit')
    try:

        exit_requested = False
        iteration = 0
        while not exit_requested:
            print('Line #{}a'.format(iteration))
            flush(.1)
            print('Line #{}b'.format(iteration))
            flush(.1)
            print('Line #{}c'.format(iteration))
            flush(.1)
            iteration+=1
            flush(1)
    finally:
        print('quitting')

