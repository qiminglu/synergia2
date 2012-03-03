#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "synergia/simulation/lattice_simulator.h"
#include "lattice_fixture.h"
#include "synergia/utils/floating_point.h"
#include "synergia/utils/boost_test_mpi_fixture.h"
BOOST_GLOBAL_FIXTURE(MPI_fixture)

const double tolerance = 1.0e-12;
const int map_order = 2;

BOOST_FIXTURE_TEST_CASE(construct, Lattice_fixture)
{
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
}

BOOST_FIXTURE_TEST_CASE(set_slices, Lattice_fixture)
{
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    Lattice_element_slices slices;
    for (Lattice_elements::const_iterator it =
            lattice_sptr->get_elements().begin(); it
            != lattice_sptr->get_elements().end(); ++it) {
        double length = (*it)->get_length();
        Lattice_element_slice_sptr first_half(
                new Lattice_element_slice(*it, 0.0, 0.5 * length));
        Lattice_element_slice_sptr second_half(
                new Lattice_element_slice(*it, 0.5 * length, length));
        slices.push_back(first_half);
        slices.push_back(second_half);
    }
    lattice_simulator.set_slices(slices);
}

BOOST_FIXTURE_TEST_CASE(get_map_order, Lattice_fixture)
{
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);

    BOOST_CHECK_EQUAL(lattice_simulator.get_map_order(), map_order);
}

BOOST_FIXTURE_TEST_CASE(get_operation_extractor_map_sptr, Lattice_fixture)
{
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);

    std::list<std::string >
            names(
                    lattice_simulator.get_operation_extractor_map_sptr()->get_extractor_names());
    std::list<std::string > expected_names;
    expected_names.push_back(default_operation_extractor_name);
    expected_names.push_back(chef_mixed_operation_extractor_name);
    expected_names.push_back(chef_map_operation_extractor_name);
    expected_names.push_back(chef_propagate_operation_extractor_name);

    BOOST_CHECK_EQUAL(names.size(), expected_names.size());
    names.sort();
    expected_names.sort();
    for (std::list<std::string >::iterator it = names.begin(), expected_it =
            expected_names.begin(); it != names.end(); ++it, ++expected_it) {
        BOOST_CHECK_EQUAL((*it), (*expected_it));
    }

    BOOST_CHECK_EQUAL(lattice_simulator.get_map_order(), map_order);
}

BOOST_FIXTURE_TEST_CASE(get_lattice_sptr, Lattice_fixture)
{
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    lattice_simulator.get_lattice_sptr();
}

BOOST_FIXTURE_TEST_CASE(get_chef_lattice_sptr, Lattice_fixture)
{
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    lattice_simulator.get_chef_lattice_sptr();
}

BOOST_AUTO_TEST_CASE(update)
{
    const double quad_length = 0.2;
    const double quad_strength = 3.2;
    const double drift_length = 3.0;
    const double bend_length = 4.0;

    Lattice_element f("quadrupole", "f");
    f.set_double_attribute("l", quad_length);
    f.set_double_attribute("k1", quad_strength);
    Lattice_element o("drift", "o");
    o.set_double_attribute("l", drift_length);
    Lattice_element d("quadrupole", "d");
    d.set_double_attribute("l", quad_length);
    d.set_double_attribute("k1", quad_strength);

    Lattice_sptr lattice_sptr(new Lattice(name));
    lattice_sptr->append(f);
    lattice_sptr->append(o);
    lattice_sptr->append(d);
    lattice_sptr->append(o);

    const int charge = pconstants::proton_charge;
    const double mass = pconstants::mp;
    const double total_energy = 125.0;
    Four_momentum four_momentum(mass, total_energy);
    Reference_particle reference_particle(charge, four_momentum);
    lattice_sptr->set_reference_particle(reference_particle);

    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    Lattice_element_slices slices;
    for (Lattice_elements::const_iterator it =
            lattice_sptr->get_elements().begin(); it
            != lattice_sptr->get_elements().end(); ++it) {
        double length = (*it)->get_length();
        Lattice_element_slice_sptr first_half(
                new Lattice_element_slice(*it, 0.0, 0.5 * length));
        Lattice_element_slice_sptr second_half(
                new Lattice_element_slice(*it, 0.5 * length, length));
        slices.push_back(first_half);
        slices.push_back(second_half);
    }
    lattice_simulator.set_slices(slices);

    double orig_quad_strength;
    for (beamline::deep_iterator
            it =
                    lattice_simulator.get_chef_lattice_sptr()->get_sliced_beamline_sptr()->deep_begin(); it
            != lattice_simulator.get_chef_lattice_sptr()->get_sliced_beamline_sptr()->deep_end(); ++it) {
        if (std::string((*it)->Type()) == "quadrupole") {
            orig_quad_strength = (*it)->Strength();
        }
    }

    for (Lattice_elements::iterator it = lattice_sptr->get_elements().begin(); it
            != lattice_sptr->get_elements().end(); ++it) {
        if ((*it)->get_type() == "quadrupole") {
            (*it)->set_double_attribute("k1", 2 * quad_strength);
        }
    }

    lattice_simulator.update();

    double new_quad_strength;
    for (beamline::deep_iterator
            it =
                    lattice_simulator.get_chef_lattice_sptr()->get_sliced_beamline_sptr()->deep_begin(); it
            != lattice_simulator.get_chef_lattice_sptr()->get_sliced_beamline_sptr()->deep_end(); ++it) {
        if (std::string((*it)->Type()) == "quadrupole") {
            new_quad_strength = (*it)->Strength();
        }
    }
    BOOST_CHECK_CLOSE(new_quad_strength, 2*orig_quad_strength, tolerance);
}

