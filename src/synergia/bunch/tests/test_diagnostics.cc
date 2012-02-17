#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "synergia/bunch/diagnostics.h"
#include "synergia/foundation/physical_constants.h"
#include "synergia/utils/boost_test_mpi_fixture.h"
BOOST_GLOBAL_FIXTURE(MPI_fixture)

const double tolerance = 1.0e-11;

const double mass = 100.0;
const double total_energy = 125.0;
const int total_num = 9;
const double real_num = 2.0e12;
const int turns = 17;
const double turn_length = 246.8;
const double partial_s = 123.4;

void
dummy_populate(Bunch &bunch)
{
    for (int part = 0; part < bunch.get_local_num(); ++part) {
        for (int i = 0; i < 6; i += 1) {
            bunch.get_local_particles()[part][i] = 10.0 * part
                    + (1.0 + part * part / 1000.0) * i;
        }
        bunch.get_local_particles()[part][Bunch::id] = part;
    }
}

struct Fixture
{
    Fixture() :
            bunch_sptr(
                    new Bunch(reference_particle, total_num, real_num, comm)), reference_particle(
                    pconstants::electron_charge, mass, total_energy), comm(
                    MPI_COMM_WORLD)
    {
        BOOST_TEST_MESSAGE("setup fixture");
        dummy_populate(*bunch_sptr);
        bunch_sptr->get_reference_particle().set_trajectory(turns, turn_length,
                partial_s);
    }
    ~Fixture()
    {
        BOOST_TEST_MESSAGE("teardown fixture");
    }

    Reference_particle reference_particle;
    Commxx comm;
    Bunch_sptr bunch_sptr;
};

BOOST_FIXTURE_TEST_CASE(construct, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
}

BOOST_FIXTURE_TEST_CASE(is_serial, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    BOOST_CHECK(diagnostics.is_serial());
}

