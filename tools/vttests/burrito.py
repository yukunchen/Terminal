# coding=utf-8
# MAKE SURE YOU SAVE THIS FILE AS UTF-8!!!

import sys
import time # time.sleep is in seconds
from common import *

# Run this file with:
#   python burrito.py
if __name__ == '__main__':
    clear_all()
    print('We are going to make a burrito:')
    print(u"\U0001F32F") # Burrito
    write('Here he have some components of a burrito:')
    # POULTRY LEG, CHEESE WEDGE, HOT PEPPER, TOMATO
    print(u"\U0001F357\U0001F9C0\U0001F336\U0001F345")
    # woman cook and man cook emoji
    print(u"👩‍🍳 packing them up ‍👨‍🍳")
    sgr('92')
    print(u'✔ Complete!')
    sgr('0')
    write('\n')
    print(u'🌯🌯🌯🌯🌯🌯🌯🌯🌯🌯🌯')
