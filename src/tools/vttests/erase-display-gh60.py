import sys
import time

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)


def test_none():
    csi('H')
    csi('2J')
    for i in range(0, 31):
        csi('4{}m'.format(i%8))
        write('Line {}\n'.format(i))
    time.sleep(1)
    csi('16;1H')
    csi('J')


def test_1():
    csi('H')
    csi('2J')
    for i in range(0, 31):
        csi('4{}m'.format(i%8))
        write('Line {}\n'.format(i))
    time.sleep(1)
    csi('16;1H')
    csi('1J')

def test_1_bottom():
    csi('H')
    csi('2J')
    for i in range(0, 31):
        csi('4{}m'.format(i%8))
        write('Line {}\n'.format(i))
    time.sleep(1)
    csi('1J')

def test_1_top():
    csi('H')
    csi('2J')
    for i in range(0, 31):
        csi('4{}m'.format(i%8))
        write('Line {}\n'.format(i))
    time.sleep(1)
    csi('H')
    csi('1J')

def test_2():
    csi('H')
    csi('2J')
    for i in range(0, 31):
        csi('4{}m'.format(i%8))
        write('Line {}\n'.format(i))
    time.sleep(1)
    csi('16;1H')
    csi('2J')

def test_2_bottom():
    csi('H')
    csi('2J')
    for i in range(0, 31):
        csi('4{}m'.format(i%8))
        write('Line {}\n'.format(i))
    time.sleep(1)
    csi('2J')

def test_2_top():
    csi('H')
    csi('2J')
    for i in range(0, 31):
        csi('4{}m'.format(i%8))
        write('Line {}\n'.format(i))
    time.sleep(1)
    csi('H')
    csi('2J')



if __name__ == '__main__':
    # print('This is the VT Test template.')
    # test_none()
    # test_1()
    # test_1_bottom()
    # test_1_top()
    test_2()
    # test_2_bottom()
    # test_2_top()
    time.sleep(1)
    time.sleep(1)

