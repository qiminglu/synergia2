#!/usr/bin/env python

import sys
import synergia
from matplotlib import pyplot
from math import sin, cos
import numpy

used_legends = []
def plot_element(element, x, y, angle, attributes, highlight):
    global used_legends
    l = element.get_length()
    bend_angle = element.get_bend_angle()
    if element.get_length() == 0.0:
        xnew = x
        ynew = y
        anglenew = angle
    else:
        ancestors = element.get_ancestors()
        type = element.get_type()
        color = list(attributes[type].color)
        if (ancestors.count(highlight) == 0) and (highlight != None):
            offset = 0.75
            color[0] = offset + (1.0 - offset) * color[0]
            color[1] = offset + (1.0 - offset) * color[1]
            color[2] = offset + (1.0 - offset) * color[2]
        label = None
        if used_legends.count(type) == 0:
            label = type
            used_legends.append(type)
        if element.get_bend_angle() == 0.0:
            xnew = x + l * cos(angle)
            ynew = y + l * sin(angle)
            anglenew = angle
            pyplot.plot([x, xnew], [y, ynew], '-',
                        color=color,
                        label=label,
                        linewidth=attributes[type].width)
        else:
            num = 8
            xn = numpy.zeros([num + 1], numpy.float64)
            yn = numpy.zeros([num + 1], numpy.float64)
            xn[0] = x
            yn[0] = y
            for i in range(1, num + 1):
                angle += bend_angle / num
                xn[i] = xn[i - 1] + (l / num) * cos(angle)
                yn[i] = yn[i - 1] + (l / num) * sin(angle)
            xnew = xn[num]
            ynew = yn[num]
            anglenew = angle
            pyplot.plot(xn, yn, '-',
                        color=color,
                        label=label,
                        linewidth=attributes[type].width)

    return xnew, ynew, anglenew

class Attributes:
    def __init__(self, color, width=2.0):
        self.color = color
        self.width = width

def get_attributes():
    attributes = {}
    attributes['drift'] = Attributes((0.5, 0.5, 0.5))
    attributes['quadrupole'] = Attributes((1, 0, 0))
    attributes['sbend'] = Attributes((0, 0, 1))
    attributes['rbend'] = Attributes((0, 0, 0.5))
    attributes['sextupole'] = Attributes((0, 0.5, 0.5))
    attributes['octupole'] = Attributes((1, 0.5, 0))
    attributes['monitor'] = Attributes((1, 1, 0))
    attributes['hmonitor'] = Attributes((0.5, 1, 0))
    attributes['vmonitor'] = Attributes((1, 0.5, 0))
    attributes['kicker'] = Attributes((0, 1, 1))
    attributes['hkicker'] = Attributes((0, 1, 0.5))
    attributes['vkicker'] = Attributes((0, 0.5, 1))
    return attributes

class Options:
    def __init__(self):
        self.lattice_file = None
        self.lattice = None
        self.legend = True
        self.highlight = None

def do_error(message):
    sys.stderr.write(message + '\n')
    sys.exit(1)

def do_help():
    print "usage: synlatticeview <filename> <lattice> [option1] ... [optionn]"
    print "available options are:"
    print "    --nolegend : do not show legend"
    print "    --highlight=name : highlight line 'name'"
    sys.exit(0)

def handle_args(args):
    if len(args) < 2:
        do_help()
    options = Options()
    options.lattice_file = args[0]
    options.lattice = args[1]
    for arg in args[2:]:
        if arg == '--help':
            do_help(plotparams)
        elif arg == '--nolegend':
            options.legend = False
        elif arg.find('--highlight') == 0:
            highlight = arg.split('=')[1]
            options.highlight = highlight
        else:
            do_error('Unknown argument "%s"' % arg)
    return options

def do_plot(options):
    lattice = synergia.lattice.Mad8_reader().get_lattice(options.lattice,
                                                         options.lattice_file)
    attributes = get_attributes()
    x = 0.0
    y = 0.0
    angle = 0.0
    count = 0
    for element in lattice.get_elements():
        x, y, angle = plot_element(element, x, y, angle, attributes,
                                   options.highlight)
        count += 1
    print "total number of elements =", count
    print "total angle =", lattice.get_total_angle()

    pyplot.axes().set_aspect('equal', 'datalim')
    pyplot.axes().margins(0.1, 0.1)
    if options.legend:
        pyplot.legend()
    pyplot.show()

if __name__ == '__main__':
    options = handle_args(sys.argv[1:])
    do_plot(options)
