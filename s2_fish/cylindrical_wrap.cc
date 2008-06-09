#include "cylindrical.h"
#include "field_domain.h"
#undef _POSIX_C_SOURCE
#include <boost/python.hpp>
#include "array_nd/array_nd_python.h"
#include "container_conversions.h"

using namespace boost::python;

#include <iostream>

void 
get_cylindrical_coords_wrapper(Macro_bunch_store &mbs, 
    object &coords)
{
    Array_2d<double> coords_array = 
        Array_nd_from_PyObject<double>(coords.ptr());
    get_cylindrical_coords(mbs,coords_array);
}

void
deposit_charge_cic_cylindrical_wrapper(const Field_domain &fdomain, 
    object &rho , Macro_bunch_store& mbs,
    const object &coords)
{
    Array_3d<double > rho_array = 
        Array_nd_from_PyObject<double>(rho.ptr());
    Array_2d<double> coords_array = 
        Array_nd_from_PyObject<double>(coords.ptr());
    deposit_charge_cic_cylindrical(fdomain, rho_array, mbs, coords_array);
}

void 
solve_cylindrical_finite_periodic_wrapper(const Field_domain &fdomain,
    object &rho, object &phi)
{
    Array_3d<double > rho_array = 
        Array_nd_from_PyObject<double>(rho.ptr());
    Array_3d<double > phi_array = 
        Array_nd_from_PyObject<double>(phi.ptr());
    solve_cylindrical_finite_periodic(fdomain, rho_array, phi_array);
}

void
solve_tridiag_nonsym_wrapper(object &diag,
                             object &abovediag,
                             object &belowdiag,
                             object &rhs,
                             object &x)
{
    Array_1d<std::complex<double> > diag_array = 
        Array_nd_from_PyObject<std::complex<double> >(diag.ptr());
    Array_1d<std::complex<double> > abovediag_array = 
        Array_nd_from_PyObject<std::complex<double> >(abovediag.ptr());
    Array_1d<std::complex<double> > belowdiag_array = 
        Array_nd_from_PyObject<std::complex<double> >(belowdiag.ptr());
    Array_1d<std::complex<double> > rhs_array = 
        Array_nd_from_PyObject<std::complex<double> >(rhs.ptr());
    Array_1d<std::complex<double> > x_array = 
        Array_nd_from_PyObject<std::complex<double> >(x.ptr());
    solve_tridiag_nonsym(diag_array,abovediag_array,belowdiag_array,
        rhs_array,x_array);
}

void
full_kick_cylindrical_wrapper(const Field_domain &fdomain,
                              object &phi, double tau,
                              Macro_bunch_store &mbs, object &coords)
{
    Array_3d<double> phi_array = Array_nd_from_PyObject<double>(phi.ptr());
    Array_2d<double> coords_array = Array_nd_from_PyObject<double>(coords.ptr());
    full_kick_cylindrical(fdomain,phi_array,tau,mbs,coords_array);
}

BOOST_PYTHON_MODULE(s2_solver_cylindrical)
{    
    //---------------------------------------------------------------------
    // std::vector<> conversions
    //---------------------------------------------------------------------
    //~ scitbx::boost_python::container_conversions::from_python_sequence <
    //~ std::vector<int>,
    //~ scitbx::boost_python::container_conversions::variable_capacity_policy > ();

    //~ boost::python::to_python_converter <
    //~ std::vector<int>,
    //~ scitbx::boost_python::container_conversions::to_tuple <
    //~ std::vector<int> > > ();

    //~ scitbx::boost_python::container_conversions::from_python_sequence <
    //~ std::vector<double>,
    //~ scitbx::boost_python::container_conversions::variable_capacity_policy > ();

    //~ boost::python::to_python_converter <
    //~ std::vector<double>,
    //~ scitbx::boost_python::container_conversions::to_tuple <
    //~ std::vector<double> > > ();

    scitbx::boost_python::container_conversions::from_python_sequence <
    std::vector<bool>,
    scitbx::boost_python::container_conversions::variable_capacity_policy > ();

    boost::python::to_python_converter <
    std::vector<bool>,
    scitbx::boost_python::container_conversions::to_tuple <
    std::vector<bool> > > ();

    class_<Field_domain>("Field_domain", init<>())
    .def(init<const std::vector<double> &,
        const std::vector<double> &,
        const std::vector<int> &,
        const std::vector<bool> &>())
    .def("set_params",&Field_domain::set_params)
    .def("get_grid_shape",&Field_domain::get_grid_shape)
    .def("get_cell_size",&Field_domain::get_cell_size)
    .def("get_periodic",&Field_domain::get_periodic)
        //~ .def("get_leftmost_indices_offsets",&Field_domain::get_leftmost_indices_offsets)
    ;
        
    def("get_cylindrical_coords",get_cylindrical_coords_wrapper);
    def("deposit_charge_cic_cylindrical",
        deposit_charge_cic_cylindrical_wrapper);
    def("solve_cylindrical_finite_periodic",
        solve_cylindrical_finite_periodic_wrapper);
    def("full_kick_cylindrical",full_kick_cylindrical_wrapper);
    // OK, solve_tridiag_nonsym was not designed to be wrapped. For the time
    // being, however, we need to wrap it in order to test it.
    def("solve_tridiag_nonsym",solve_tridiag_nonsym_wrapper);
}

