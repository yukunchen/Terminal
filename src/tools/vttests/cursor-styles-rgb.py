import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def osc(seq):
    sys.stdout.write('\x1b]{}\x07'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print('This is the VT Test template.')
    # styles = [
    #     'blinking block',
    #     'blinking block (Default)',
    #     'solid block',
    #     'blinking underline',
    #     'solid underline',
    #     'blinking vertical',
    #     'solid vertical'
    # ]
    # for i in range(0, 7):
    #     print('i={}'.format(i))
    #     csi('{} q'.format(i))
    #     print(styles[i])
    #     time.sleep(2)
    print('Set cursor color with text')
    osc('12;red')
    print('cursor should be red')
    sys.stdout.flush()
    time.sleep(1)

    print('Set cursor color with #RRGGBB')
    osc('12;#ff00ff')
    print('cursor should be MAGENTA')
    sys.stdout.flush()
    time.sleep(1)

    print('Set cursor color with rgb:RR/GG/BB')
    osc('12;rgb:00/ff/00')
    print('cursor should be GREEN')
    sys.stdout.flush()
    time.sleep(1)

    # This definitely doesn't work
    #print('We\'re going to try and usr RGB for cursor colors')
    #osc('12;128;255;255')

    print('reseting the color')
    osc('112')

