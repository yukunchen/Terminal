import sys
import time # time.sleep is in seconds
from common import *

if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    csi('44m')
    csi('200B')
    flush(1)
    write('\n')
    write('\rThis whole line should have a blue BG')
    flush(1)
    write('\n\n')
    write('\rThis whole line should have a blue BG')
    flush(1)
    csi('200A')
    flush(1)
    write('\x1bM')
    write('\rThis whole line should have a blue BG')
    flush(1)
    csi('0m')
