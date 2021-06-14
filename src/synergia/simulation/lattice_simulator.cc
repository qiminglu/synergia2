
#ifdef __CUDA_ARCH__

// no implementations for CUDA arch

#else


#include "synergia/simulation/lattice_simulator.h"
#include "synergia/foundation/physical_constants.h"
#include "synergia/foundation/math_constants.h"

#include "synergia/simulation/propagator.h"
#include "synergia/simulation/independent_stepper_elements.h"

#include "synergia/utils/multi_array_conversions.h"


namespace Lattice_simulator
{
    static double closed_orbit_tolerance 
        = default_closed_orbit_tolerance;
}


void
Lattice_simulator::set_closed_orbit_tolerance(double tolerance)
{
    closed_orbit_tolerance = tolerance;
}

double
Lattice_simulator::get_closed_orbit_tolerance()
{
    return closed_orbit_tolerance;
}


std::array<double, 6>
Lattice_simulator::tune_linear_lattice(Lattice & lattice)
{
    return tune_rfcavities(lattice);
}

std::array<double, 6>
Lattice_simulator::tune_circular_lattice(Lattice & lattice)
{
    // calculate closed orbit
    auto state = calculate_closed_orbit(lattice, 0.0);
    lattice.get_reference_particle().set_state(state);

    return tune_rfcavities(lattice);
}

std::array<double, 6>
Lattice_simulator::tune_rfcavities(Lattice & lattice)
{
    // make a copy of the original lattice
    Lattice temp_lattice(lattice);
    auto & ref = temp_lattice.get_reference_particle();

    // set rfcavity volt to 0 on the copied lattice
    for (auto & ele : temp_lattice.get_elements())
    {
        if (ele.get_type() == element_type::rfcavity) 
            ele.set_double_attribute("volt", 0.0);
    }

    // setup the propagator
    Propagator propagator(temp_lattice, Independent_stepper_elements(1));

    // bunch simulator
    auto sim = Bunch_simulator::create_single_bunch_simulator(
            ref, Commxx().size(), 1e09);

    // 
    sim.get_bunch()
        .get_design_reference_particle()
        .set_state(ref.get_state());

    // propagate actions
    double accum_cdt = 0.0;
    sim.reg_prop_action_step_end([&accum_cdt](Bunch_simulator& sim, Lattice&, int, int, void*) { 
        accum_cdt += sim.get_bunch().get_design_reference_particle().get_state()[4]; 
    }, nullptr );

    // propagate to get the accumulated cdt
    Logger simlog(0, LoggerV::ERROR);
    propagator.propagate(sim, simlog, 1);

    // return the state
    auto state = sim.get_bunch().get_reference_particle().get_state();
    state[Bunch::cdt] = accum_cdt;

    // go back and set the frequency of the cavities based on the accumulated cdt
    double f = pconstants::c/accum_cdt;

    for (auto & ele : lattice.get_elements())
    {
        if (ele.get_type() == element_type::rfcavity) 
        {
            // set the frequency of the cavity if it doesn't already have one set and
            // there is a reasonable harmonic number
            if ( ele.get_double_attribute("freq", -1.0) <= 0.0 
                    && ele.get_double_attribute("harmon", -1.0) > 0.0 ) 
            {
                double harmon = ele.get_double_attribute("harmon");
                // MAD-X definition of frequency is MHz
                ele.set_double_attribute("freq", harmon*f*1.0e-6);
            }
        }
    }

    return state;
}


#include <gsl/gsl_vector.h>
#include <gsl/gsl_multiroots.h>
#include "synergia/libFF/ff_element.h"

namespace
{
    struct Closed_orbit_params
    {
        const double dpp;
        Lattice lattice;

        Closed_orbit_params(double dpp, Lattice lattice) 
            : dpp(dpp), lattice(lattice)
        {
            // turn off any RF cavities because they 
            // screw up the closed orbit calcation
            for (auto & ele : lattice.get_elements())
                if (ele.get_type() == element_type::rfcavity)
                    ele.set_double_attribute("volt", 0.0);
        }
    };

    int
    propagate_co_try(const gsl_vector *co_try, void *params, gsl_vector *co_results)
    {
        Closed_orbit_params *copp = static_cast<Closed_orbit_params *>(params);

        auto comm = Commxx();
        Bunch bunch(copp->lattice.get_reference_particle(), 
                comm.size(), 1.0e10, comm);

        auto lp = bunch.get_host_particles();

        // set phase space coordinates in bunch
        double orbit_start[4];
        orbit_start[0] = lp(0, 0) = gsl_vector_get(co_try, 0);
        orbit_start[1] = lp(0, 1) = gsl_vector_get(co_try, 1);
        orbit_start[2] = lp(0, 2) = gsl_vector_get(co_try, 2);
        orbit_start[3] = lp(0, 3) = gsl_vector_get(co_try, 3);

        lp(0, 4) = 0.0;
        lp(0, 5) = copp->dpp;

        // checkin
        bunch.checkin_particles();
        
        // propagate
        for(auto const& ele : copp->lattice.get_elements())
            FF_element::apply(ele, bunch);

        // checkout
        bunch.checkout_particles();

        gsl_vector_set(co_results, 0, lp(0, 0) - orbit_start[0]);
        gsl_vector_set(co_results, 1, lp(0, 1) - orbit_start[1]);
        gsl_vector_set(co_results, 2, lp(0, 2) - orbit_start[2]);
        gsl_vector_set(co_results, 3, lp(0, 3) - orbit_start[3]);

        return GSL_SUCCESS;
    }
}

