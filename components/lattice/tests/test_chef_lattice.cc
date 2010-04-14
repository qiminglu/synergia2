#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "components/lattice/chef_lattice.h"
#include "components/lattice/chef_utils.h"
#include <basic_toolkit/PhysicsConstants.h>

const std::string name("fodo");
const double mass = PH_NORM_mp;
const double total_energy = 8.9;
const double quad_length = 0.2;
const double quad_strength = 0.07;
const double drift_length = 3.0;
const double tolerance = 1.0e-12;

struct Fodo_fixture
{
    Fodo_fixture() :
        four_momentum(mass, total_energy), reference_particle(four_momentum),
                lattice(name)
    {
        BOOST_TEST_MESSAGE("setup fixture");
        lattice.set_reference_particle(reference_particle);
        Lattice_element f("quadrupole", "f");
        f.set_double_attribute("l", quad_length);
        f.set_double_attribute("k1", quad_strength);
        Lattice_element o("drift", "o");
        o.set_double_attribute("l", drift_length);
        Lattice_element d("quadrupole", "d");
        d.set_double_attribute("l", quad_length);
        d.set_double_attribute("k1", quad_strength);

        lattice.append(f);
        lattice.append(o);
        lattice.append(d);
        lattice.append(o);
    }
    ~Fodo_fixture()
    {
        BOOST_TEST_MESSAGE("teardown fixture");
    }

    Lattice lattice;
    Four_momentum four_momentum;
    Reference_particle reference_particle;
};

const double bend_length = 0.15;
const int n_cells = 3;
const double pi = 3.141592653589793;

struct Fobodobo_sbend_fixture
{
    Fobodobo_sbend_fixture() :
        four_momentum(mass, total_energy), reference_particle(four_momentum),
                lattice(name)
    {
        BOOST_TEST_MESSAGE("setup fixture");
        lattice.set_reference_particle(reference_particle);
        Lattice_element f("quadrupole", "f");
        f.set_double_attribute("l", quad_length);
        f.set_double_attribute("k1", quad_strength);
        Lattice_element o("drift", "o");
        o.set_double_attribute("l", drift_length);
        Lattice_element d("quadrupole", "d");
        d.set_double_attribute("l", quad_length);
        d.set_double_attribute("k1", quad_strength);

        double bend_angle = 2 * pi / (2 * n_cells);
        Lattice_element b("sbend", "b");
        b.set_double_attribute("l", bend_length);
        b.set_double_attribute("angle", bend_angle);

        lattice.append(f);
        lattice.append(o);
        lattice.append(b);
        lattice.append(o);
        lattice.append(d);
        lattice.append(o);
        lattice.append(b);
        lattice.append(o);
    }
    ~Fobodobo_sbend_fixture()
    {
        BOOST_TEST_MESSAGE("teardown fixture");
    }

    Lattice lattice;
    Four_momentum four_momentum;
    Reference_particle reference_particle;
};

struct Fobodobo_sbend_markers_fixture
{
    Fobodobo_sbend_markers_fixture() :
        four_momentum(mass, total_energy), reference_particle(four_momentum),
                lattice(name)
    {
        BOOST_TEST_MESSAGE("setup fixture");
        lattice.set_reference_particle(reference_particle);
        Lattice_element f("quadrupole", "f");
        f.set_double_attribute("l", quad_length);
        f.set_double_attribute("k1", quad_strength);
        Lattice_element o("drift", "o");
        o.set_double_attribute("l", drift_length);
        Lattice_element d("quadrupole", "d");
        d.set_double_attribute("l", quad_length);
        d.set_double_attribute("k1", quad_strength);

        double bend_angle = 2 * pi / (2 * n_cells);
        Lattice_element b("sbend", "b");
        b.set_double_attribute("l", bend_length);
        b.set_double_attribute("angle", bend_angle);

        Lattice_element m("marker", "marker");

        lattice.append(f);
        lattice.append(m);
        lattice.append(o);
        lattice.append(m);
        lattice.append(b);
        lattice.append(m);
        lattice.append(o);
        lattice.append(m);
        lattice.append(d);
        lattice.append(m);
        lattice.append(o);
        lattice.append(m);
        lattice.append(b);
        lattice.append(m);
        lattice.append(o);
    }
    ~Fobodobo_sbend_markers_fixture()
    {
        BOOST_TEST_MESSAGE("teardown fixture");
    }

    Lattice lattice;
    Four_momentum four_momentum;
    Reference_particle reference_particle;
};

struct Fobodobo_rbend_fixture
{
    Fobodobo_rbend_fixture() :
        four_momentum(mass, total_energy), reference_particle(four_momentum),
                lattice(name)
    {
        BOOST_TEST_MESSAGE("setup fixture");
        lattice.set_reference_particle(reference_particle);
        Lattice_element f("quadrupole", "f");
        f.set_double_attribute("l", quad_length);
        f.set_double_attribute("k1", quad_strength);
        Lattice_element o("drift", "o");
        o.set_double_attribute("l", drift_length);
        Lattice_element d("quadrupole", "d");
        d.set_double_attribute("l", quad_length);
        d.set_double_attribute("k1", quad_strength);

        double bend_angle = 2 * pi / (2 * n_cells);
        Lattice_element b("rbend", "b");
        b.set_double_attribute("l", bend_length);
        b.set_double_attribute("angle", bend_angle);

        lattice.append(f);
        lattice.append(o);
        lattice.append(b);
        lattice.append(o);
        lattice.append(d);
        lattice.append(o);
        lattice.append(b);
        lattice.append(o);
    }
    ~Fobodobo_rbend_fixture()
    {
        BOOST_TEST_MESSAGE("teardown fixture");
    }

