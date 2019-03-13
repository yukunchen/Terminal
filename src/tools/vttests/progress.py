import sys
import time
import math
def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

PROGRESS_BAR_WIDTH = 60
def write_progress_bar(percent):
    real_width = PROGRESS_BAR_WIDTH - 2
    filled_chars = int(math.ceil(real_width * percent))
    empty_chars = real_width - filled_chars
    write('[')
    write('#' * filled_chars)
    write('.' * empty_chars)
    write(']')

if __name__ == '__main__':
    csi('H')
    csi('2J')
    print('This is the VT Test template.')
    print('loading...')
    csi('s') # save cursor
    percent = 0.0
    interval = .25
    increment = .03
    while(percent < 1.0):
        csi('3;1H')
        #csi('u')
        write_progress_bar(percent)
        percent += increment
        sys.stdout.flush()
        time.sleep(interval)
    csi('u') # restore cursor
    print('Done!')