std::array<double, 6> 
Lattice_simulator::calculate_closed_orbit(
        Lattice const& lattice, double dpp)
{
    // create params object, make a copy of the lattice
    Closed_orbit_params cop(dpp, lattice);

    const size_t ndim = 4; // solve closed orbit in x, xp, y, yp

    // init coordinates
    auto state = lattice.get_reference_particle().get_state();

    //const gsl_multiroot_fsolver_type * T = gsl_multiroot_fsolver_hybrid;
    const gsl_multiroot_fsolver_type * T = gsl_multiroot_fsolver_hybrids;

    //const gsl_multiroot_fsolver_type * T = gsl_multiroot_fsolver_dnewton;
    gsl_multiroot_fsolver * solver = gsl_multiroot_fsolver_alloc(T, ndim);

    // co_try are the coordinates of the closed orbit
    gsl_vector *co_try = gsl_vector_alloc(ndim);

    // initialize the closed orbit
    for (int i=0; i<ndim; ++i)
    {
        gsl_vector_set(co_try, i, 0.0);

        // or use the memory from last found closed orbit?
        //gsl_vector_set(co_try, i, state[i]);
    }

    gsl_multiroot_function F;
    F.f = &propagate_co_try;
    F.n = ndim;
    F.params = &cop;

    gsl_multiroot_fsolver_set(solver, &F, co_try);

    int niter=0;
    const int maxiter = 100;

    do 
    {
        int rc;
        rc = gsl_multiroot_fsolver_iterate(solver);

        switch(rc) 
        {
        case GSL_ENOPROG:
            throw std::runtime_error(
                    "Closed orbit solver unable to converge. "
                    "Is the tolerance too tight?");
            break;

        case GSL_EBADFUNC:
            throw std::runtime_error(
                    "Closed orbit solver failed to evaluate solution");
            break;

        default:
            break;
        }

        gsl_multiroot_fsolver_f(solver);

    } while ((gsl_multiroot_test_residual(solver->f, closed_orbit_tolerance) 
                == GSL_CONTINUE) && (++niter < maxiter));

    if (niter == maxiter) 
    {
        std::stringstream sstr;
        sstr << "Could not locate closed orbit after " 
             << maxiter << " iterations";

        throw std::runtime_error( sstr.str() );
    }

    std::array<double, 6> costate;
    gsl_vector *froots = gsl_multiroot_fsolver_root(solver);
    costate[0] = gsl_vector_get(froots, 0);
    costate[1] = gsl_vector_get(froots, 1);
    costate[2] = gsl_vector_get(froots, 2);
    costate[3] = gsl_vector_get(froots, 3);
    costate[4] = 0.0;
    costate[5] = cop.dpp;

    gsl_multiroot_fsolver_free(solver);
    gsl_vector_free(co_try);
    return costate;
}

#include "synergia/foundation/trigon.h"

std::array<double, 2>
filter_transverse_tunes(double const* jac);
 
// [tune_h, tune_v, c_delta_t]
std::array<double, 3>
Lattice_simulator::calculate_tune_and_cdt(
        Lattice const& lattice, double dpp)
{
    // trigon bunch
    using trigon_t = Trigon<double, 2, 6>;

    // get the reference particle
    auto const& ref = lattice.get_reference_particle();

    // closed orbit
    auto probe = Lattice_simulator::calculate_closed_orbit(lattice, dpp);

    // comm world
    Commxx comm;

    bunch_t<trigon_t> tb(ref, comm.size(), comm);
    bunch_t<double>   pb(ref, comm.size(), 1e9, comm);

    // design reference particle from the closed orbit
    auto ref_l = ref;
    ref_l.set_state(probe);
    tb.set_design_reference_particle(ref_l);
    pb.set_design_reference_particle(ref_l);

    auto tparts = tb.get_host_particles();
    auto pparts = pb.get_host_particles();

    // init value
    for(int i=0; i<6; ++i) 
    {
        tparts(0, i).set(probe[i], i);
        pparts(0, i) = probe[i];
    }

    // check in
    tb.checkin_particles();
    pb.checkin_particles();

    // init c_delta_t
    double c_delta_t = 0.0;

    // propagate trigon
    for(auto & ele : lattice.get_elements())
    {
        if (ele.get_type() == element_type::rfcavity)
        {
            Lattice_element dup = ele;
            dup.set_double_attribute("volt", 0.0);

            FF_element::apply(dup, tb);
            FF_element::apply(dup, pb);
        }
        else
        {
            FF_element::apply(ele, tb);
            FF_element::apply(ele, pb);
        }

        // cdt from reference particle
        c_delta_t += pb.get_design_reference_particle()
                       .get_state()[Bunch::cdt];
    }

    // checkout particles
    tb.checkout_particles();
    pb.checkout_particles();

    // cdt from actual particle
    c_delta_t += pparts(0, 4);

    // one-turn-map
    auto kjac = tb.get_jacobian(0);

    //auto jac = karray_to_matrix(kjac);
    //auto nus = filter_transverse_tunes(jac);
    auto nus = filter_transverse_tunes(kjac.data());

#if 0
    std::cout << "nus0 = " << nus[0] << "\n";
    std::cout << "nus1 = " << nus[1] << "\n";
    std::cout << "c_delta_t = " << c_delta_t << "\n";
#endif

    double tune_h = nus[0];
    double tune_v = nus[1];

    return {tune_h, tune_v, c_delta_t};
}