    Lattice lattice;
    Four_momentum four_momentum;
    Reference_particle reference_particle;
};

struct Fobodobo_rbend_markers_fixture
{
    Fobodobo_rbend_markers_fixture() :
        four_momentum(mass, total_energy), reference_particle(four_momentum),
                lattice(name)
    {
        BOOST_TEST_MESSAGE("setup fixture");
        lattice.set_reference_particle(reference_particle);
        Lattice_element f("quadrupole", "f");
        f.set_double_attribute("l", quad_length);
        f.set_double_attribute("k1", quad_strength);
        Lattice_element o("drift", "o");
        o.set_double_attribute("l", drift_length);
        Lattice_element d("quadrupole", "d");
        d.set_double_attribute("l", quad_length);
        d.set_double_attribute("k1", quad_strength);

        double bend_angle = 2 * pi / (2 * n_cells);
        Lattice_element b("rbend", "b");
        b.set_double_attribute("l", bend_length);
        b.set_double_attribute("angle", bend_angle);

        Lattice_element m("marker", "marker");

        lattice.append(f);
        lattice.append(m);
        lattice.append(o);
        lattice.append(m);
        lattice.append(b);
        lattice.append(m);
        lattice.append(o);
        lattice.append(m);
        lattice.append(d);
        lattice.append(m);
        lattice.append(o);
        lattice.append(m);
        lattice.append(b);
        lattice.append(m);
        lattice.append(o);
    }
    ~Fobodobo_rbend_markers_fixture()
    {
        BOOST_TEST_MESSAGE("teardown fixture");
    }

    Lattice lattice;
    Four_momentum four_momentum;
    Reference_particle reference_particle;
};

BOOST_FIXTURE_TEST_CASE(construct, Fodo_fixture)
{
    Chef_lattice chef_lattice(lattice);
}

BOOST_FIXTURE_TEST_CASE(construct2, Fodo_fixture)
{
    Chef_lattice chef_lattice(lattice,
            get_standard_lattice_element_to_chef_fn_map());
}

BOOST_FIXTURE_TEST_CASE(get_beamline_sptr, Fodo_fixture)
{
    Chef_lattice chef_lattice(lattice);
    BmlPtr beamline_sptr = chef_lattice.get_beamline_sptr();
    std::cout << "\nchef fodo\n";
    print_chef_beamline(beamline_sptr);
    propagate_reference_particle(lattice.get_reference_particle(),
            beamline_sptr);
    // Not much of a test!!!
}

BOOST_FIXTURE_TEST_CASE(get_beamline_sptr_sbends, Fobodobo_sbend_fixture)
{
    Chef_lattice chef_lattice(lattice);
    BmlPtr beamline_sptr = chef_lattice.get_beamline_sptr();
    std::cout << "\nchef fobodobo sbend\n";
    print_chef_beamline(beamline_sptr);
    propagate_reference_particle(lattice.get_reference_particle(),
            beamline_sptr);
    for (Lattice_elements::iterator it = lattice.get_elements().begin(); it
            != lattice.get_elements().end(); ++it) {
        std::cout << (*it)->get_name() << " -> ";
        Chef_elements chef_elements(chef_lattice.get_chef_elements(*(*it)));
        std::cout << chef_elements.size() << " elements: ";
        for (Chef_elements::const_iterator cit = chef_elements.begin(); cit
                != chef_elements.end(); ++cit) {
            std::cout << (*cit)->Name() << " ";
        }
        std::cout << std::endl;
    }
    // Not much of a test!!!
}

BOOST_FIXTURE_TEST_CASE(get_beamline_sptr_sbends_markers,
        Fobodobo_sbend_markers_fixture)
{
    Chef_lattice chef_lattice(lattice);
    BmlPtr beamline_sptr = chef_lattice.get_beamline_sptr();
    std::cout << "\nchef fobodobo sbend with markers\n";
    print_chef_beamline(beamline_sptr);
    propagate_reference_particle(lattice.get_reference_particle(),
            beamline_sptr);
    // Not much of a test!!!
}

BOOST_FIXTURE_TEST_CASE(get_beamline_sptr_rbends, Fobodobo_rbend_fixture)
{
    Chef_lattice chef_lattice(lattice);
    BmlPtr beamline_sptr = chef_lattice.get_beamline_sptr();
    std::cout << "\nchef fobodobo rbend\n";
    print_chef_beamline(beamline_sptr);
    propagate_reference_particle(lattice.get_reference_particle(),
            beamline_sptr);
    for (Lattice_elements::iterator it = lattice.get_elements().begin(); it
            != lattice.get_elements().end(); ++it) {
        std::cout << (*it)->get_name() << " -> ";
        Chef_elements chef_elements(chef_lattice.get_chef_elements(*(*it)));
        std::cout << chef_elements.size() << " elements: ";
        for (Chef_elements::const_iterator cit = chef_elements.begin(); cit
                != chef_elements.end(); ++cit) {
            std::cout << (*cit)->Name() << " ";
        }
        std::cout << std::endl;
    }
    // Not much of a test!!!
}

BOOST_FIXTURE_TEST_CASE(get_beamline_sptr_rbends_markers,
        Fobodobo_rbend_markers_fixture)
{
    Chef_lattice chef_lattice(lattice);
    BmlPtr beamline_sptr = chef_lattice.get_beamline_sptr();
    std::cout << "\nchef fobodobo rbend with markers\n";
    print_chef_beamline(beamline_sptr);
    propagate_reference_particle(lattice.get_reference_particle(),
            beamline_sptr);
    // Not much of a test!!!
}
