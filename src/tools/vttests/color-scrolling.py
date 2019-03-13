import sys
import time

# this is a file for testing color scrolling with vtpipeterm.
# For one reason or another, the colors don't always persist.

def csi(seq):
    sys.stdout.write('\x1b[{}'.format(seq))

def write(s):
    sys.stdout.write(s)

if __name__ == '__main__':

    # this block seems like it's able to scroll just fine
    # csi('H')
    # csi('2J')
    # for i in range(0, 30):
    #     csi('{}m'.format(30+(i%8)))
    #     write('line #{}\n'.format(i))

    # try changing colors
    # This segment can cause the wrong colors after scrolling it.
    # It seems that scrolling the colors causes the rest of the line to not update.
    # csi('H')
    # csi('2J')
    # for i in range(0, 15):
    #     color_index = i%7 + 1
    #     next_color = (color_index+1)%7 + 1
    #     csi('{}m'.format(30+color_index))
    #     write('line #{}'.format(i))
    #     csi('{}m'.format(30+next_color))
    #     write('\t more text')
    #     write('\n')


    # Reproduce what the terminal is going to get on a scroll.
    # A bunch of color, then a delete line on the first line.
    # csi('H')
    # csi('2J')
    # for i in range(0, 15):
    #    color_index = i%7 + 1
    #     next_color = (color_index+1)%7 + 1
    #     csi('{}m'.format(30+color_index))
    #     write('line #{}'.format(i))
    #     csi('{}m'.format(30+next_color))
    #     write('\t more text')
    #     write('\n')
    # csi('s') # ansi.sys save cursor
    # csi('H')
    # csi('M')
    # csi('u') # ansi.sys restore cursor

    # So the above segment revealed a bug that was fixed, but something else
    # is still broken..
    #
    # As it turn out, nothing was broken.
    # openvt would launch vtpipeterm in the inbox console, which didn't have the fix..
    csi('H')
    csi('2J')
    for i in range(0, 15):
        color_one = ((i * 20) % 230) + 16
        color_two = ((i * 30) % 230) + 16
        color_index = i%7 + 1
        next_color = (color_index+1)%7 + 1
        csi('38;5;{}m'.format(color_one))
        # csi('{}m'.format(30+color_index))
        write('line #{} {}'.format(i, color_one))
        csi('38;5;{}m'.format(color_two))
        # csi('{}m'.format(30+next_color))
        write('\t more text {}'.format(color_two))
        write('\n')
    csi('s') # ansi.sys save cursor
    csi('H')
    csi('3L')
    csi('M')
    csi('u') # ansi.sys restore cursor