chromaticities_t
Lattice_simulator::get_chromaticities(
        Lattice const& lattice, double dpp)
{
    chromaticities_t chroms;

    auto ref = lattice.get_reference_particle();
    double gamma = ref.get_gamma();

    // tune = [tune_h, tune_v, cdt]
    auto tune_0  = calculate_tune_and_cdt(lattice, 0.0);
    auto tune_p  = calculate_tune_and_cdt(lattice, dpp);
    auto tune_m  = calculate_tune_and_cdt(lattice, -dpp);
    auto tune_pp = calculate_tune_and_cdt(lattice, 2.0*dpp);
    auto tune_mm = calculate_tune_and_cdt(lattice, -2.0*dpp);

    // five point stencil:
    // given function f(x) = a_0 + a_1*x + a_2*x**2 + a_3*x**3 + a_4*x**4
    // choose offset spacing d, calculate values
    // y++ = f(2*d)
    // y+ = f(d)
    // y0 = f(0)
    // y- = f(-d)
    // y-- = f(-2*d)
    // then you can easily calculate:
    // y0 = a_0
    // y+ - y- = 2*a_1*d + 2*a_3*d**3
    // y++ - y-- = 4*a_1*d + 16*a_3*d**3
    // y+ + y- - 2*y0 = 2*a_2*d**2 + 2*a_4*d**4
    // y++ + y-- - 2*y0 = 8*a_2*d**2 + 32*a_4*d**4
    // yielding expressions:
    // 8*(y+ - y-) - (y++ - y--) = 12*a_1*d
    // (y++ - y--) - 2*(y+ - y-) = 12*a_3*d**3
    // 16*(y+ + y- - 2*y_0) - (y++ + y-- - 2*y_0) = 24*a_2*d**2
    // (y++ + y-- - 2*y_0) - 4*(y+ + y- - 2*y_0) = 24*a_4*d**4

    double tune_h0 = tune_0[0];
    double tune_v0 = tune_0[1];
    double cT0 = tune_0[2];

    double tune_h_plus = tune_p[0];
    double tune_v_plus = tune_p[1];
    double c_delta_t_plus = tune_p[2];

    double tune_h_minus = tune_m[0];
    double tune_v_minus = tune_m[1];
    double c_delta_t_minus = tune_m[2];

    double tune_h_plusplus = tune_pp[0];
    double tune_v_plusplus = tune_pp[1];
    double c_delta_t_plusplus = tune_pp[2];

    double tune_h_minusminus = tune_mm[0];
    double tune_v_minusminus = tune_mm[1];
    double c_delta_t_minusminus = tune_mm[2];

#if 0
    std::cout << "tune_0: " << tune_0[0] << ", " << tune_0[1] << ", " << tune_0[2] << "\n";
    std::cout << "tune_p: " << tune_p[0] << ", " << tune_p[1] << ", " << tune_p[2] << "\n";
    std::cout << "tune_m: " << tune_m[0] << ", " << tune_m[1] << ", " << tune_m[2] << "\n";
    std::cout << "tune_pp: " << tune_pp[0] << ", " << tune_pp[1] << ", " << tune_pp[2] << "\n";
    std::cout << "tune_mm: " << tune_mm[0] << ", " << tune_mm[1] << ", " << tune_mm[2] << "\n";
#endif

    double a_h_chrom, b_h_chrom;
    a_h_chrom = 0.5*(tune_h_plus-tune_h_minus)/dpp;
    b_h_chrom = 0.25*(tune_h_plusplus-tune_h_minusminus)/dpp;
    double horizontal_chromaticity_alt=(4*a_h_chrom - b_h_chrom)/3.0;

    double a_v_chrom, b_v_chrom;
    a_v_chrom = 0.5*(tune_v_plus-tune_v_minus)/dpp;
    b_v_chrom = 0.25*(tune_v_plusplus-tune_v_minusminus)/dpp;
    double vertical_chromaticity_alt=(4*a_v_chrom - b_v_chrom)/3.0;

    double a_slip, b_slip;
    a_slip = 0.5*(c_delta_t_plus-c_delta_t_minus)/cT0 / dpp;
    b_slip = 0.25*(c_delta_t_plusplus-c_delta_t_minusminus)/cT0 / dpp;
    double slip_factor_alt =(4*a_slip-b_slip)/3.0;
    double cdtp_m_cdtm = c_delta_t_plus - c_delta_t_minus;
    double cdtp_p_cdtm_m2cdt0 = c_delta_t_plus + c_delta_t_minus - 2*cT0;
    double cdtpp_m_cdtmm = c_delta_t_plusplus - c_delta_t_minusminus;
    double cdtpp_p_cdtmm_m2cdt0 = c_delta_t_plusplus + c_delta_t_minusminus - 2*cT0;

    double qxp_m_qxm = tune_h_plus - tune_h_minus;
    double qxp_p_qxm_m2qx0 = tune_h_plus + tune_h_minus - 2*tune_h0;
    double qxpp_m_qxmm = tune_h_plusplus - tune_h_minusminus;
    double qxpp_p_qxmm_m2qx0 = tune_h_plusplus + tune_h_minusminus - 2*tune_h0;

    double qyp_m_qym = tune_v_plus - tune_v_minus;
    double qyp_p_qym_m2qy0 = tune_v_plus + tune_v_minus - 2*tune_v0;
    double qypp_m_qymm = tune_v_plusplus - tune_v_minusminus;
    double qypp_p_qymm_m2qy0 = tune_v_plusplus + tune_v_minusminus - 2*tune_v0;

    chroms.slip_factor 
        = (8.0*cdtp_m_cdtm - cdtpp_m_cdtmm)/(12.0*dpp*cT0);

    chroms.slip_factor_prime 
        = 2.0*(16*cdtp_p_cdtm_m2cdt0 - cdtpp_p_cdtmm_m2cdt0) /(24.0*dpp*dpp*cT0);

    chroms.momentum_compaction 
        = chroms.slip_factor + 1. / gamma / gamma;

    // d^2 cdt/d dpop^2 = 2! *a_2

    // d^3 cdt/ d dpop^3 = 3! * a_3 but I'm not planning on using it
    // double d3cdt_d_dpop3 = 6.0*(cdtpp_m_cdtmm - 2.0*cdtp_m_cdtm)/(12.0*dpp*dpp*dpp*cT0);

    // d^4 cdt / d dpop^4 = 4! * a_4 but I'm not planning on using it
    // double d4cdt_d_dpop4 = 24.0*(cdtpp_p_cdtmm_m2cdt0 - 4.0*cdtp_p_cdtm_m2cdt0)/(24.0*dpp*dpp*dpp*dpp*cT0);

    chroms.horizontal_chromaticity 
        = (8.0*qxp_m_qxm - qxpp_m_qxmm)/(12.0*dpp);

    chroms.vertical_chromaticity 
        = (8.0*qyp_m_qym - qypp_m_qymm)/(12.0*dpp);

    chroms.horizontal_chromaticity_prime 
        = 2.0*(16*qxp_p_qxm_m2qx0 - qxpp_p_qxmm_m2qx0) /(24.0*dpp*dpp);

    chroms.vertical_chromaticity_prime 
        = 2.0*(16*qyp_p_qym_m2qy0 - qypp_p_qymm_m2qy0) /(24.0*dpp*dpp);


    return chroms;
}

karray2d_row
Lattice_simulator::get_linear_one_turn_map(Lattice const& lattice)
{
    // 2nd order one-turn-map is sufficient for the jacobian
    auto map = get_one_turn_map<2>(lattice);
    return map.jacobian();
}

std::array<double, 3>
Lattice_simulator::map_to_twiss(karray2d_row map)
{
    // map must be 2x2
    if (map.extent(0)!=2 || map.extent(1)!=2)
        throw std::runtime_error("map_to_twiss: wrong dimensions");

    // [alpha, beta, psi]
    std::array<double, 3> ret;

    double cosmu = 0.5 * (map(0, 0) + map(1, 1));
    double asinmu = 0.5 * (map(0, 0) - map(1, 1));

    if (abs(cosmu) > 1.0)
        throw std::runtime_error("map_to_twiss: map is unstable");

    double mu = acos(cosmu);

    // beta is positive
    if (map(0,1) < 0.0) mu = 2.0*mconstants::pi - mu;

    ret[0] = asinmu / sin(mu);
    ret[1] = map(0,1) / sin(mu);
    ret[2] = mu / (2.0 * mconstants::pi);

    return ret;
}

