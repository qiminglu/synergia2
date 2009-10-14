#include "bunch.h"
#include "utils/parallel_utils.h"
#include <stdexcept>

void
Fixed_t_z_zeroth::fixed_t_to_fixed_z(Bunch &bunch)
{
    // This routine just copied/translated from older branch.
    // Check/fix me please.
    double gamma_ref = bunch.get_reference_particle().get_gamma();
    double mass = bunch.get_mass();
    MArray2d_ref particles = bunch.get_local_particles();
    for (int part = 0; part < bunch.get_local_num(); ++part) {
        double f_x = particles[part][Bunch::xp] / mass;
        double f_y = particles[part][Bunch::yp] / mass;
        double gamma = gamma_ref - particles[part][Bunch::tp] / mass;
        double beta = sqrt(1.0 - (1 + f_x * f_x + f_y * f_y) / (gamma * gamma));
        particles[part][Bunch::z] /= -gamma * beta;
    }
}

void
Fixed_t_z_zeroth::fixed_z_to_fixed_t(Bunch &bunch)
{
    // This routine just copied/translated from older branch.
    // Check/fix me please.
    double gamma_ref = bunch.get_reference_particle().get_gamma();
    double mass = bunch.get_mass();
    MArray2d_ref particles = bunch.get_local_particles();
    for (int part = 0; part < bunch.get_local_num(); ++part) {
        double f_x = particles[part][Bunch::xp] / mass;
        double f_y = particles[part][Bunch::yp] / mass;
        double gamma = gamma_ref - particles[part][Bunch::tp] / mass;
        double beta = sqrt(1.0 - (1 + f_x * f_x + f_y * f_y) / (gamma * gamma));
        particles[part][Bunch::z] *= -gamma * beta;
    }
}

void
Fixed_t_z_ballistic::fixed_t_to_fixed_z(Bunch &bunch)
{
    std::cout << "stub: ballistic fixed_t_to_fixed_z\n";
}

void
Fixed_t_z_ballistic::fixed_z_to_fixed_t(Bunch &bunch)
{
    std::cout << "stub: ballistic fixed_z_to_fixed_t\n";
}

Bunch::Bunch(Reference_particle const& reference_particle, int particle_charge,
        int total_num, double real_num, Commxx const& comm) :
    reference_particle(reference_particle), comm(comm), default_converter()
{
    this->particle_charge = particle_charge;
    this->total_num = total_num;
    this->real_num = real_num;
    local_num = decompose_1d_local(comm, total_num);
    local_particles = new MArray2d(boost::extents[local_num][7]);
    state = fixed_z;
    particles_valid = false;
    converter_ptr = &default_converter;
}

void
Bunch::set_particle_charge(int particle_charge)
{
    this->particle_charge = particle_charge;
}

void
Bunch::set_real_num(double real_num)
{
    this->real_num = real_num;
}

void
Bunch::set_local_num(int local_num)
{
    this->local_num = local_num;
    if (local_particles->shape()[0] < local_num) {
        local_particles->resize(boost::extents[local_num][7]);
    }
}

void
Bunch::update_total_num()
{
    int old_total_num = total_num;
    MPI_Allreduce(&local_num, &total_num, 1, MPI_INT, MPI_SUM, comm.get());
    real_num = (total_num * real_num) / old_total_num;
}

void
Bunch::set_converter(Fixed_t_z_converter &converter)
{
    this->converter_ptr = &converter;
}

void
Bunch::convert_to_state(State state)
{
    if (this->state != state) {
        if (state == fixed_t) {
            converter_ptr->fixed_z_to_fixed_t(*this);
            this->state = fixed_t;
        } else if (state == fixed_z) {
            converter_ptr->fixed_t_to_fixed_z(*this);
            this->state = fixed_z;
        } else {
            throw std::runtime_error("Unknown state in Bunch::convert_to_state");
        }
    }
}

Reference_particle &
Bunch::get_reference_particle()
{
    return reference_particle;
}

MArray2d_ref
Bunch::get_local_particles()
{
    return *local_particles;
}

int
Bunch::get_particle_charge()
{
    return particle_charge;
}

double
Bunch::get_mass()
{
    return reference_particle.get_four_momentum().get_mass();
}

double
Bunch::get_real_num()
{
    return real_num;
}

int
Bunch::get_local_num()
{
    return local_num;
}

int
Bunch::get_total_num()
{
    return total_num;
}

Bunch::State
Bunch::get_state()
{
    return state;
}

Bunch::~Bunch()
{
    delete local_particles;
}
