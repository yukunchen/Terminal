import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

# sol_fg = '38;5;137m' # kinda brownish
# sol_fg = '38;5;107m' # greenish?
sol_fg = '38;5;240m' # grey
sol_bg = '48;5;230m'
sol_str_fg = '38;5;80m'
sol_str_bg = sol_bg

if __name__ == '__main__':
    csi('H')
    csi('2J')

    for i in range(1, 10):

        csi(sol_fg)
        csi(sol_bg)

        write('This is line ')
        csi(sol_str_fg)
        csi(sol_str_bg)
        write('"{}"'.format(i))

        if i%2 == 0:
            csi(sol_fg)
            csi(sol_bg)
            write('foo '*i)
        csi('K')
        # write('          ')
        write('\n')
    csi('0m')

#     csi(sol_fg)
#     csi(sol_bg)
    # csi('48;2;100;255;100m')

    #csi('3;8r')
    csi('3;H')
    csi('L')
    # csi('r')
    csi('15;H')