BOOST_FIXTURE_TEST_CASE(calculate_element_lattice_functions, Fobodobo_sbend_fixture)
{
    const int map_order = 1;
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    lattice_simulator.calculate_element_lattice_functions();
}

BOOST_FIXTURE_TEST_CASE(calculate_slice_lattice_functions, Fobodobo_sbend_fixture)
{
    const int map_order = 1;
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    Lattice_element_slices slices;
    for (Lattice_elements::const_iterator it =
            lattice_sptr->get_elements().begin(); it
            != lattice_sptr->get_elements().end(); ++it) {
        double length = (*it)->get_length();
        Lattice_element_slice_sptr first_half(
                new Lattice_element_slice(*it, 0.0, 0.5 * length));
        Lattice_element_slice_sptr second_half(
                new Lattice_element_slice(*it, 0.5 * length, length));
        slices.push_back(first_half);
        slices.push_back(second_half);
    }
    lattice_simulator.set_slices(slices);
    lattice_simulator.calculate_slice_lattice_functions();
}

BOOST_FIXTURE_TEST_CASE(get_element_lattice_functions, Fobodobo_sbend_fixture)
{
    const int map_order = 1;
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    for (Lattice_elements::iterator it = lattice_sptr->get_elements().begin(); it
            != lattice_sptr->get_elements().end(); ++it) {
        Lattice_functions f(lattice_simulator.get_lattice_functions(*(*it)));
    }
}

BOOST_FIXTURE_TEST_CASE(get_slice_lattice_functions, Fobodobo_sbend_fixture)
{
    const int map_order = 1;
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    Lattice_element_slices slices;
    for (Lattice_elements::const_iterator it =
            lattice_sptr->get_elements().begin(); it
            != lattice_sptr->get_elements().end(); ++it) {
        double length = (*it)->get_length();
        Lattice_element_slice_sptr first_half(
                new Lattice_element_slice(*it, 0.0, 0.5 * length));
        Lattice_element_slice_sptr second_half(
                new Lattice_element_slice(*it, 0.5 * length, length));
        slices.push_back(first_half);
        slices.push_back(second_half);
    }
    lattice_simulator.set_slices(slices);
    for (Lattice_element_slices::iterator it = slices.begin(); it
            != slices.end(); ++it) {
        Lattice_functions f(lattice_simulator.get_lattice_functions(*(*it)));
    }
}

BOOST_FIXTURE_TEST_CASE(get_tunes, Fobodobo_sbend_fixture)
{
    const int map_order = 1;
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    const double tolerance = 1.0e-3;
    const double expected_horizontal_tune = 0.70859;
    const double expected_vertical_tune = 0.00865009;
    BOOST_CHECK_CLOSE(lattice_simulator.get_horizontal_tune(),
            expected_horizontal_tune, tolerance);
    BOOST_CHECK_CLOSE(lattice_simulator.get_vertical_tune(),
            expected_vertical_tune, tolerance);
}

