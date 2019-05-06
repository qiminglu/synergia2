#ifndef LATTICE_SIMULATOR_H_
#define LATTICE_SIMULATOR_H_

#include <string>

#include "synergia/utils/multi_array_typedefs.h"
#include "synergia/lattice/lattice.h"
#include "synergia/lattice/chef_lattice.h"
#include "synergia/simulation/operation_extractor.h"
#include "synergia/simulation/aperture_operation_extractor.h"
#include "synergia/simulation/step.h"
#include <physics_toolkit/LattFuncSage.h>
#include <physics_toolkit/EdwardsTengSage.h>
#include <physics_toolkit/LBSage.h>
#include <physics_toolkit/DispersionSage.h>
#include <physics_toolkit/BeamlineContext.h>
#include <physics_toolkit/normalFormSage.h>

#include <string>
#include "synergia/utils/serialization.h"

typedef boost::shared_ptr<normalFormSage > Normal_form_sage_sptr; // syndoc:include

void map_to_twiss(Const_MArray2d_ref one_turn_map, double &alpha, double& beta, double& psi);
void map_to_twiss(Const_MArray2d_view one_turn_map, double &alpha, double& beta, double& psi);

struct Lattice_functions
{
    Lattice_functions();
    Lattice_functions(LattFuncSage::lattFunc const& latt_func);
    Lattice_functions(Const_MArray2d_ref one_turn_map);
    double alpha_x, alpha_y;
    double beta_x, beta_y;
    double psi_x, psi_y;
    double D_x, D_y;
    double Dprime_x, Dprime_y;
    double arc_length;
    template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version);
};

struct Long_lattice_functions
{
	Long_lattice_functions();
    Long_lattice_functions(Const_MArray2d_ref one_turn_map);
    double alpha; // should really be 0 for longitudinal
    double beta;
    double psi;
};

struct ET_lattice_functions
{
    ET_lattice_functions();
    ET_lattice_functions(EdwardsTengSage::Info const& ET_Info);
    double beta_x;
    double beta_y;
    double alpha_x;
    double alpha_y;
    double phi;
    double arc_length;
    template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version);

};

struct LB_lattice_functions
{
    LB_lattice_functions();
    LB_lattice_functions(LBSage::Info const& LB_Info);
    double beta_1x;
    double beta_1y;
    double beta_2x;
    double beta_2y;
    double alpha_1x;
    double alpha_1y;
    double alpha_2x;
    double alpha_2y;
    double u1;
    double u2;
    double u3;
    double u4;
    double nu_1;
    double nu_2;
    double arc_length;
    template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version);
};

struct Dispersion_functions
{
    Dispersion_functions();
    Dispersion_functions(DispersionSage::Info const& Disp_Info);
    double dispersion_x;
    double dispersion_y;
    double dPrime_x;
    double dPrime_y;
    double closedOrbit_x;
    double closedOrbit_y;
    double closedOrbitP_x;
    double closedOrbitP_y;
    double arc_length;
    template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version);
};

class Lattice_simulator
{
private:
    Lattice_sptr lattice_sptr;

    Lattice_element_slices slices;
    bool have_slices;

    Chef_lattice_sptr chef_lattice_sptr;

    Operation_extractor_map_sptr extractor_map_sptr;
    Aperture_operation_extractor_map_sptr aperture_extractor_map_sptr;

