import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)


def color_cube(step=8):
    for r in range(0, 255, step):
        for g in range(0, 255, step):
            for b in range(0, 255, step):
                sys.stdout.write('\x1b[48;2;{};{};{}m  \x1b[0m'.format(r,g,b))
            sys.stdout.write('\n')
        sys.stdout.write('\n\n')

if __name__ == '__main__':
    # step = 8
    # if len(sys.argv) > 1:
    #     step = int(sys.argv[1])
    # color_cube(step)
    # print('1')
    # print('2')
    # print('3')
    csi('H')
    csi('2J')

    # print('5')
    # print('6')
    # print('7')

    # Clear to end
    csi('H')
    # csi('2J')
    print('123456789\n234567890\n345678901')
    csi('2;5H')
    csi('0J')
    write('x')
    time.sleep(.5)
    time.sleep(5)

    # From Beginning
    csi('H')
    # csi('2J')
    print('123456789\n234567890\n345678901')
    csi('2;5H')
    csi('1J')
    write('y')
    time.sleep(.5)
    time.sleep(5)

    # All
    csi('H')
    # csi('2J')
    print('123456789\n234567890\n345678901')
    csi('2;5H')
    csi('2J')
    write('z')
    time.sleep(.5)
    time.sleep(5)
