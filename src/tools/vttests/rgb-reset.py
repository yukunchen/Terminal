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
    print('reset BG should not touch FG:')
    print('\E[38;2;64;128;255mTest\E[49mTest\E[m')
    csi('38;2;64;128;255m')
    write('Test')
    csi('49m')
    write('Test')
    csi('m')
    print('')

    print('reset FG should not touch BG:')
    print('\E[48;2;64;128;255mTest\E[39mTest\E[m')
    csi('48;2;64;128;255m')
    write('Test')
    csi('39m')
    write('Test')
    csi('m')
    print('')

    print('Underline should not touch BG:')
    print('\E[48;2;64;128;255mTest\E[4mTest\E[m')
    csi('48;2;64;128;255m')
    write('Test')
    csi('4m')
    write('Test')
    csi('m')
    print('')

    csi('4;48;2;64;128;255m')
    write('UnderlineTest')
    csi('m')
    print('')


