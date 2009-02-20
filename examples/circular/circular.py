#!/usr/bin/env bwpython

import numpy
import time
import math
import os
import sys

import synergia
import s2_fish

from mpi4py import MPI

if ( __name__ == '__main__'):
    t0 = time.time()

    myopts = synergia.Options("circular")
    #~ myopts.add("current",0.5,"current",float)
    myopts.add("transverse",0,"longitudinally uniform beam",int)
    myopts.add("maporder",2,"map order",int)
    myopts.add("emittance",5.89533703303356e-07,"emittance",float)
    myopts.add("dpop",1.0e-10,"(delta p)/p RMS width",float)
    myopts.add("dpopoffset", 1.0e-4, "offset in dpop", float)
    myopts.add("kicks",32,"kicks per line",int)
    myopts.add("turns",10,"number of turns",int)
    myopts.add("latticefile","foborodobo_s.lat","",str)
    myopts.add("tgridnum",16,"transverse grid cells",int)
    myopts.add("lgridnum",64,"",int)
    myopts.add("xoffset",0,"transverse offset in x",float)
    myopts.add("yoffset",0,"transverse offset in y",float)
#    myopts.add("zoffset",0,"offset in z", float)
#    myopts.add("xoffset",4.26e-4,"transverse offset in x",float)
#    myopts.add("yoffset",1.86e-4,"transverse offset in y",float)
    myopts.add("zoffset",0,"offset in z", float)
    myopts.add("space_charge",0,"",int)
    myopts.add("impedance",0,"",int)
    myopts.add("energy",100.004401675138,"",float)
    myopts.add("partpercell",1,"",float)
    myopts.add("bunches",1,"",int)
    myopts.add("bunchnp",1.0e11,"number of particles per bunch",float)
    
    myopts.add_suboptions(synergia.opts)
    myopts.parse_argv(sys.argv)
    job_mgr = synergia.Job_manager(sys.argv,myopts,
                                      [myopts.get("latticefile")])

    t0 = time.time()
    energy = myopts.get("energy")
    mass = synergia.PH_NORM_mp
    kinetic_energy = energy-mass
    charge = 1.0
    initial_phase = 0.0
    scaling_frequency = 47713451.5923694
    pipexradius = 0.03
    pipeyradius = 0.03
#    pipexradius = 0.123
#    pipeyradius = 0.0508
    part_per_cell = myopts.get("partpercell")
    kicks_per_line = myopts.get("kicks")
    tgridnum = myopts.get("tgridnum")
    lgridnum = myopts.get("lgridnum")
    griddim = (tgridnum,tgridnum,lgridnum)
    num_particles = int(griddim[0]*griddim[1]*griddim[2] * part_per_cell)
    solver = "3d"
    xoffset = myopts.get("xoffset")
    yoffset = myopts.get("yoffset")
    zoffset = myopts.get("zoffset")
    
    pipe_conduct= 1.4e6 # [/s] (stainless steel)
    
    impedance=myopts.get("impedance")
    space_charge=myopts.get("space_charge")

    if MPI.COMM_WORLD.Get_rank() ==0:
        print "space_charge =",space_charge
        print "impedance =",space_charge
        print "num_particles =",num_particles

    ee = synergia.Error_eater()
    ee.start()
    gourmet = synergia.Gourmet(os.path.join(os.getcwd(),myopts.get("latticefile"))
        ,"model",kinetic_energy,
                        scaling_frequency)

### insert rf freq
    for element in gourmet.beamline:
        ###print element.Type()
        if element.Type() == 'rfcavity':
            element.setFrequency(59955852.5381452)
            print "my rf cavity frequency is ", element.getRadialFrequency()/(2.0*math.pi)

    # try without commissioning
    #gourmet.needs_commission = True
    #gourmet.is_commissioned = False
    #gourmet._commission()

    #gourmet.check()
    # sys.exit(1)
    #jet_particle = gourmet.get_initial_jet_particle()
    #particle = gourmet.get_initial_particle()
    #gourmet.printpart(particle)
    #for element in gourmet.beamline:
    #    element.propagateJetParticle(jet_particle)
    #    element.propagateParticle(particle)
    #    map = gourmet._convert_linear_maps([jet_particle.State().jacobian()])[0]
    #    print element.Name(),element.Type()
    #    gourmet.printpart(particle)
