#!/usr/bin/env python

# jfa; This file is a goal version of a simple script. It does not (yet) work,
# and never may.

import synergia

num_macro_particles = 1000
seed = 4
grid = [16, 16, 16]
num_real_particles = 1e12
num_steps = 10
num_turns = 1000
map_order = 2
emit = 1e-6
stdz = 0.01
dpop = 1e-4

lattice = synergia.Mad8_reader().get_lattice("fodo", "fodo.lat")
space_charge = synergia.Space_charge_3d_open_hockney(grid)
lattice_simulator = synergia.Lattice_simulator(lattice, map_order)
stepper = synergia.Split_operator_stepper(lattice_simulator, space_charge,
                                          num_steps)
bunch = generate_matched_bunch_transverse(lattice_simulator, emit, emit, stdz, dpop,
                                        num_real_particles, num_macro_particles,
                                        seed=seed)
propagator = Propagator(stepper)
propagator.propagate(bunch, num_turns, True, False)
