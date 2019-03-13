import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':
    csi('2J')
    csi('H')
    print('This is the VT Test template.')

    write('\x1b[?25l')
    write('\x1b[49m\x1b[38;2;175;135;95m\x1b[48;2;28;28;28m^M[0]SPC\x1b[39m\x1b[48;2;68;68;68mSPC0\x1b[38;2;188;188;188m:\x1b[38;2;238;238;238mbash\x1b[38;2;0;255;215m*SPC\x1b[38;2;175;135;95m\x1b[48;2;28;28;28m\x1b[90X\x1b[90D\x1b[39m\x1b[48;2;98;98;98mSPC24/01SPC\x1b[48;2;138;138;138mSPC15:15:27SPC\x1b[?25h')
    write('\n')
    time.sleep(1)
    write('\x1b[?25l')
    write('\x1b[49m\x1b[38;2;175;135;95m\x1b[48;2;28;28;28m^M[0]SPC\x1b[39m\x1b[48;2;68;68;68mSPC0\x1b[38;2;188;188;188m:\x1b[38;2;238;238;238mbash\x1b[38;2;0;255;215m*SPC\x1b[38;2;175;135;95m\x1b[48;2;28;28;28m\x1b[90X\x1b[90D\x1b[39m\x1b[48;2;98;98;98mSPC24/01SPC\x1b[48;2;138;138;138mSPC15:15:28SPC\x1b[?25h')
    write('\n')
    time.sleep(1)
    write('\x1b[?25l')
    write('\x1b[49m\x1b[38;2;175;135;95m\x1b[48;2;28;28;28m^M[0]SPC\x1b[39m\x1b[48;2;68;68;68mSPC0\x1b[38;2;188;188;188m:\x1b[38;2;238;238;238mbash\x1b[38;2;0;255;215m*SPC\x1b[38;2;175;135;95m\x1b[48;2;28;28;28m\x1b[90X\x1b[90D\x1b[39m\x1b[48;2;98;98;98mSPC24/01SPC\x1b[48;2;138;138;138mSPC15:15:29SPC\x1b[?25h')