double
Lattice_simulator::get_bucket_length(Lattice const& lattice)
{
    double freq = 0.0;
    double freq2 = 0.0;

    double harmon = 0.0;
    double harmon2 = 0.0;

    bool iswf = false;
    bool iswh = false;

    double eps = 1e-6;

    for(auto & ele : lattice.get_elements())
    {
        if (ele.get_type() == element_type::rfcavity)
        {
            if (ele.has_double_attribute("harmon"))
            {
                harmon = ele.get_double_attribute("harmon");

                if (iswh && (abs(harmon-harmon2)>eps))
                    throw std::runtime_error("get_bucket_length:" 
                            " rf elements with different harmonic" 
                            " number found!");

                harmon2 = harmon;
                iswh = true;
            }

            if (ele.has_double_attribute("freq"))
            {
                freq = ele.get_double_attribute("freq");

                if (iswf && (abs(freq-freq2)>eps))
                    throw std::runtime_error("get_bucket_length:" 
                            " rf elements with different frequency" 
                            " found!");

                freq2 = freq;
                iswf = true;
            }
        }
    }

    // use harmonic number
    if (iswh) return lattice.get_length()/harmon;

    // use frequency
    if (iswf)
    {
        double beta = lattice.get_reference_particle().get_beta();
        return pconstants::c * beta / freq;
    }

    // or return 0
    return 0.0;
}


// --------------------------------------------
// adjust tunes
// --------------------------------------------


namespace
{
    double
    get_AT_corrector_strength(Lattice_element const& elm)
    {
        // TODO: more element types
        // ...
        if (elm.get_type() == element_type::quadrupole)
            return elm.get_double_attribute("k1", 0.0);

        throw std::runtime_error(
                "Bad element type passed to get_AT_corrector_strength: "
                + elm.get_name());
    }


    void
    set_AT_corrector_strength(Lattice_element& elm, double strength)
    {
        // TODO: more element types
        // ...
        if (elm.get_type() == element_type::quadrupole)
            elm.set_double_attribute("k1", strength);
    }

    void
    get_strengths_param(
            std::vector<Lattice_element*> const& elms,
            std::vector<double> & original_strengths,
            double & param, bool & relative)
    {
        relative = false;

        double lastval = get_AT_corrector_strength(*(elms[0]));
        const double tolerance = 1.0e-12;

        for (int i=0; i<elms.size(); ++i)
        {
            double val = get_AT_corrector_strength(*(elms[i]));
            if (std::abs(val - lastval) > tolerance) relative = true;

            original_strengths.at(i) = val;
            lastval = val;
        }

        param = relative ? 1.0 : original_strengths[0];
    }



    struct Adjust_tunes_params
    {
        Lattice& lattice;

        double h_nu_target;
        double v_nu_target;

        std::vector<Lattice_element*> h_elements;
        std::vector<Lattice_element*> v_elements;

        std::vector<double> h_original_strengths;
        std::vector<double> v_original_strengths;

        double h_param;
        double v_param;

        bool h_relative; 
        bool v_relative;

        Adjust_tunes_params(
                Lattice& lattice,
                double horizontal_tune, 
                double vertical_tune,
                std::vector<Lattice_element*> const& h_correctors,
                std::vector<Lattice_element*> const& v_correctors)
            : lattice(lattice)
            , h_nu_target(horizontal_tune)
            , v_nu_target(vertical_tune)
            , h_elements(h_correctors)
            , v_elements(v_correctors)
            , h_original_strengths(h_correctors.size())
            , v_original_strengths(v_correctors.size())
            , h_param(1.0)
            , v_param(1.0)
            , h_relative(false)
            , v_relative(false)
        { 
            get_strengths_param(h_elements, h_original_strengths, 
                    h_param, h_relative);

            get_strengths_param(v_elements, v_original_strengths, 
                    v_param, v_relative);

#if 0
            if (verbosity > 1) 
            {
                logger << "Lattice_simulator::adjust_tunes: h_relative = "
                        << h_relative << ", v_relative = " << v_relative
                        << std::endl;
            }
#endif
        }
    };



    int
    adjust_tunes_function(
            const gsl_vector* x, 
            void* params, 
            gsl_vector* f)
    { 
        Adjust_tunes_params * atparams = 
            static_cast<Adjust_tunes_params*>(params);

        double h_param = gsl_vector_get(x, 0);

        for (int i=0; i<atparams->h_elements.size(); ++i)
        {
            double str = atparams->h_relative 
                ? h_param * atparams->h_original_strengths[i] 
                : h_param;

            auto pe = atparams->h_elements[i];
            set_AT_corrector_strength(*pe, str);
        }

        double v_param = gsl_vector_get(x, 1);

        for (int i=0; i<atparams->v_elements.size(); ++i)
        {
            double str = atparams->v_relative 
                ? v_param * atparams->v_original_strengths[i] 
                : v_param;

            auto pe = atparams->v_elements[i];
            set_AT_corrector_strength(*pe, str);
        }

        auto nus = Lattice_simulator::calculate_tune_and_cdt(
                atparams->lattice, 0.0);

        double nu_h = nus[0];
        double nu_v = nus[1];

        gsl_vector_set(f, 0, nu_h - atparams->h_nu_target);
        gsl_vector_set(f, 1, nu_v - atparams->v_nu_target);

        return GSL_SUCCESS;
    }
}