BOOST_FIXTURE_TEST_CASE(adjust_tunes, Fobodobo_sbend_fixture)
{
    const int map_order = 1;
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);

    Lattice_elements horizontal_correctors, vertical_correctors;
    for (Lattice_elements::iterator it = lattice_sptr->get_elements().begin(); it
            != lattice_sptr->get_elements().end(); ++it) {
        if ((*it)->get_type() == "quadrupole") {
            if ((*it)->get_double_attribute("k1") > 0.0) {
                horizontal_correctors.push_back(*it);
            } else {
                vertical_correctors.push_back(*it);
            }
        }
    }
    const double new_horizontal_tune = 0.69;
    const double new_vertical_tune = 0.15;
    const double tolerance = 1.0e-6;
    lattice_simulator.adjust_tunes(new_horizontal_tune, new_vertical_tune,
            horizontal_correctors, vertical_correctors, tolerance);
    BOOST_CHECK(std::abs(lattice_simulator.get_horizontal_tune() -
                    new_horizontal_tune) < tolerance);
    BOOST_CHECK(std::abs(lattice_simulator.get_vertical_tune() -
                    new_vertical_tune) < tolerance);
}

BOOST_FIXTURE_TEST_CASE(get_linear_one_turn_map, Foborodobo32_fixture)
{
  const int map_order = 5;
  const double tolerance = 1.0e-10;
  Lattice_simulator lattice_simulator(lattice_sptr, map_order);

  const double precalc_map[6][6] =
    {
      {-2.19357726128732,32.9385414827834,0,0,-5.62169337392918e-05,2.1037055586748},
      {-0.198001573221548,2.51726768373267,0,0,-3.53019959335299e-05,0.225092380126584},
      {0,0,1.07033464770303,1.26550130626506,0,0},
      {0,0,-0.043725938974272,0.882588234565397,0,0},
      {-0.077644019330161,2.12631144692458,0,0,0.996935702805962,4.9072335958152},
      {-1.78674162102745e-05,-0.000311185657541453,0,0,-0.000628318530717954,1.00004300477563}
    };

  MArray2d gotten_map(lattice_simulator.get_linear_one_turn_map());
  for (int i=0; i<6; ++i) {
    for (int j=0; j<6; ++j) {
      BOOST_CHECK(floating_point_equal(gotten_map[i][j], precalc_map[i][j],tolerance));
    }
  }
}

BOOST_FIXTURE_TEST_CASE(is_ring, Foborodobo32_fixture)
{
  const int map_order = 5;
  Lattice_simulator lattice_simulator(lattice_sptr, map_order);

  BOOST_CHECK(lattice_simulator.is_ring());
}

BOOST_FIXTURE_TEST_CASE(linear_human_normal_human, Foborodobo32_fixture)
{
  const int map_order = 3;
  const double tolerance = 1.0e-12;
  Lattice_simulator lattice_simulator(lattice_sptr, map_order);

  // in the linear case, this should just be a matrix multiplication
  // and be very good

    // fill the bunch with three points each direction
    const double test_points[] = {1.0e-3, 1.0e-4, 1.0e-3, 1.0e-4, 0.1, 0.1/200};

    int pidx=0;
#if 1
    const int num_macro_particles = 3*3*3*3*3*3;
    MArray2d particles(boost::extents[num_macro_particles][7]);
    for (int i0= -1; i0!=2; ++i0) {
      for (int i1= -1; i1!=2; ++i1) {
	for (int i2= -1; i2!=2; ++i2) {
	  for (int i3= -1; i3!=2; ++i3) {
	    for (int i4= -1; i4!=2; ++i4) {
	      for (int i5= -1; i5!=2; ++i5) {
		particles[pidx][0] = test_points[0]*i0;
		particles[pidx][1] = test_points[1]*i1;
		particles[pidx][2] = test_points[2]*i2;
		particles[pidx][3] = test_points[3]*i3;
		particles[pidx][4] = test_points[4]*i4;
		particles[pidx][5] = test_points[5]*i5;
		++pidx;
	      }
	    }
	  }
	}
      }
    }
#else
    const int num_macro_particles = 1;
    MArray2d particles(boost::extents[num_macro_particles][7]);

    for (int i=0; i<num_macro_particles; ++i) {
      for (int j=0; j<6; ++j) {
	particles[i][j] = 0.0;
      }
      particles[i][pidx] = test_points[pidx];
      ++pidx;
    }
#endif

    MArray2d particles_orig(particles);

#if 0
    std::cout << "particles[0][]: " <<
      particles[0][0] << ", " << particles[0][1] << ", " << particles[0][2] << ", " << particles[0][3] << ", " << particles[0][4] << ", " << particles[0][5] << std::endl;
#endif

    lattice_simulator.convert_human_to_normal(particles);

#if 0
    std::cout << "normal particles[0][]: " <<
      particles[0][0] << ", " << particles[0][1] << ", " << particles[0][2] << ", " << particles[0][3] << ", " << particles[0][4] << ", " << particles[0][5] << std::endl;
#endif

    lattice_simulator.convert_normal_to_human(particles);

#if 0
    std::cout << "human particles[0][]: " <<
      particles[0][0] << ", " << particles[0][1] << ", " << particles[0][2] << ", " << particles[0][3] << ", " << particles[0][4] << ", " << particles[0][5] << std::endl;
#endif

    for (int i=0; i<num_macro_particles; ++i) {
      for (int j=0; j<6; ++j) {
	BOOST_CHECK(floating_point_equal(particles[i][j], particles_orig[i][j], tolerance));
      }
    }
}