BOOST_FIXTURE_TEST_CASE(get_s, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    BOOST_CHECK_CLOSE(diagnostics.get_s(), partial_s, tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_repetition, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    BOOST_CHECK_EQUAL(diagnostics.get_repetition(), turns);
}

BOOST_FIXTURE_TEST_CASE(get_trajectory_length, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    BOOST_CHECK_CLOSE(diagnostics.get_trajectory_length(),
            turns * turn_length + partial_s, tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_num_particles, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    BOOST_CHECK_EQUAL(diagnostics.get_num_particles(), total_num);
}

BOOST_FIXTURE_TEST_CASE(get_real_num_particles, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    BOOST_CHECK_CLOSE(diagnostics.get_real_num_particles(), real_num,
            tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_mean, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
#include "test_diagnostics_get_mean.icc"
}

BOOST_FIXTURE_TEST_CASE(get_std, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
#include "test_diagnostics_get_std.icc"
}

// BOOST_FIXTURE_TEST_CASE(get_bunchmin, Fixture)
// {
//     Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
// #include "test_diagnostics_get_bunchmin.icc"
// }

//BOOST_FIXTURE_TEST_CASE(get_bunchmax, Fixture)
//{
//    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
//#include "test_diagnostics_get_bunchmax.icc"
//}

BOOST_FIXTURE_TEST_CASE(write_, Fixture)
{
    Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    diagnostics.write();
}
// test_note: We are not (yet) testing the content of the output file.

// n.b. no test for update because it is called internally for other tests.
BOOST_FIXTURE_TEST_CASE(serialize_basic, Fixture)
{
    {
        Diagnostics_basic diagnostics(bunch_sptr, "dummy.h5");
        diagnostics.update();
        diagnostics.write();

        xml_save(diagnostics, "diagnostics_basic.xml");
    }

    Diagnostics_basic loaded;
    xml_load(loaded, "diagnostics_basic.xml");
}

BOOST_FIXTURE_TEST_CASE(construct_full2, Fixture)
{
    Diagnostics_full2 diagnostics(bunch_sptr, "dummy.h5");
}

BOOST_FIXTURE_TEST_CASE(is_serial_full2, Fixture)
{
    Diagnostics_full2 diagnostics(bunch_sptr, "dummy.h5");
    BOOST_CHECK(diagnostics.is_serial());
}

BOOST_FIXTURE_TEST_CASE(get_s_full2, Fixture)
{
    Diagnostics_full2 diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    BOOST_CHECK_CLOSE(diagnostics.get_s(), partial_s, tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_num_particles_full2, Fixture)
{
    Diagnostics_full2 diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    BOOST_CHECK_EQUAL(diagnostics.get_num_particles(), total_num);
}

BOOST_FIXTURE_TEST_CASE(get_real_num_particles_full2, Fixture)
{
    Diagnostics_full2 diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
    BOOST_CHECK_CLOSE(diagnostics.get_real_num_particles(), real_num,
            tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_mean_full2, Fixture)
{
    Diagnostics_full2 diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
#include "test_diagnostics_get_mean.icc"
}

BOOST_FIXTURE_TEST_CASE(get_std_full2, Fixture)
{
    Diagnostics_full2 diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
#include "test_diagnostics_get_mean.icc"
}

BOOST_FIXTURE_TEST_CASE(get_mom2_full2, Fixture)
{
    Diagnostics_full2 diagnostics(bunch_sptr, "dummy.h5");
    diagnostics.update();
#include "test_diagnostics_get_mom2.icc"
}

BOOST_FIXTURE_TEST_CASE(get_corr_full2, Fixture)
{
    const double tolerance_corr = 1.0e-10;
    Bunch_sptr bunch2_sptr(
            new Bunch(reference_particle, total_num, real_num, comm));
    MArray2d_ref particles(bunch2_sptr->get_local_particles());
#include "test_diagnostics_get_random_particles.icc"
    Diagnostics_full2 diagnostics(bunch2_sptr, "dummy.h5");
    diagnostics.update();
#include "test_diagnostics_get_corr.icc"
}

const double tolerance_emit2d = 1.0e-11;

// BOOST_FIXTURE_TEST_CASE(get_emitx_full2, Fixture)
// {
//     Bunch_sptr bunch2_sptr(new Bunch(reference_particle, total_num, real_num,
//             comm));
//     MArray2d_ref particles(bunch2_sptr->get_local_particles());
// #include "test_diagnostics_get_random_particles.icc"
//     Diagnostics_full2 diagnostics(bunch2_sptr, "dummy.h5");
//     diagnostics.update();
// #include "test_diagnostics_get_emitx.icc"
// }

/*BOOST_FIXTURE_TEST_CASE(get_emity_full2, Fixture)
 {
 Bunch_sptr bunch2_sptr(new Bunch(reference_particle, total_num, real_num,
 comm));
 MArray2d_ref particles(bunch2_sptr->get_local_particles());
 #include "test_diagnostics_get_random_particles.icc"
 Diagnostics_full2 diagnostics(bunch2_sptr, "dummy.h5");
 diagnostics.update();
 #include "test_diagnostics_get_emity.icc"
 }*/

// BOOST_FIXTURE_TEST_CASE(get_emitz_full2, Fixture)
// {
//     Bunch_sptr bunch2_sptr(new Bunch(reference_particle, total_num, real_num,
//             comm));
//     MArray2d_ref particles(bunch2_sptr->get_local_particles());
// #include "test_diagnostics_get_random_particles.icc"
//     Diagnostics_full2 diagnostics(bunch2_sptr, "dummy.h5");
//     diagnostics.update();
// #include "test_diagnostics_get_emitz.icc"
// }
// const double tolerance_emit4d = 1.0e-11;
//
// BOOST_FIXTURE_TEST_CASE(get_emitxy_full2, Fixture)
// {
//     Bunch_sptr bunch2_sptr(new Bunch(reference_particle, total_num, real_num,
//             comm));
//     MArray2d_ref particles(bunch2_sptr->get_local_particles());
// #include "test_diagnostics_get_random_particles.icc"
//     Diagnostics_full2 diagnostics(bunch2_sptr, "dummy.h5");
//     diagnostics.update();
// #include "test_diagnostics_get_emitxy.icc"
// }
//
// const double tolerance_emit6d = 1.0e-10;
//
// BOOST_FIXTURE_TEST_CASE(get_emitxyz_full2, Fixture)
// {
//     Bunch_sptr bunch2_sptr(new Bunch(reference_particle, total_num, real_num,
//             comm));
//     MArray2d_ref particles(bunch2_sptr->get_local_particles());
// #include "test_diagnostics_get_random_particles.icc"
//     Diagnostics_full2 diagnostics(bunch2_sptr, "dummy.h5");
//     diagnostics.update();
// #include "test_diagnostics_get_emitxyz.icc"
// }
BOOST_FIXTURE_TEST_CASE(serialize_full2, Fixture)
{
    {
        Diagnostics_full2 diagnostics(bunch_sptr, "dummy_full2.h5");
        diagnostics.update();
        diagnostics.write();
        xml_save(diagnostics, "full2.xml");
    }
    Diagnostics_full2 loaded;
    xml_load(loaded, "full2.xml");
}

// test_note: We are not (yet) testing the content of the output file.