void
Lattice_simulator::adjust_tunes(
        Lattice& lattice,
        double horizontal_tune, 
        double vertical_tune,
        double tolerance )
{
    std::vector<Lattice_element*> h_correctors;
    std::vector<Lattice_element*> v_correctors;

    for(auto & e : lattice.get_elements())
    {
        if (e.has_marker(marker_type::h_tunes_corrector))
            h_correctors.push_back(&e);

        if (e.has_marker(marker_type::v_tunes_corrector))
            v_correctors.push_back(&e);
    }

    Adjust_tunes_params atparams(
            lattice,
            horizontal_tune, vertical_tune,
            h_correctors, v_correctors );

    const size_t n = 2;
    gsl_multiroot_function f = { 
        &adjust_tunes_function, n, (void*)&atparams };

    gsl_vector * x = gsl_vector_alloc(n);
    gsl_vector_set(x, 0, atparams.h_param);
    gsl_vector_set(x, 1, atparams.v_param);

    const gsl_multiroot_fsolver_type * T = gsl_multiroot_fsolver_hybrids;
    gsl_multiroot_fsolver * s = gsl_multiroot_fsolver_alloc(T, n);
    gsl_multiroot_fsolver_set(s, &f, x);

    int status;
    size_t iter = 0;
    const int max_iter = 100;

    do 
    { 
        iter++;

#if 0
        if (verbosity > 1) 
        {
            logger << "Lattice_simulator::adjust_tunes: iteration " 
                   << iter << std::endl;
        }
#endif

        status = gsl_multiroot_fsolver_iterate(s);
        if (status) break;

        status = gsl_multiroot_test_residual(s->f, tolerance);

    } while (status == GSL_CONTINUE && iter < max_iter);

    gsl_multiroot_fsolver_free(s);
    gsl_vector_free(x);

    if (iter >= max_iter) 
        throw std::runtime_error(
                "Lattice_elements::adjust_tunes: "
                "solver failed to converge" );
#if 0
    if (verbosity > 1) 
    {
        logger 
            << "Lattice_simulator::adjust_tunes: "
            << "begin new corrector quads" << std::endl;
    }
#endif

#if 0
    if (verbosity > 1) 
    {
        logger 
            << "Lattice_simulator::adjust_tunes: "
            << "end new corrector quads" << std::endl;
    }
#endif

#if 0
    if (verbosity > 0) 
    {
        logger 
            << "Lattice_simulator::adjust_tunes: "
            << "convergence achieved in " << iter 
            << " iterations" << std::endl;

        logger 
            << std::setprecision(decimal_digits(tolerance)+2);

        logger 
            << "Lattice_simulator::adjust_tunes: "
            << "final horizontal tune = " << get_horizontal_tune() 
            << ", final vertical tune = " << get_vertical_tune() 
            << std::endl;
    }
#endif

}


// --------------------------------------------
// Lattice Functions
// --------------------------------------------


void 
Lattice_simulator::CourantSnyderLatticeFunctions(Lattice& lattice)
{
    constexpr const int order = 2;
    using trigon_t = Trigon<double, order, 6>;

    const int ix  = 0;
    const int ipx = 1;
    const int iy  = 2;
    const int ipy = 3;

    auto const& ref = lattice.get_reference_particle();
    auto probe = calculate_closed_orbit(lattice);

    auto map = get_one_turn_map<order>(lattice);
    //auto jac = karray_to_matrix(map.jacobian());
    auto jac = map.jacobian();

    //JetParticle  jparticle(jp);
    //Particle      particle(jp);

    // .......... Check coupling ............................
    //::checkForCoupling(mtrx);

    // Calculate initial lattice functions ...
    // ... first horizontal

    double cs = ( jac(ix, ix) + jac(ipx, ipx) )/2.0;

    if( fabs( cs ) > 1.0 ) 
    {
        std::stringstream ss;
        ss << "*** ERROR ***                                     \n"
           << "*** ERROR *** LattSim::CourantSnyderLatticeFunctions \n"
           << "*** ERROR *** cos( psi_H ) = " << cs << "\n"
           << "*** ERROR *** Lattice is unstable.                \n"
           << "*** ERROR *** Cannot continue with calculation.   \n"
           << "*** ERROR ***                                     \n"
           << std::endl;

        throw std::runtime_error(ss.str());
    } 
    
    double sn = ( jac(ix, ipx) > 0.0 ) 
        ? sqrt( 1.0 - cs*cs ) : - sqrt( 1.0 - cs*cs );

    if( sn == 0.0 ) 
    {
        std::stringstream ss;
        ss << "*** ERROR ***                                     \n"
           << "*** ERROR *** LattSim::CourantSnyderLatticeFunctions \n"
           << "*** ERROR *** Integer horizontal tune.            \n"
           << "*** ERROR ***                                     \n"
           << std::endl;

        throw std::runtime_error(ss.str());
    }

    double beta_x = jac(ix, ipx) / sn;
    double alpha_x = (jac(ix, ix) - jac(ipx, ipx)) / (2.0*sn);

    // ... then vertical.
    cs = (jac(iy, iy) + jac(ipy, ipy))/2.0;

    if( fabs( cs ) <= 1.0 ) 
    {
        if( jac(iy, ipy) > 0.0 )  sn =  sqrt( 1.0 - cs*cs );
        else                      sn = -sqrt( 1.0 - cs*cs );
    }
    else 
    {
        std::stringstream ss;
        ss << "*** ERROR ***                                     \n"
           << "*** ERROR *** LattSim::CourantSnyderLatticeFunctions \n"
           << "*** ERROR *** cos( psi_V ) = " << cs << "\n"
           << "*** ERROR *** Lattice is unstable.                \n"
           << "*** ERROR *** Cannot continue with calculation.   \n"
           << "*** ERROR ***                                     \n"
           << std::endl;

        throw std::runtime_error(ss.str());
    }

    if( sn == 0.0 ) 
    {
        std::stringstream ss;
        ss << "*** ERROR ***                                     \n"
           << "*** ERROR *** LattSim::CourantSnyderLatticeFunctions \n"
           << "*** ERROR *** Integer vertical tune.              \n"
           << "*** ERROR ***                                     \n"
           << std::endl;

        throw std::runtime_error(ss.str());
    }

    double beta_y = jac(iy, ipy) / sn;
    double alpha_y = (jac(iy, iy) - jac(ipy, ipy)) / ( 2.0*sn );

    double beta0H  = beta_x;
    double beta0V  = beta_y;
    double alpha0H = alpha_x;
    double alpha0V = alpha_y;

    double oldpsiH = 0.0;
    double oldpsiV = 0.0;

    double tb      = 0.0;
    double t       = 0.0;
    double lng     = 0.0;
    double psi_x   = 0.0;
    double psi_y   = 0.0;

    // set to id map, ref points set to state
    //jparticle.setState( particle.State() );

    Commxx comm;
    bunch_t<trigon_t> bunch(ref, comm.size(), comm);

    // design reference particle from the closed orbit
    auto ref_l = ref;
    ref_l.set_state(probe);
    bunch.set_design_reference_particle(ref_l);

    auto tparts = bunch.get_host_particles();

    // init value
    for(int i=0; i<6; ++i) 
        tparts(0, i).set(probe[i], i);

    // check in
    bunch.checkin_particles();

    // propagate trigon
    for(auto& elm : lattice.get_elements())
    {
        // At one time, dipoles with non-standard faces were discriminated 
        // against and wouldn't have
        // their phase advance calculated.
        // bool is_regular = 
        //     ( ( typeid(*lbe) != typeid(rbend)    ) &&
        //     (   typeid(*lbe) != typeid(CF_rbend) ) &&
        //     (   typeid(*lbe) != typeid(Slot)     ) &&
        //     (   typeid(*lbe) != typeid(srot)     ) &&		   
        //     (     (*lbe).hasStandardFaces()      )  );

        bool is_regular = true;

        //lng += elm.OrbitLength( particle );
        lng += elm.get_length();
        FF_element::apply(elm, bunch);

        auto mtrx = bunch.get_jacobian(0);

        tb = mtrx(ix,ix) * beta0H -  mtrx(ix,ipx) * alpha0H;
        beta_x  = (tb * tb + mtrx(ix,ipx) * mtrx(ix,ipx))/beta0H;

        alpha_x = -1.0*(tb*(mtrx(ipx,ix)*beta0H - mtrx(ipx,ipx)*alpha0H) 
                + mtrx(ix,ipx)*mtrx(ipx,ipx))/beta0H;

        if ( is_regular ) 
        {
             t = atan2(mtrx(ix,ipx),tb);

             // numerical round off errs introduce unphisical jumps in phase 
             // while(t < oldpsiH) t += M_TWOPI; 
             while(t < oldpsiH*(1.-1.e-4)) t += mconstants::pi*2;

             psi_x = oldpsiH = t;
         }
         else 
         {
             psi_x = oldpsiH;
         }

         tb = mtrx(iy,iy) * beta0V -  mtrx(iy,ipy) * alpha0V;
         beta_y = (tb * tb + mtrx(iy,ipy) * mtrx(iy,ipy))/beta0V;

         alpha_y = -1.0*(tb*(mtrx(ipy,iy)*beta0V - mtrx(ipy,ipy)*alpha0V) 
                 + mtrx(iy,ipy)*mtrx(ipy,ipy))/beta0V;

         if ( is_regular ) 
         {
             t = atan2(mtrx(iy,ipy),tb);

             // numerical round off errs introduce unphisical jumps in phase 
             // while(t < oldpsiV) t += M_TWOPI; 
             while(t < oldpsiV*(1.-1.e-4)) t += mconstants::pi*2;

             psi_y = oldpsiV = t;
         }
         else 
         {
             psi_y = oldpsiV;
         } 

         elm.lf.arcLength = lng;
         elm.lf.beta.hor  = beta_x;
         elm.lf.beta.ver  = beta_y;
         elm.lf.alpha.hor = alpha_x;
         elm.lf.alpha.ver = alpha_y;
         elm.lf.psi.hor   = psi_x;
         elm.lf.psi.ver   = psi_y;

    }  // End loop on lbe ...

}

