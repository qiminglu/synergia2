#!/usr/bin/env python

import sys
sys.path.append('..')
sys.path.append('../../foundation')

from mpi4py import MPI
from pyfoundation import Reference_particle, Four_momentum
from pybunch import Bunch, Fixed_t_z_zeroth, Fixed_t_z_ballistic
import numpy
from nose.tools import *

mass = 2.2;
total_energy = 3.0
total_num = 100
real_num = 1.0e12
proton_charge = 1
reference_particle = Reference_particle(mass, total_energy)

def test_construct():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
               MPI.COMM_WORLD)

def test_set_get_particle_charge():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
               MPI.COMM_WORLD)
    new_charge = -proton_charge
    b.set_particle_charge(new_charge)
    assert_equal(b.get_particle_charge(),new_charge)

def test_set_get_real_num():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    new_real_num = 2*real_num
    b.set_real_num(new_real_num)
    assert_equal(b.get_real_num(),new_real_num)

def test_set_get_local_num():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    new_local_num = 2*b.get_local_num()
    b.set_local_num(new_local_num)
    assert_equal(b.get_local_num(),new_local_num)

def test_update_total():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    new_local_num = 100
    new_total_num = MPI.COMM_WORLD.Get_size() * new_local_num
    b.set_local_num(new_local_num)
    b.update_total_num()
    assert_equal(b.get_total_num(),new_total_num)
    
def test_convert_to_state():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    b.convert_to_state(Bunch.fixed_t)

def test_set_converter():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    converter = Fixed_t_z_ballistic()
    b.set_converter(converter)

def test_get_reference_particle():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    r = b.get_reference_particle()
    assert_equal(reference_particle.get_total_energy(),r.get_total_energy())
    
def test_get_local_particles():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    p = b.get_local_particles()
    assert p.shape[0] >= b.get_local_num()
    assert p.shape[1] == 7

def test_get_mass():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    assert_equal(reference_particle.get_four_momentum().get_mass(),
                 b.get_mass())

def test_get_state():
    b = Bunch(reference_particle, proton_charge, total_num, real_num,
              MPI.COMM_WORLD)
    assert b.get_state() == Bunch.fixed_z
    
def test_attrs():
    assert Bunch.x == 0
    assert Bunch.xp == 1
    assert Bunch.y == 2
    assert Bunch.yp == 3
    assert Bunch.z == 4
    assert Bunch.zp == 5
    assert Bunch.t == 4
    assert Bunch.tp == 5
    assert Bunch.id == 6