#        print Numeric.array2string(map,precision=2)
    #    energy = jet_particle.ReferenceEnergy()
    #    print "energy =",energy
    #    jet_particle = gourmet.get_jet_particle(energy)
    #sys.exit(1)
    
    gourmet.insert_space_charge_markers(kicks_per_line)
    (alpha_x, alpha_y, beta_x, beta_y) = synergia.matching.get_alpha_beta(gourmet)
    #~ print "(alpha_x, alpha_y, beta_x, beta_y) = %g, %g, %g, %g" % (alpha_x, alpha_y, beta_x, beta_y)
    
    
    beam_parameters = synergia.Beam_parameters(mass, charge, kinetic_energy,
                                         initial_phase, scaling_frequency,
                                         transverse=0)
    betagamma=beam_parameters.get_beta()*beam_parameters.get_gamma() 

    emittance = myopts.get("emittance")
    pz = beam_parameters.get_gamma() * beam_parameters.get_beta() * beam_parameters.mass_GeV

    (xwidth,xpwidth,rx) = synergia.matching.match_twiss_emittance(emittance,alpha_x,beta_x)
    beam_parameters.x_params(sigma = xwidth, lam = xpwidth * pz,r = rx,offset=xoffset)
    
    (ywidth,ypwidth,ry) = synergia.matching.match_twiss_emittance(emittance,alpha_y,beta_y)
    beam_parameters.y_params(sigma = ywidth, lam = ypwidth * pz,r = ry,offset=yoffset)
    
    sigma_z_meters = 0.43
    beam_parameters.z_params(sigma = sigma_z_meters, lam = myopts.get("dpop")* pz, offset=zoffset, offset_p = myopts.get("dpopoffset")*pz)

    sys.stdout.flush()
    
    s = 0.0
    line_length = gourmet.orbit_length()
    bunch_spacing = line_length/588.0
    if MPI.COMM_WORLD.Get_rank() ==0:
        print "line_length =",line_length
        print "bunch_spacing =",bunch_spacing
    tau = 0.5*line_length/kicks_per_line
    kick_time = 0.0
    beta = beam_parameters.get_beta()
    current = myopts.get("bunchnp")* \
        synergia.physics_constants.PH_MKS_e/ \
        (bunch_spacing/(beta*synergia.physics_constants.PH_MKS_c))
    print "current =",current
    
    numbunches = myopts.get("bunches")
    bunches = []
    diags = []
    for bunchnum in range(0,numbunches):
        bunches.append(s2_fish.Macro_bunch(mass,1))
        bunches[bunchnum].init_gaussian(num_particles,current,beam_parameters)
        bunches[bunchnum].write_particles("begin-%02d"%bunchnum)
        diags.append(synergia.Diagnostics(gourmet.get_initial_u()))

    log = open("log","w")
    if MPI.COMM_WORLD.Get_rank() ==0:
            output = "start propagation"
            print output
            log.write("%s\n" % output)
            log.flush()
    for turn in range(1,myopts.get("turns")+1):
        t1 = time.time()
        s = synergia.propagate(s,gourmet,
            bunches,diags,griddim,use_s2_fish=True,periodic=True,
            impedance=impedance,space_charge=space_charge,
            pipe_radiusx=pipexradius,pipe_radiusy=pipeyradius,
            pipe_conduct=pipe_conduct,bunch_spacing=bunch_spacing)
        if MPI.COMM_WORLD.Get_rank() ==0:
            output = "turn %d time = %g"%(turn,time.time() - t1)
            print output
            log.write("%s\n" % output)
            log.flush()
    for bunchnum in range(0,numbunches):
        if MPI.COMM_WORLD.Get_rank() == 0:
            diags[bunchnum].write_hdf5("mi-%02d"%bunchnum)
    for bunchnum in range(0,numbunches):
        bunches[bunchnum].write_particles("end-%02d"%bunchnum)
    log.close()
    if MPI.COMM_WORLD.Get_rank() ==0:
        print "elapsed time =",time.time() - t0
 
 