void
Lattice_simulator::calc_dispersions(Lattice& lattice)
{
    const double dpp = 0.005;

    constexpr const int order = 2;
    using trigon_t = Trigon<double, order, 6>;

    const int ix  = 0;
    const int ipx = 1;
    const int iy  = 2;
    const int ipy = 3;

    auto const& ref = lattice.get_reference_particle();

    // Preliminary steps ...
    auto probe1 = calculate_closed_orbit(lattice, 0.0);
    auto probe2 = calculate_closed_orbit(lattice, dpp);

    auto fstJac = get_one_turn_map<order>(lattice, 0.0).jacobian();
    auto secJac = get_one_turn_map<order>(lattice, dpp).jacobian();

    // propagate through elements
    Commxx comm;
    bunch_t<double> b1(ref, comm.size(), 1e9, comm);
    bunch_t<double> b2(ref, comm.size(), 1e9, comm);

    // design reference particle from the closed orbit
    auto ref1 = ref;
    ref1.set_state(probe1);
    b1.set_design_reference_particle(ref1);

    auto ref2 = ref;
    ref2.set_state(probe2);
    b2.set_design_reference_particle(ref2);

    // local particle data
    auto part1 = b1.get_host_particles();
    auto part2 = b2.get_host_particles();

    // init value
    for(int i=0; i<6; ++i) 
    {
        part1(0, i) = probe1[i];
        part2(0, i) = probe2[i];
    }

    // check in
    b1.checkin_particles();
    b2.checkin_particles();

    // arcLength
    double lng = 0.0;

    // Attach initial dispersion data to the elements...
    for(auto& elm : lattice.get_elements())
    {
        lng += elm.get_length();

        FF_element::apply(elm, b1);
        FF_element::apply(elm, b2);

        b1.checkout_particles();
        b2.checkout_particles();

        std::array<double, 6> d;

        for(int i=0; i<6; ++i)
            d[i] = (part2(0, i) - part1(0, i)) / dpp;

        elm.lf.dispersion.hor = d[ix];
        elm.lf.dispersion.ver = d[iy];
        elm.lf.dPrime.hor     = d[ipx];
        elm.lf.dPrime.ver     = d[ipy];
        elm.lf.arcLength      = lng;
    }


    // Attach tune and chromaticity to the beamline ........
#if 0
    Vector    firstNu(2), secondNu(2);
    if( ( 0 == filterTransverseTunes( firstJacobian, firstNu   ) ) &&
      ( 0 == filterTransverseTunes( secondJacobian, secondNu ) ) )
    {
    lr_.tune.hor = firstNu(0);
    lr_.tune.ver = firstNu(1);
    lr_.chromaticity.hor = ( secondNu(0) - firstNu(0) ) / dpp;
    lr_.chromaticity.ver = ( secondNu(1) - firstNu(1) ) / dpp;
    }
    else {
    (*pcerr) << "*** ERROR ***                                        \n"
            "*** ERROR *** LattFuncSage::Disp_Calc                \n"
            "*** ERROR ***                                        \n"
            "*** ERROR *** Horrible error occurred while trying   \n"
            "*** ERROR *** to filter the tunes.                   \n"
            "*** ERROR ***                                        \n"
         << endl;
    ret = 111;

    return ret;
    }
#endif
}





// --------------------------------------------
// adjust chromaticities
// --------------------------------------------

