import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def main(argv):
    csi('?12l')
    print 'Blinking off'
    time.sleep(5)

    csi('?12h')
    print 'Blinking on'
    time.sleep(5)

    csi('?25;12l')
    print 'invisible'
    time.sleep(5)

    csi('?25;12h')
    print 'visible on'
    time.sleep(5)

if __name__ == '__main__':
    main(sys.argv)
