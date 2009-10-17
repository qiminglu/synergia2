#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "components/bunch/bunch.h"
#include "utils/boost_test_mpi_fixture.h"
BOOST_GLOBAL_FIXTURE(MPI_fixture)
;

const double tolerance = 1.0e-15;

const double mass = 100.0;
const double total_energy = 125.0;
const int proton_charge = 1;
const int total_num = 100;
const double real_num = 2.0e12;

struct Fixture
{
    Fixture() :
        four_momentum(mass, total_energy), reference_particle(four_momentum),
                comm(MPI_COMM_WORLD), bunch(reference_particle, proton_charge,
                        total_num, real_num, comm)
    {
        BOOST_TEST_MESSAGE("setup fixture");
    }
    ~Fixture()
    {
        BOOST_TEST_MESSAGE("teardown fixture");
    }

    Four_momentum four_momentum;
    Reference_particle reference_particle;
    Commxx comm;
    Bunch bunch;
};

void
dummy_populate(Bunch &bunch, int id_offset = 0)
{
    for (int part = 0; part < bunch.get_local_num(); ++part) {
        int id = part + id_offset;
        // coordinates
        for (int i = 0; i < 6; i += 2) {
            bunch.get_local_particles()[part][i] = 10.0 * id + i;
        }
        // momenta
        for (int i = 1; i < 6; i += 2) {
            bunch.get_local_particles()[part][i] = 1e-4 * (10.0 * id + i);
        }
        bunch.get_local_particles()[part][Bunch::id] = id;
    }
}

