import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print('This is the VT Test template.')
    print('\x1B[7m Reversed \x1B[27m Normal \x1B[38;2;128;5;255m Set 24-bit foreColor \x1B[7m Reversed \x1B[27m Normal')
    csi('0m')
    csi('38;2;255;128;0m')
    write(' rgb foreground ')
    csi('7m')
    write(' reverse ')
    csi('38;2;255;128;128m')
    write(' change fg ')
    csi('27m')
    write(' positive ')
    csi('0m')
    print('')