BOOST_FIXTURE_TEST_CASE(normal_human_normal, Foborodobo32_fixture)
{
  const int map_order = 3;
  const double tolerance = 1.0e-12;
  Lattice_simulator lattice_simulator(lattice_sptr, map_order);

  // fill the bunch with three points at fixed action each
  // direction but angles uniformly spread around 2*pi

  const int n_angles = 24;
  // this is the square-root of the action
  const double test_actions[] = {1.0e-4, 1.0e-4, 1.0e-4};

    int pidx=0;

    const int num_macro_particles = n_angles*n_angles*n_angles;
    MArray2d particles(boost::extents[num_macro_particles][7]);

    for (int iph0=0; iph0<n_angles; ++iph0) {
      double phase0 = (2.0*mconstants::pi/(2.0*n_angles)) * (2*iph0+1);
      for (int iph1=0; iph1<n_angles; ++iph1) {
	double phase1 = (2.0* mconstants::pi/(2.0*n_angles)) * (2*iph1+1);
	for (int iph2=0; iph2<n_angles; ++iph2) {
	  double phase2 = (2.0*mconstants::pi/(2.0*n_angles)) * (2*iph2+1);
	  particles[pidx][0] = test_actions[0]*sin(phase0);
	  particles[pidx][1] = -test_actions[0]*cos(phase0);
	  particles[pidx][2] = test_actions[1]*sin(phase1);
	  particles[pidx][3] = -test_actions[1]*cos(phase1);
	  particles[pidx][4] = test_actions[2]*sin(phase2);
	  particles[pidx][5] = -test_actions[2]*cos(phase2);
	  ++pidx;
	}
      }
    }
    MArray2d particles_orig(particles);

    lattice_simulator.convert_normal_to_human(particles);
    lattice_simulator.convert_human_to_normal(particles);

    for (int i=0; i<num_macro_particles; ++i) {
      for (int j=0; j<6; ++j) {
	BOOST_CHECK(floating_point_equal(particles[i][j], particles_orig[i][j], tolerance));
      }
    }
}

BOOST_FIXTURE_TEST_CASE(check_linear_normal_form, Foborodobo32_fixture)
{
  const int map_order = 3;
  const double tolerance = 1.0e-10;
  Lattice_simulator lattice_simulator(lattice_sptr, map_order);

  BOOST_CHECK(lattice_simulator.check_linear_normal_form());
}

BOOST_FIXTURE_TEST_CASE(serialize_xml, Lattice_fixture)
{
    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
    xml_save(lattice_simulator, "lattice_simulator1.xml");

//    Lattice_simulator loaded;
//    xml_load(loaded, "lattice_simulator1.xml");
}

//BOOST_FIXTURE_TEST_CASE(serialize_sliced_chef_beamline, Lattice_fixture)
//{
//    Lattice_simulator lattice_simulator(lattice_sptr, map_order);
//    Lattice_element_slices slices;
//    for (Lattice_elements::const_iterator it =
//            lattice_sptr->get_elements().begin(); it
//            != lattice_sptr->get_elements().end(); ++it) {
//        double length = (*it)->get_length();
//        Lattice_element_slice_sptr first_half(
//                new Lattice_element_slice(*it, 0.0, 0.5 * length));
//        Lattice_element_slice_sptr second_half(
//                new Lattice_element_slice(*it, 0.5 * length, length));
//        slices.push_back(first_half);
//        slices.push_back(second_half);
//    }
//    lattice_simulator.set_slices(slices);
//
//    xml_save(lattice_simulator, "lattice_simulator2.xml");
//
//    Lattice_simulator loaded;
//    xml_load(loaded, "lattice_simulator2.xml");
//}