namespace
{
    void 
    change_chromaticityby(
            double dh, double dv,  
            std::vector<Lattice_element*> const& h_correctors,
            std::vector<Lattice_element*> const& v_correctors)
    { 
#if 0
        ensure_jet_environment(map_order);
        BmlPtr beamline_sptr(chef_lattice_sptr->get_beamline_sptr());
        BeamlineContext beamline_context(
                reference_particle_to_chef_particle(
                        lattice_sptr->get_reference_particle()), beamline_sptr);
        beamline_context.handleAsRing();
        set_chef_chrom_correctors(horizontal_correctors, *chef_lattice_sptr,
                beamline_context, true);
        set_chef_chrom_correctors(vertical_correctors, *chef_lattice_sptr,
                beamline_context, false);
        int status = beamline_context.changeChromaticityBy(dh, dv); 
          if (status == BeamlineContext::NO_CHROMATICITY_ADJUSTER) {
                  throw std::runtime_error(
                          "Lattice_simulator::adjust_chromaticities: no corrector elements found");
          } 
          else if (status != BeamlineContext::OKAY) {
                  throw std::runtime_error(
                          "Lattice_simulator::adjust_chromaticities: failed with unknown status");
          }
          
         extract_sextupole_strengths(horizontal_correctors, *chef_lattice_sptr);
         extract_sextupole_strengths(vertical_correctors, *chef_lattice_sptr);
         update();
         have_chromaticities = false;
#endif
    }

    struct ChromAdjuster
    {
        using MatrixD = Eigen::Matrix<double, 
              Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

        const int N;
        std::vector<Lattice_element*> correctors;
        MatrixD f_;

        ChromAdjuster(
            std::vector<Lattice_element*> const& h_correctors,
            std::vector<Lattice_element*> const& v_correctors)
        : N(h_correctors.size() + v_correctors.size())
        , correctors(N), f_(N, 2)
        {
            auto h_size = h_correctors.size();
            auto v_size = v_correctors.size();

            for(size_t i=0; i<h_size; ++i)
            {
                correctors[i] = h_correctors[i];
                f_(i, 0) = 1.0;
                f_(i, 1) = 0.0;
            }

            for(size_t i=0; i<v_size; ++i)
            {
                correctors[i+h_size] = v_correctors[i];
                f_(i+h_size, 0) = 0.0;
                f_(i+h_size, 1) = 1.0;
            }
        }

        void change_chrom_by(double dh, double dv)
        {
#if 0
            JetParticle  jpr(jp); 
            JetParticle  jpr2(jp);
            JetParticle  jpr3(jp);

            // Calculate current lattice functions
            LattFuncSage lfs( myBeamlinePtr_ );
            lfs.attachLocalData(true);
            myBeamlinePtr_->propagate(jpr);

            lfs.CourantSnyderLatticeFunctions( jpr);
            lfs.NewDisp_Calc   ( jpr2);
#endif

            MatrixD beta(2, N);
            MatrixD delta_xi(2, 1);
            double dsp;

            // delta_xi = beta * _f * c
            // w = _f * c
            // delta strength_k = 2 * pi * brho * w_k / dps_k

            bool gotDispersion = false;
            bool gotBetas = false;

            for( int j = 0; j < N; ++j) 
            {
                gotDispersion = false;
                gotBetas      = false;

#if 0
                BarnacleList::iterator ptr = 
                    correctors_[j]->dataHook.find( "Dispersion" );

                if( ptr ==  correctors_[j]->dataHook.end() ) {     	
                  ptr = correctors_[j]->dataHook.find( "Twiss" );
                }

                if( ptr != correctors_[j]->dataHook.end() ) { 
                  dsp = boost::any_cast<LattFuncSage::lattFunc>(ptr->info).dispersion.hor;
                  gotDispersion = true;
                }
                else {
                  dsp = 0.0;  // Just to give it a value.
                }

                ptr = correctors_[j]->dataHook.find( "Twiss" );

                // NOTE: possibly 2nd time this is done.
                if(ptr !=  correctors_[j]->dataHook.end() ) 
                {
                    beta(0,j) =   boost::any_cast<LattFuncSage::lattFunc>(
                            ptr->info).beta.hor * dsp;

                    beta(1,j) = - boost::any_cast<LattFuncSage::lattFunc>(
                            ptr->info).beta.ver * dsp;

                    gotBetas = true;
                }
#endif
               
                if( !(gotBetas && gotDispersion) ) 
                {
                    throw std::runtime_error( 
                            "ChromaticityAdjuster::changeChromaticityBy(): " 
                            "Lattice functions or dispersion not yet "
                            "calculated.");
                }
            }

            // Adjust chromaticity
            delta_xi(0,0) = dh;
            delta_xi(1,0) = dv;
         
            double brho = 1.0; //jpr.ReferenceBRho();
            MatrixD c_ = (2.0*mconstants::pi*brho)
                * ( (beta*f_).inverse() * delta_xi );
            MatrixD w  = f_* c_;
         
            for(int j=0; j<N; ++j)
            {
                double str = correctors[j]->get_double_attribute("k2");
                str += w(j, 0);

                correctors[j]->set_double_attribute("k2", str);
            }

        }
    };


}

