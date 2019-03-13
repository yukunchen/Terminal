import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print('This is the VT Test template.')
    styles = [
        'blinking block',
        'blinking block (Default)',
        'solid block',
        'blinking underline',
        'solid underline',
        'blinking vertical',
        'solid vertical'
    ]
    for i in range(0, 7):
        print('i={}'.format(i))
        csi('{} q'.format(i))
        print(styles[i])
        time.sleep(2)

