import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def osc(seq):
    sys.stdout.write('\x1b]{}\x07'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    r, g, b = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
    print('Setting cursor color to RGB({}, {}, {})'.format(r, g, b))
    r_str = '%x' % r
    g_str = '%x' % g
    b_str = '%x' % b
    seq = '12;rgb:{}/{}/{}'.format(r_str, g_str, b_str)
    osc(seq)