    int map_order;
    double bucket_length; /// bucket length  lattice_length/harmon
    bool have_element_lattice_functions;
    bool have_slice_lattice_functions;
    bool have_element_et_lattice_functions;
    bool have_slice_et_lattice_functions;
    bool have_element_lb_lattice_functions;
    bool have_slice_lb_lattice_functions;
    bool have_element_dispersion;
    bool have_slice_dispersion;
    double horizontal_tune, vertical_tune;
    bool have_tunes;
    double horizontal_chromaticity, vertical_chromaticity;
    double horizontal_chromaticity_prime, vertical_chromaticity_prime;
    bool have_chromaticities;
    double alt_horizontal_chromaticity, alt_vertical_chromaticity;
    bool have_alt_chromaticities;
    double closed_orbit_length;
    double rf_bucket_length;///rf  bucket length  beta*c/freq
    bool have_close_orbit_registered;
    double momentum_compaction, slip_factor, slip_factor_prime;
   // MArray2d linear_one_turn_map;
    std::map<Lattice_element *, Lattice_functions > lattice_functions_element_map;
    std::map<Lattice_element_slice *, Lattice_functions > lattice_functions_slice_map;
    std::map<Lattice_element *, ET_lattice_functions > et_lattice_functions_element_map;
    std::map<Lattice_element_slice *, ET_lattice_functions > et_lattice_functions_slice_map;
    std::map<Lattice_element *, LB_lattice_functions > lb_lattice_functions_element_map;
    std::map<Lattice_element_slice *, LB_lattice_functions > lb_lattice_functions_slice_map;
    std::map<Lattice_element *, Dispersion_functions > dispersion_element_map;
    std::map<Lattice_element_slice *, Dispersion_functions > dispersion_slice_map;
    void
    construct_extractor_map();
    void
    construct_aperture_extractor_map();
    BmlContextPtr
    get_beamline_context_clone();
    BmlContextPtr
    get_sliced_beamline_context_clone();
    void
    construct_sliced_chef_beamline();
    void
    get_tunes(bool use_eigen_tune);
    void
    calculate_normal_form(bool sliced);
    Normal_form_sage_sptr normal_form_sage_sptr;
    void
    get_chromaticities(double dpp);
    void
    get_alt_chromaticities(double dpp);

    MArray1d
    tune_rfcavities();

public:

    /// @param lattice_sptr the Lattice
    /// @param map_order order for Chef_map operations
    Lattice_simulator(Lattice_sptr lattice_sptr, int map_order);
    // Default constructor for serialization use only
    Lattice_simulator();
    Lattice_simulator(Lattice_simulator const& lattice_simulator);
    void
    set_slices(Lattice_element_slices const& slices);
    Lattice_element_slices const&
    get_slices() const;
    int
    get_map_order() const;
    void
    set_bucket_length();
    /// bucket length is in z_lab frame, lattice_length/harmon
    void
    set_rf_bucket_length();
    /// bucket length is in z_lab frame, beta*c/freq
    double
    get_bucket_length();
    double
    get_rf_bucket_length();
    int
    get_number_buckets();
    Operation_extractor_map_sptr
    get_operation_extractor_map_sptr();
    Aperture_operation_extractor_map_sptr
    get_aperture_operation_extractor_map_sptr();
    Lattice &
    get_lattice();
    Lattice_sptr
    get_lattice_sptr();
    Chef_lattice &
    get_chef_lattice();
    Chef_lattice_sptr
    get_chef_lattice_sptr();
    void
    update();
    void
    register_closed_orbit(bool sliced=true);
    MArray1d
    get_closed_orbit(double dpop = 0.0, bool sliced=true);
    double
    get_closed_orbit_length();
    double
    get_rf_frequency();
    void
    calculate_element_lattice_functions(); // Courant Snyder lattice functions
    void
    calculate_slice_lattice_functions(); // Courant Snyder lattice functions
    void
    calculate_element_et_lattice_functions(); // Edwards Teng lattice functions
    void
    calculate_slice_et_lattice_functions(); // Edwards Teng lattice functions
    void
    calculate_element_lb_lattice_functions(); // Lebedev Bogacz lattice functions
    void
    calculate_slice_lb_lattice_functions(); // Lebedev Bogacz lattice functions
    void
    calculate_element_dispersion_functions(); //claulate dispersion and closed orbit
    void
    calculate_slice_dispersion_functions(); //claulate dispersion and closed orbit
    Lattice_functions const&
    get_lattice_functions(Lattice_element & lattice_element);
    Lattice_functions const&
    get_lattice_functions(Lattice_element_slice & lattice_element_slice);
    ET_lattice_functions const&
    get_et_lattice_functions(Lattice_element & lattice_element);
    ET_lattice_functions const&
    get_et_lattice_functions(Lattice_element_slice & lattice_element_slice);
    LB_lattice_functions const&
    get_lb_lattice_functions(Lattice_element & lattice_element);
    LB_lattice_functions const&
    get_lb_lattice_functions(Lattice_element_slice & lattice_element_slice);
    Dispersion_functions const&
    get_dispersion_functions(Lattice_element & lattice_element);
    Dispersion_functions const&
    get_dispersion_functions(Lattice_element_slice & lattice_element_slice);
    void
    print_cs_lattice_functions(); // Courant Snyder
    void
    print_et_lattice_functions(); // Edwards Teng
    void
    print_lb_lattice_functions(); // Lebedev Bogacz
    void
    print_dispersion_closedOrbit();
    void
    print_lattice_functions(); // all (CS, LB, ET, Dispersion)
    std::pair<double, double >
    get_both_tunes(bool use_eigen_tune = false);
    double
    get_horizontal_tune(bool use_eigen_tune = false);
    double
    get_vertical_tune(bool use_eigen_tune = false);
    bool
    is_ring();
    Normal_form_sage_sptr
    get_normal_form_sptr(bool sliced=true);
    MArray2d
    get_linear_one_turn_map(bool sliced=true);
    void
    convert_xyz_to_normal(MArray2d_ref coords);
    void
    convert_normal_to_xyz(MArray2d_ref coords);
    bool
    check_linear_normal_form();
    std::vector<double >
    get_stationary_actions(const double stdx, const double stdy,
            const double std_cdt);
    void
    adjust_tunes(double horizontal_tune, double vertical_tune,
            Lattice_elements const& horizontal_correctors,
            Lattice_elements const& vertical_correctors, double tolerance =
                    1.0e-5, int verbosity = 0);
    void
    adjust_tunes_chef(double horizontal_tune, double vertical_tune,
            Lattice_elements const& horizontal_correctors,
            Lattice_elements const& vertical_correctors, int max_steps, double tolerance =
                    1.0e-4, int verbosity = 1);  
   void 
   change_tunesby(double dh, double dv,  Lattice_elements const& horizontal_correctors,
        Lattice_elements const& vertical_correctors, Logger & logger, int verbosity);                 
                    
