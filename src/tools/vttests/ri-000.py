import sys
import time # time.sleep is in seconds
from common import *
def case_0():
    clear_all()
    print('This is case_0')

    for i in range(0, 20):
        print(i)

    margins(4, 8)
    cup(5, 1)
    flush(1)
    esc('M')

    flush(1)
    esc('M')
    flush(1)
    esc('M')
    flush(1)
    esc('M')

    margins()

def case_1():
    clear_all()
    print('This is case_1')

    for i in range(0, 20):
        print(i)

    margins(0, 4)
    cup(2, 1)
    flush(1)
    esc('M')

    flush(1)
    esc('M')
    flush(1)
    esc('M')
    flush(1)
    esc('M')

    margins()

def case_2():
    clear_all()
    print('This is case 2')

    for i in range(0, 20):
        print(i)

    margins(4, 8)
    cup(2, 1)
    flush(1)
    esc('M')

    flush(1)
    esc('M')
    flush(1)
    esc('M')
    flush(1)
    esc('M')

    margins()

# $ python ~/vttests/ri-000.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    flush(1)
    case_0()
    flush(1)
    case_1()
    flush(1)
    case_2()
    flush(1)

