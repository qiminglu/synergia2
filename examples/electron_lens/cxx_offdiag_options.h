#ifndef CXX_OFFDIAG_OPTIONS_H_
#define CXX_OFFDIAG_OPTIONS_H_
#include <string>

// this file was automatically generated by the command
//     synergia cxx_offdiag_options.py --create-cxx-options-source
// DO NOT EDIT

struct Cxx_offdiag_options
{
    Cxx_offdiag_options(int argc, char **argv);
    int elensdivide;
    int num_bunches;
    double yrms;
    double elenslength;
    double transhemit;
    double emity;
    int gridy;
    int gridx;
    double dpoprms;
    int gridz;
    double aperture_sigma;
    double halpha;
    int seed;
    bool cell_particles;
    double zrms;
    int macroparticles;
    std::string transport;
    double errsize;
    int load_bunch;
    double elenslongrms;
    double zoffset;
    double real_particles;
    int save_bunch;
    int turn_track;
    std::string stepper;
    int num_steps;
    double set_nux;
    double set_nuy;
    bool beamparams;
    double k2l;
    double valpha;
    bool zparticle;
    int turn_period;
    int space_charge_2dh;
    int verbosity;
    int concurrentio;
    int space_charge_rec;
    std::string lattice_file;
    double emitx;
    double yoffset;
    int space_charge_3dh;
    int spc_comm_size;
    bool flatbucket;
    bool elensadaptive;
    double transvemit;
    int magiccomp;
    int num_steps_else;
    int steps_per_quad;
    double elensradius;
    int checkpointperiod;
    double xrms;
    int harmon;
    double xoffset;
    double rf_voltage;
    int map_order;
    std::string solver;
    double sccomp;
    double elensenergy;
    int spc_tuneshift;
    int errelement;
    double elenscurrent;
    double vbeta;
    int turn_particles;
    int num_turns;
    bool bpms;
    double hbeta;
    bool apertures;
    int maxturns;
};
#endif /* CXX_OFFDIAG_OPTIONS_H_ */
