import sys
import time # time.sleep is in seconds
from common import *
from tqdm import tqdm

# $ python ~/vttests/name-of-file.py
if __name__ == '__main__':
    clear_all()
    print('This is the VT Test template.')
    for i in range(0, 100):
        print('line {}'.format(i))
    for _ in tqdm(range(1000)):
        time.sleep(0.05)