void
Lattice_simulator::adjust_chromaticities(
        Lattice& lattice,
        double horizontal_chromaticity, 
        double vertical_chromaticity,
        double tolerance,
        int max_steps )
{
    std::vector<Lattice_element*> h_correctors;
    std::vector<Lattice_element*> v_correctors;

    // extract the H/V correctors
    for(auto & e : lattice.get_elements())
    {
        if (e.has_marker(marker_type::h_chrom_corrector))
            h_correctors.push_back(&e);

        if (e.has_marker(marker_type::v_chrom_corrector))
            v_correctors.push_back(&e);
    }

    // Adjuster
    ChromAdjuster ca(h_correctors, v_correctors);;

    // current chromaticities
    auto chroms = get_chromaticities(lattice);

    double chr_h = chroms.horizontal_chromaticity;
    double chr_v = chroms.vertical_chromaticity;

    // delta = target - current
    double dh = horizontal_chromaticity - chr_h;
    double dv = vertical_chromaticity - chr_v;

    int count = 0;

    // loop
    while (((std::abs(dh) > tolerance) || (std::abs(dv) > tolerance)) 
            && (count < max_steps)) 
    {
        //int status = beamline_context.changeChromaticityBy(dh, dv);

#if 1
        //if (verbosity>0)
        { 
            std::cout 
                << "  step = " << count 
                << " chromaticity (H,V):  (" << chr_h<< ", " << chr_v<< ")"
                << "   (Delta H, Delta V): (" << dh << ", " << dv << ")" 
                <<    std::endl; 
        }
#endif 

        ca.change_chrom_by(dh, dv);
        //change_chromaticityby(dh, dv, h_correctors, v_correctors);

        auto chroms = get_chromaticities(lattice);

        chr_h = chroms.horizontal_chromaticity;
        chr_v = chroms.vertical_chromaticity;

        dh = horizontal_chromaticity - chr_h;
        dv = vertical_chromaticity - chr_v;

        count++; 
    }

#if 0
    if (verbosity>0)
    { 
        logger
            << " after steps=" << count 
            << " chromaticity (H,V):  (" << chr_h<<", " <<chr_v<<")"
            << "   (Delta H, Delta V): (" << dh << ", " << dv <<")"
            <<    std::endl; 
    }
#endif


    //extract_sextupole_strengths(horizontal_correctors, *chef_lattice_sptr);
    //extract_sextupole_strengths(vertical_correctors, *chef_lattice_sptr);
    //update();

#if 0
    Logger flogger(0, "sextupole_correctors.txt", false, true);
    flogger
        << "! the sextupole correctors  for the chromaticity (H, V):  (" 
        << chr_h << " ,  " << chr_v << " ) " 
        << std::endl;

    write_sextupole_correctors(h_correctors, v_correctors, *chef_lattice_sptr, flogger);

    logger
        << "sextupole correctors strength written in the file sextupole_correctors.txt"
        <<std::endl;
#endif

    if (count == max_steps)  
        throw std::runtime_error( 
                "Lattice_simulator::adjust_chromaticities: "
                "Convergence not achieved. Increase the maximum number of steps.");
}


 
#if 0
    const int verbosity=1;
    ensure_jet_environment(map_order);
    BmlPtr beamline_sptr(chef_lattice_sptr->get_beamline_sptr());
    BeamlineContext beamline_context(
            reference_particle_to_chef_particle(
                    lattice_sptr->get_reference_particle()), beamline_sptr);
    beamline_context.handleAsRing();
    set_chef_chrom_correctors(horizontal_correctors, *chef_lattice_sptr,
            beamline_context, true);
    set_chef_chrom_correctors(vertical_correctors, *chef_lattice_sptr,
            beamline_context, false);

    double chr_h = get_horizontal_chromaticity();
    double chr_v = get_vertical_chromaticity();

    double dh = horizontal_chromaticity - chr_h;
    double dv = vertical_chromaticity - chr_v;
    int count = 0;

    Logger logger(0);
    if (verbosity>0){
        logger<<"_________________________________________"<<std::endl;
        logger <<" Initial chromaticiy (H,V):  ("<< chr_h<<", "
                    <<chr_v<<")"<<std::endl;
        logger <<" Desired chromaticity (H,V):  ("<< horizontal_chromaticity<<", "
                    <<vertical_chromaticity<<")"<<std::endl;
        logger<<"_________________________________________"<<std::endl;
        logger<<"adjusting chromaticity:"<<std::endl;
    }

    while (((std::abs(dh) > tolerance) || (std::abs(dv) > tolerance))
            && (count < max_steps)) {
        int status = beamline_context.changeChromaticityBy(dh, dv);

       if (verbosity>0){
          logger<< "  step=" << count << " chromaticity (H,V):  (" << chr_h<<", "
                 <<chr_v<<")"<< "   (Delta H, Delta V): (" << dh << ", " << dv <<")"<<    std::endl;
        }

       change_chromaticityby(dh,dv, horizontal_correctors, vertical_correctors, logger, verbosity);
        chr_h = get_horizontal_chromaticity();
        chr_v = get_vertical_chromaticity();
        dh = horizontal_chromaticity - chr_h;
        dv = vertical_chromaticity - chr_v;
        count++;
    }

    if (verbosity>0){
      logger<< " after steps=" << count << " chromaticity (H,V):  (" << chr_h<<", "
               <<chr_v<<")"<< "   (Delta H, Delta V): (" << dh << ", " << dv <<")"<<    std::endl; 
    }

    extract_sextupole_strengths(horizontal_correctors, *chef_lattice_sptr);
    extract_sextupole_strengths(vertical_correctors, *chef_lattice_sptr);
    update();

    Logger flogger(0, "sextupole_correctors.txt", false, true);
    flogger      << "! the sextupole correctors  for the chromaticity (H, V):  ("
                << chr_h << " ,  " << chr_v << " ) " << std::endl;
    write_sextupole_correctors(horizontal_correctors, vertical_correctors,
                *chef_lattice_sptr, flogger);
    logger<<"sextupole correctors strength written in the file  sextupole_correctors.txt"<<std::endl;
    if (count == max_steps)  throw std::runtime_error(
        "Lattice_simulator::adjust_chromaticities: Convergence not achieved. Increase the maximum number of steps.");
}

void 
Lattice_simulator::change_chromaticityby(double dh, double dv,  Lattice_elements const& horizontal_correctors,
        Lattice_elements const& vertical_correctors, Logger & logger, int verbosity)
{
    
    
    ensure_jet_environment(map_order);
    BmlPtr beamline_sptr(chef_lattice_sptr->get_beamline_sptr());
    BeamlineContext beamline_context(
            reference_particle_to_chef_particle(
                    lattice_sptr->get_reference_particle()), beamline_sptr);
    beamline_context.handleAsRing();
    set_chef_chrom_correctors(horizontal_correctors, *chef_lattice_sptr,
            beamline_context, true);
    set_chef_chrom_correctors(vertical_correctors, *chef_lattice_sptr,
            beamline_context, false);
    int status = beamline_context.changeChromaticityBy(dh, dv); 
      if (status == BeamlineContext::NO_CHROMATICITY_ADJUSTER) {
              throw std::runtime_error(
                      "Lattice_simulator::adjust_chromaticities: no corrector elements found");
      } 
      else if (status != BeamlineContext::OKAY) {
              throw std::runtime_error(
                      "Lattice_simulator::adjust_chromaticities: failed with unknown status");
      }
      
     extract_sextupole_strengths(horizontal_correctors, *chef_lattice_sptr);
     extract_sextupole_strengths(vertical_correctors, *chef_lattice_sptr);
     update();
     have_chromaticities = false;
#endif



#endif // __CUDA_ARCH__