void
compare_bunches(Bunch &bunch1, Bunch &bunch2, double tolerance = tolerance,
        bool check_state = true)
{
    BOOST_CHECK_EQUAL(bunch1.get_reference_particle().get_total_energy(),
            bunch2.get_reference_particle().get_total_energy());
    BOOST_CHECK_EQUAL(bunch1.get_particle_charge(),
            bunch2.get_particle_charge());
    BOOST_CHECK_CLOSE(bunch1.get_mass(), bunch2.get_mass(), tolerance);
    BOOST_CHECK_CLOSE(bunch1.get_real_num(), bunch1.get_real_num(), tolerance);
    BOOST_CHECK_EQUAL(bunch1.get_local_num(), bunch2.get_local_num());
    BOOST_CHECK_EQUAL(bunch1.get_total_num(), bunch2.get_total_num());
    if (check_state) {
        BOOST_CHECK_EQUAL(bunch1.get_state(), bunch2.get_state());
    }
    for (int part = 0; part < bunch1.get_local_num(); ++part) {
        for (int i = 0; i < 7; ++i) {
            BOOST_CHECK_CLOSE(bunch1.get_local_particles()[part][i],
                    bunch2.get_local_particles()[part][i], tolerance);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(construct, Fixture)
{
}

BOOST_FIXTURE_TEST_CASE(copy_construct, Fixture)
{
    dummy_populate(bunch);
    Bunch second_bunch(bunch);
    compare_bunches(bunch, second_bunch);
}

BOOST_FIXTURE_TEST_CASE(assign, Fixture)
{
    Bunch second_bunch(reference_particle, proton_charge, total_num + 10,
            real_num * 2, comm);
    dummy_populate(bunch);
    second_bunch = bunch;
    compare_bunches(bunch, second_bunch);
}

BOOST_FIXTURE_TEST_CASE(get_particle_charge, Fixture)
{
    BOOST_CHECK_CLOSE(bunch.get_particle_charge(),proton_charge,tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_mass, Fixture)
{
    BOOST_CHECK_CLOSE(bunch.get_mass(),mass,tolerance);
}

BOOST_FIXTURE_TEST_CASE(set_particle_charge, Fixture)
{
    const int electron_charge = -1;
    bunch.set_particle_charge(electron_charge);
    BOOST_CHECK_CLOSE(bunch.get_particle_charge(),electron_charge,tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_real_num, Fixture)
{
    BOOST_CHECK_CLOSE(bunch.get_real_num(),real_num,tolerance);
}

BOOST_FIXTURE_TEST_CASE(set_real_num, Fixture)
{
    const double new_real_num = real_num * 1.5;
    bunch.set_real_num(new_real_num);
    BOOST_CHECK_CLOSE(bunch.get_real_num(),new_real_num,tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_local_num, Fixture)
{
    // n.b.: this test assumes that we are running on one processor
    BOOST_CHECK_EQUAL(bunch.get_local_num(),total_num);
}

BOOST_FIXTURE_TEST_CASE(set_local_num, Fixture)
{
    int new_local_num = total_num / comm.get_size() - 5;
    bunch.set_local_num(new_local_num);
    BOOST_CHECK_EQUAL(bunch.get_local_num(),new_local_num);
}

BOOST_FIXTURE_TEST_CASE(get_total_num, Fixture)
{
    BOOST_CHECK_EQUAL(bunch.get_total_num(),total_num);
}

BOOST_FIXTURE_TEST_CASE(update_total_num, Fixture)
{
    const int new_local_num = 47;
    bunch.set_local_num(new_local_num);
    bunch.update_total_num();
    BOOST_CHECK_EQUAL(bunch.get_total_num(),
            new_local_num*Commxx(MPI_COMM_WORLD).get_size());
}

BOOST_FIXTURE_TEST_CASE(get_reference_particle, Fixture)
{
    Reference_particle ref(bunch.get_reference_particle());
}

BOOST_FIXTURE_TEST_CASE(get_const_reference_particle, Fixture)
{
    const Reference_particle ref(bunch.get_reference_particle());
}

BOOST_FIXTURE_TEST_CASE(get_local_particles, Fixture)
{
    MArray2d_ref local_particles(bunch.get_local_particles());
    BOOST_CHECK_EQUAL(local_particles.shape()[1],7);
    BOOST_CHECK(local_particles.shape()[0] >= bunch.get_local_num());
}

BOOST_FIXTURE_TEST_CASE(get_const_local_particles, Fixture)
{
    Const_MArray2d_ref local_particles(bunch.get_local_particles());
    BOOST_CHECK_EQUAL(local_particles.shape()[1],7);
    BOOST_CHECK(local_particles.shape()[0] >= bunch.get_local_num());
}

BOOST_FIXTURE_TEST_CASE(increase_local_num, Fixture)
{
    const int small_total_num = 10;
    const int increase = 5;
    Bunch bunch2(reference_particle, proton_charge, small_total_num, real_num,
            comm);
    // populate bunch2
    MArray2d_ref particles(bunch2.get_local_particles());
    int old_local_num = bunch2.get_local_num();
    for (int particle = 0; particle < old_local_num; ++particle) {
        for (int index = 0; index < 7; ++index) {
            particles[particle][index] = particle * 10.0 + index;
        }
    }

    // expand bunch2 and verify that old values are still there
    bunch2.set_local_num(old_local_num + increase);
    bunch2.update_total_num();
    MArray2d_ref particles2(bunch2.get_local_particles());
    BOOST_CHECK_EQUAL(particles2.shape()[1],7);
    BOOST_CHECK(particles2.shape()[0] >= bunch2.get_local_num());
    for (int particle = 0; particle < old_local_num; ++particle) {
        for (int index = 0; index < 7; ++index) {
            BOOST_CHECK_CLOSE(particles[particle][index],
                    particles[particle][index],tolerance);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(get_state, Fixture)
{
    Bunch::State state;
    state = bunch.get_state();
    BOOST_CHECK_EQUAL(state,Bunch::fixed_z);
}

BOOST_FIXTURE_TEST_CASE(convert_to_state, Fixture)
{
    // This is a trivial test too see that converting to
    // fixed_t then back to fixed_z gives the original bunch.
    dummy_populate(bunch);
    Bunch second_bunch(bunch);
    bunch.convert_to_state(Bunch::fixed_t);
    bunch.convert_to_state(Bunch::fixed_z);
    const double convert_tolerance = 1.0e-9;
    compare_bunches(bunch, second_bunch, convert_tolerance);

}

class Fixed_t_z_dummy : public Fixed_t_z_converter
{
public:
    void
    fixed_t_to_fixed_z(Bunch &bunch)
    {
    }
    ;
    void
    fixed_z_to_fixed_t(Bunch &bunch)
    {
    }
    ;
};

BOOST_FIXTURE_TEST_CASE(set_converter, Fixture)
{
    // This test relies on the Fixed_t_z_dummy class not
    // doing anything to the bunch. It verifies that we are not
    // using the default converter after set_converter.
    Fixed_t_z_dummy converter;
    bunch.set_converter(converter);
    dummy_populate(bunch);
    Bunch second_bunch(bunch);
    bunch.convert_to_state(Bunch::fixed_t);
    compare_bunches(bunch, second_bunch, tolerance, false);
}

BOOST_FIXTURE_TEST_CASE(inject, Fixture)
{
    Bunch total_bunch(bunch);
    const int local_num = 10;
    bunch.set_local_num(local_num);
    bunch.update_total_num();
    Bunch second_bunch(bunch);
    dummy_populate(bunch);
    dummy_populate(second_bunch, local_num);
    total_bunch.set_local_num(2* local_num );
    total_bunch.update_total_num();
    dummy_populate(total_bunch);
    bunch.inject(second_bunch);
    compare_bunches(bunch, total_bunch);
}

BOOST_FIXTURE_TEST_CASE(inject_mismatched_weights, Fixture)
{
    Bunch second_bunch(bunch);
    second_bunch.set_real_num(second_bunch.get_real_num() * 2.0);
    bool caught_error = false;
    try {
        bunch.inject(second_bunch);
    }
    catch (std::runtime_error) {
        caught_error = true;
    }
    BOOST_CHECK(caught_error);
}
