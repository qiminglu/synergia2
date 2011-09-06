#include "bunch.h"
#include "diagnostics.h"
#include "multi_diagnostics.h"
#include "populate.h"
#include "analysis.h"
#include <boost/python.hpp>
#include "synergia/utils/numpy_multi_ref_converter.h"
#include "synergia/utils/comm_converter.h"

using namespace boost::python;

PyObject *
bunch_get_comm_workaround(Bunch const& bunch)
{
    PyObject *retval;
    retval = PyMPIComm_New(bunch.get_comm().get());
    return retval;
}

BOOST_PYTHON_MODULE(bunch)
{
    import_array();
    if (import_mpi4py() < 0) {
        return;
    }

    class_<Fixed_t_z_converter, boost::noncopyable > ("Fixed_t_z_converter",
            no_init);
    class_<Fixed_t_z_zeroth, bases<Fixed_t_z_converter > > ("Fixed_t_z_zeroth",
            init< > ());
    class_<Fixed_t_z_ballistic, bases<Fixed_t_z_converter > > (
            "Fixed_t_z_ballistic", init< > ());

    class_<Diagnostics, Diagnostics_sptr, boost::noncopyable >
        ("Diagnostics", no_init)
        .def("update", &Diagnostics::update)
        .def("write", &Diagnostics::write)
        .def("update_and_write", &Diagnostics::update_and_write)
        ;

    class_<Diagnostics_basic, Diagnostics_basic_sptr, bases<Diagnostics > >
        ("Diagnostics_basic",init<Bunch_sptr, std::string const& >())
        .def("get_s", &Diagnostics_basic::get_s)
        .def("get_repetition", &Diagnostics_basic::get_repetition)
        .def("get_trajectory_length", &Diagnostics_basic::get_trajectory_length)
        .def("get_mean", &Diagnostics_basic::get_mean)
        .def("get_std", &Diagnostics_basic::get_std)
        ;

    class_<Diagnostics_full2, Diagnostics_full2_sptr, bases<Diagnostics > >
            ("Diagnostics_full2",init<Bunch_sptr, std::string const& >())
        .def("get_s", &Diagnostics_full2::get_s)
        .def("get_repetition", &Diagnostics_full2::get_repetition)
        .def("get_trajectory_length", &Diagnostics_full2::get_trajectory_length)
        .def("get_mean", &Diagnostics_full2::get_mean)
        .def("get_std", &Diagnostics_full2::get_std)
        .def("get_mom2", &Diagnostics_full2::get_mom2)
        .def("get_corr", &Diagnostics_full2::get_corr)
        .def("get_emitx", &Diagnostics_full2::get_emitx)
        .def("get_emity", &Diagnostics_full2::get_emity)
        .def("get_emitz", &Diagnostics_full2::get_emitz)
        .def("get_emitxy", &Diagnostics_full2::get_emitxy)
        .def("get_emitxyz",&Diagnostics_full2::get_emitxyz)
        ;

    class_<Diagnostics_track, Diagnostics_track_sptr, bases<Diagnostics > >
        ("Diagnostics_track",init<Bunch_sptr, std::string const&, int >())
        ;

    class_<Diagnostics_particles, Diagnostics_particles_sptr, bases<Diagnostics > >
            ("Diagnostics_particles",init<Bunch_sptr, std::string const& >())
        .def(init<Bunch_sptr, std::string const&, int >())
        .def(init<Bunch_sptr, std::string const&, int, int >())
        ;

    class_<Multi_diagnostics>("Multi_diagnostics",
            init<>())
        .def("append", &Multi_diagnostics::append)
        ;

    class_<Analysis>("Analysis", init<std::string const&, int>())
      .def("get_betatron_averaged_tune", &Analysis::get_betatron_averaged_tune)
      .def("get_betatron_tune_spread", &Analysis::get_betatron_tune_spread)
      .def("get_betatron_tunes", &Analysis::get_betatron_tunes)
      .def("get_XCoords_forTunes", &Analysis::get_XCoords_forTunes)
      .def("get_YCoords_forTunes", &Analysis::get_YCoords_forTunes)
      .def("get_num_betatron_tune_found", &Analysis::get_num_betatron_tune_found)
      .def("set_minimum_required_turn_Num", &Analysis::set_minimum_required_turn_Num)
      .def("get_minimum_required_turn_Num", &Analysis::get_minimum_required_turn_Num)
      .def("get_num_turns", &Analysis::get_num_turns)
      .def("get_num_part_first_bunch", &Analysis::get_num_part_first_bunch)
      .def("get_transverse_action_for_particle", &Analysis::get_transverse_action_for_particle)
      .def("get_transverse_action_for_bunch", &Analysis::get_transverse_action_for_bunch)
    ;
    
    def("no_diagnostics", no_diagnostics);
    def("populate_6d", populate_6d);
    def("populate_transverse_gaussian", populate_transverse_gaussian);
    def("populate_uniform_cylinder", populate_uniform_cylinder);
    def("populate_transverse_KV_GaussLong", populate_transverse_KV_GaussLong);

    typedef Reference_particle & (Bunch::*get_reference_particle_non_const_type)();
    typedef MArray2d_ref (Bunch::*get_local_particles_non_const_type)();
    scope
        Bunch_scope =
            class_<Bunch > ("Bunch", init<Reference_particle const&,
                    int, double, Commxx const& > ())
                .def(init<Reference_particle const&, int, double,
                        Commxx const&, int >())
                .def(init<Reference_particle const&, int, double,
                        Commxx const&, double >())
                .def("set_particle_charge", &Bunch::set_particle_charge)
                .def("set_real_num", &Bunch::set_real_num)
                .def("set_local_num", &Bunch::set_local_num)
                .def("update_total_num", &Bunch::update_total_num)
                .def("set_converter", &Bunch::set_converter)
                .def("convert_to_state", &Bunch::convert_to_state)
                .def("get_reference_particle",
                        get_reference_particle_non_const_type(
                                &Bunch::get_reference_particle),
                        return_internal_reference< >())
                .def("get_local_particles",
                        get_local_particles_non_const_type(
                                &Bunch::get_local_particles))
                .def("get_particle_charge", &Bunch::get_particle_charge)
                .def("get_mass", &Bunch::get_mass)
                .def("get_real_num", &Bunch::get_real_num)
                .def("get_local_num", &Bunch::get_local_num)
                .def("get_total_num", &Bunch::get_total_num)
                .def("get_state", &Bunch::get_state)
                .def("get_z_period_length", &Bunch::get_z_period_length)
                .def("is_periodic", &Bunch::is_z_periodic)
                // jfa: the following implementation does not work for reasons I do
                //      not understand.
//                .def("get_comm",
//                            &Bunch::get_comm, return_internal_reference< > ())
                .def("get_comm",
                            &bunch_get_comm_workaround)
                .def("inject", &Bunch::inject)
                ;

    enum_<Bunch::State > ("State")
        .value("fixed_z", Bunch::fixed_z)
        .value("fixed_t", Bunch::fixed_t)
        .value("fixed_z_lab", Bunch::fixed_z_lab)
        .value("fixed_t_lab", Bunch::fixed_t_lab)
        .value("fixed_z_bunch", Bunch::fixed_z_bunch)
        .value("fixed_t_bunch", Bunch::fixed_t_bunch)
        .export_values();

    scope().attr("x") = Bunch::x;
    scope().attr("xp") = Bunch::xp;
    scope().attr("y") = Bunch::y;
    scope().attr("yp") = Bunch::yp;
    scope().attr("z") = Bunch::z;
    scope().attr("zp") = Bunch::zp;
    scope().attr("cdt") = Bunch::cdt;
    scope().attr("dpop") = Bunch::dpop;
    scope().attr("id") = Bunch::id;
}    
