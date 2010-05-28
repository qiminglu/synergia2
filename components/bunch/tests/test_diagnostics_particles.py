import sys
sys.path.append('..')
sys.path.append('../../foundation')
sys.path.append('../../convertors')

from mpi4py import MPI
from pyfoundation import Reference_particle, Four_momentum
from pybunch import Bunch
from pybunch import Diagnostics_particles
import pyconvertors
import numpy
from nose.tools import *

mass = 2.2;
total_energy = 3.0
total_num = 100
real_num = 1.0e12
proton_charge = 1
turns = 17;
turn_length = 246.8
partial_s = 123.4
max_particles = 4

reference_particle = Reference_particle(proton_charge, mass, total_energy)
reference_particle.set_trajectory(turns, turn_length, partial_s)
comm = MPI.COMM_WORLD
bunch = Bunch(reference_particle, total_num, real_num, comm)
particles = bunch.get_local_particles()
particles[:, 0:6] = numpy.random.lognormal(size=[bunch.get_total_num(), 6])

def test_construct():
    diagnostics = Diagnostics_particles()

def test_construct2():
    diagnostics = Diagnostics_particles(max_particles)

def test_construct3():
    diagnostics = Diagnostics_particles(bunch)

def test_construct4():
    diagnostics = Diagnostics_particles(bunch, max_particles)
