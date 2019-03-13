import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    print('This is the VT Test template.')

    # print lines numbered [0,44]
    for i in range(0, 45):
        write('Line {}'.format(i))
        if i < 44:
            write('\n')

    # on a 30 tall console, 
    # Line 15 is the first line of the console right now, 
    # and line 44 is at the bottom.
    #

    csi('25;1H')
    csi('3L')
    write('foo')



    csi('1;8H')
    csi('M')
    write('bar')