    double
    get_slip_factor(double dpp=1.0e-5);
    MArray1d
    get_slip_factors(double dpp=1.0e-5);
    double
    get_momentum_compaction(double dpp=1.e-5);
    double
    get_horizontal_chromaticity(double dpp=1.e-5);
    MArray1d
    get_horizontal_chromaticities(double dpp=1.0e-5);
    double
    get_vertical_chromaticity(double dpp=1.e-5);
    MArray1d
    get_vertical_chromaticities(double dpp=1.0e-5);
    double
    get_alt_horizontal_chromaticity(double dpp=1.e-5);
    double
    get_alt_vertical_chromaticity(double dpp=1.e-5);
    void
    adjust_chromaticities(double horizontal_chromaticity,
            double vertical_chromaticity,
            Lattice_elements const& horizontal_correctors,
            Lattice_elements const& vertical_correctors, double tolerance =
                    1.0e-4, int max_steps = 6);
    void 
    change_chromaticityby(double dh, double dv,  Lattice_elements const& horizontal_correctors,
        Lattice_elements const& vertical_correctors, Logger & logger, int verbosity);               

    // propagate the lattice reference particle through the lattice slices and set the 
    // reference c*t for each lattice slice after tuning.
    // note that all the rf cavities will be set to 0 strength during the tuning process
    MArray1d
    tune_linear_lattice();

    // find the closed orbit for the circular lattice, propagate the lattice reference 
    // particle through the lattice slices using the closed orbit, and set the reference 
    // c*t for each lattice slice after tuning.
    // return values is the state for calcualted closed orbit
    // note that all the rf cavities will be set to 0 strength during the tuning process
    MArray1d
    tune_circular_lattice(double tolerance=1.0e-13);

    template<class Archive>
        void
        save(Archive & ar, const unsigned int version) const;
    template<class Archive>
        void
        load(Archive & ar, const unsigned int version);

    BOOST_SERIALIZATION_SPLIT_MEMBER();

    ~Lattice_simulator();
};

#include "synergia/simulation/dense_mapping.h"
class Dense_mapping_calculator
{
private:
    std::map<Lattice_element *, Dense_mapping> element_map;
public:
    Dense_mapping_calculator(Lattice_simulator& lattice_simulator, bool closed_orbit);
    Dense_mapping get_dense_mappping(Lattice_element& lattice_element);
    ~Dense_mapping_calculator();
};

#endif /* LATTICE_SIMULATOR_H_ */
