#include "ff_sbend.h"
#include "synergia/lattice/chef_utils.h"

FF_sbend::FF_sbend()
{

}

double FF_sbend::get_reference_cdt(double length, double strength, double angle,
                                   bool ledge, bool redge,
                                   double e1, double e2, 
                                   double us_edge_k_p, double ds_edge_k_p,
                                   double dphi,
                                   std::complex<double> const & phase,
                                   std::complex<double> const & term,
                                   Reference_particle &reference_particle)
{
    if (length == 0)
    {
        reference_particle.set_state_cdt(0.0);
        return 0.0;
    }
    else
    {
        double pref = reference_particle.get_momentum();
        double m = reference_particle.get_mass();

        double x(reference_particle.get_state()[Bunch::x]);
        double xp(reference_particle.get_state()[Bunch::xp]);
        double y(reference_particle.get_state()[Bunch::y]);
        double yp(reference_particle.get_state()[Bunch::yp]);
        double cdt(0.0);
        double dpop(reference_particle.get_state()[Bunch::dpop]);

        double ce1 = cos(-e1);
        double se1 = sin(-e1);
        double ce2 = cos(-e2);
        double se2 = sin(-e2);

        if (ledge)
        {
            FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce1, se1, pref, m);
            FF_algorithm::edge_unit(y, xp, yp, dpop, us_edge_k_p);
        }

        FF_algorithm::bend_complete(x, xp, y, yp, cdt, dpop,
                   dphi, strength, pref, m, 0.0/*ref cdt*/, phase, term);

        if (redge)
        {
            FF_algorithm::edge_unit(y, xp, yp, dpop, ds_edge_k_p);
            FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce2, se2, pref, m);
        }

        reference_particle.set_state(x, xp, y, yp, cdt, dpop);
        return cdt;
    }
}


double FF_sbend::get_reference_cdt(double length, double angle, double strength,
                                   bool ledge, bool redge,
                                   double e1, double e2, 
                                   double us_edge_k_p, double ds_edge_k_p,
                                   std::complex<double> phase_e1, 
                                   std::complex<double> phase_e2, 
                                   double * kl,
                                   Reference_particle &reference_particle)
{
    if (length == 0) 
    {
        reference_particle.set_state_cdt(0.0);
        return 0.0;
    } 
    else 
    {
        double pref = reference_particle.get_momentum();
        double m = reference_particle.get_mass();

        double x(reference_particle.get_state()[Bunch::x]);
        double xp(reference_particle.get_state()[Bunch::xp]);
        double y(reference_particle.get_state()[Bunch::y]);
        double yp(reference_particle.get_state()[Bunch::yp]);
        double cdt(0.0);
        double dpop(reference_particle.get_state()[Bunch::dpop]);

        double ce1 = cos(-e1);
        double se1 = sin(-e1);
        double ce2 = cos(-e2);
        double se2 = sin(-e2);

        double step_length = length / steps;
        double step_angle  = angle / steps;
        double r0 = length / angle;

        double step_strength[6] = { kl[0] * step_length, kl[1] * step_length,
                                    kl[2] * step_length, kl[3] * step_length,
                                    kl[4] * step_length, kl[5] * step_length };

        if (ledge)
        {
            FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce1, se1, pref, m);

            //FF_algorithm::edge_unit(y, yp, us_edge_k);
            FF_algorithm::edge_unit(y, xp, yp, dpop, us_edge_k_p);

            FF_algorithm::bend_edge(x, xp, y, yp, cdt, dpop, e1, phase_e1, strength, pref, m);
        }

        // bend
#if 0
        FF_algorithm::bend_yoshida6<double, FF_algorithm::thin_cf_kick_2<double>, 2>
            ( x, xp, y, yp, cdt, dpop,
              pref, m, 0.0 /* step ref_cdt */,
              step_angle, step_strength,
              r0, strength, steps );
#endif

        if (redge)
        {
            FF_algorithm::bend_edge(x, xp, y, yp, cdt, dpop, e2, phase_e2, strength, pref, m);

            //FF_algorithm::edge_unit(y, yp, ds_edge_k);
            FF_algorithm::edge_unit(y, xp, yp, dpop, ds_edge_k_p);

            FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce2, se2, pref, m);
        }

        reference_particle.set_state(x, xp, y, yp, cdt, dpop);
        return cdt;
    }
}

void FF_sbend::apply(Lattice_element_slice const& slice, JetParticle& jet_particle)
{
    throw std::runtime_error("libFF sbend on JetParticle not implemented");

#if 0
    double length = slice.get_right() - slice.get_left();
    double angle = slice.get_lattice_element().get_double_attribute("angle");

    double cos_angle = cos(angle);
    double sin_angle = sin(angle);

    typedef PropagatorTraits<JetParticle>::State_t State_t;
    typedef PropagatorTraits<JetParticle>::Component_t Component_t;

    State_t& state = jet_particle.State();

    Component_t & x(state[Chef::x]);
    Component_t & xp(state[Chef::xp]);
    Component_t & y(state[Chef::y]);
    Component_t & yp(state[Chef::yp]);
    Component_t & cdt(state[Chef::cdt]);
    Component_t & dpop(state[Chef::dpop]);

    double reference_momentum = jet_particle.ReferenceMomentum();
    double reference_brho     = jet_particle.ReferenceBRho();
    double m = jet_particle.Mass();

    sbend_unit(x, xp, y, yp, cdt, dpop,
               length, cos_angle, sin_angle,
               reference_momentum, m, reference_brho);
#endif
}

void FF_sbend::apply(Lattice_element_slice const& slice, Bunch& bunch)
{
    double t = simple_timer_current();

    double      a = slice.get_lattice_element().get_double_attribute("angle");
    double      l = slice.get_lattice_element().get_double_attribute("l");
    double length = slice.get_right() - slice.get_left();
    double  angle = ( length / l ) * a;

    double     r0 = l / a;

    double ledge  = slice.has_left_edge();
    double redge  = slice.has_right_edge();

    double cos_angle = cos(angle);
    double sin_angle = sin(angle);

    double e1 = 0.0;
    double e2 = 0.0;

    Reference_particle       & ref_l = bunch.get_design_reference_particle();
    Reference_particle const & ref_b = bunch.get_reference_particle();

    if (slice.get_lattice_element().has_double_attribute("e1"))
        e1 = slice.get_lattice_element().get_double_attribute("e1");
    if (slice.get_lattice_element().has_double_attribute("e2"))
        e2 = slice.get_lattice_element().get_double_attribute("e2");


    int cf = 0;  // combined function
    double k_l[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    if (slice.get_lattice_element().has_double_attribute("k1"))
    {
        // quad component
        k_l[0] = slice.get_lattice_element().get_double_attribute("k1");
        k_l[1] = 0.0;

        if (k_l[0] != 0) cf = 1;
    }

    if (slice.get_lattice_element().has_double_attribute("k2"))
    {
        // sextupole component
        k_l[2] = slice.get_lattice_element().get_double_attribute("k2");
        k_l[3] = 0.0;

        if (k_l[2] != 0) cf = 2;
    }

    if (slice.get_lattice_element().has_double_attribute("k3"))
    {
        // octupole component
        k_l[4] = slice.get_lattice_element().get_double_attribute("k3");
        k_l[5] = 0.0;

        if (k_l[4] != 0) cf = 3;
    }

    double scale = ref_l.get_momentum() / (ref_b.get_momentum() * (1.0 + ref_b.get_state()[Bunch::dpop]));

    double k_b[6] = { k_l[0] * scale, k_l[1] * scale,
                      k_l[2] * scale, k_l[3] * scale,
                      k_l[4] * scale, k_l[5] * scale };

    double usAngle = e1;
    double dsAngle = -e2;
    double usFaceAngle = e1;
    double dsFaceAngle = e2;

    if (!redge)
    {
        dsAngle = 0;
        dsFaceAngle = 0;
    }

    if (!ledge)
    {
        usAngle = 0;
        usFaceAngle = 0;
    }

    int local_num = bunch.get_local_num();
    int local_s_num = bunch.get_local_spectator_num();

    MArray2d_ref particles = bunch.get_local_particles();
    MArray2d_ref s_particles = bunch.get_local_spectator_particles();

    // lattice reference
    double pref_l = ref_l.get_momentum();
    double brho_l = pref_l / PH_CNV_brho_to_p;
    double    m_l = ref_l.get_mass();
    int  charge_l = ref_l.get_charge();

    // the particle dpop is with respect to this momentum which goes with the bunch
    double pref_b = ref_b.get_momentum();
    double brho_b = pref_b / PH_CNV_brho_to_p;
    double    m_b = bunch.get_mass();

    // common
    double strength = brho_l * a / l;

    double psi = angle - (usFaceAngle + dsFaceAngle);
    double dphi = -psi;
    std::complex<double> phase = std::exp( std::complex<double>(0.0, psi) );
    std::complex<double> term = std::complex<double>(0.0, length / angle) *
                                std::complex<double>(1.0 - cos_angle, - sin_angle) *
                                std::complex<double>(cos(dsFaceAngle), -sin(dsFaceAngle));

    double ce1 = cos(-e1);
    double se1 = sin(-e1);
    double ce2 = cos(-e2);
    double se2 = sin(-e2);

    // edge kick strength (scaled to particle)
    double us_edge_k =   ((charge_l > 0) ? 1.0 : -1.0) * strength * tan(usAngle) / brho_b;
    double ds_edge_k = - ((charge_l > 0) ? 1.0 : -1.0) * strength * tan(dsAngle) / brho_b;

    // edge kick (per) particle (full strength, angles are calculated at each particle)
    double us_edge_k_p =   ((charge_l > 0) ? 1.0 : -1.0) * strength / brho_b;
    double ds_edge_k_p = - ((charge_l > 0) ? 1.0 : -1.0) * strength / brho_b;

    // edge kick x/y
    double us_edge_k_x = 0.0;
    double us_edge_k_y = 0.0;

    double ds_edge_k_x = 0.0;
    double ds_edge_k_y = 0.0;

    if (cf == 0)
    {
#if 0
        // the reference time uses the momentum of the lattice
        double reference_cdt = get_reference_cdt(length, strength, angle, ledge, redge, 
                e1, e2, us_edge_k_p/scale, ds_edge_k_p/scale, dphi, phase, term, ref_l);
#endif

        // propagate the reference particle, and set the edge kick strength 
        // from the reference particle
        double    x_l = ref_l.get_state()[Bunch::x];
        double   xp_l = ref_l.get_state()[Bunch::xp];
        double    y_l = ref_l.get_state()[Bunch::y];
        double   yp_l = ref_l.get_state()[Bunch::yp];
        double  cdt_l = 0.0;
        double dpop_l = ref_l.get_state()[Bunch::dpop];

        if (ledge)
        {
            // slot
            FF_algorithm::slot_unit(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l, ce1, se1, pref_l, m_l);

            double p_l = 1.0 + dpop_l;
            double zp_l = sqrt(p_l*p_l - xp_l*xp_l - yp_l*yp_l);

            us_edge_k_x = us_edge_k_p * (xp_l/zp_l);
            us_edge_k_y = us_edge_k_p * (yp_l/zp_l);

            // edge kick strenth are scaled to bunch. so need to div by "scale" to scale
            // it to the lattice reference
            FF_algorithm::edge_unit(y_l, xp_l, yp_l, us_edge_k_x/scale, us_edge_k_y/scale, 0);
        }

        FF_algorithm::bend_complete(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l,
                   dphi, strength, pref_l, m_l, 0.0/*ref cdt*/, phase, term);

        if (redge)
        {
            double p_l = 1.0 + dpop_l;
            double zp_l = sqrt(p_l*p_l - xp_l*xp_l - yp_l*yp_l);

            ds_edge_k_x = ds_edge_k_p * (xp_l/zp_l);
            ds_edge_k_y = ds_edge_k_p * (yp_l/zp_l);

            // edge kick strenth are scaled to bunch. so need to div by "scale" to scale
            // it to the lattice reference
            FF_algorithm::edge_unit(y_l, xp_l, yp_l, ds_edge_k_x/scale, ds_edge_k_y/scale, 0);

            // slot
            FF_algorithm::slot_unit(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l, ce2, se2, pref_l, m_l);
        }

        ref_l.set_state(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l);
        double ref_cdt = cdt_l;
 
        // no combined function
        // bunch particles
        {
            #pragma omp parallel for
            for (int part = 0; part < local_num; ++part)
            {
                double x   (particles[part][Bunch::x   ]);
                double xp  (particles[part][Bunch::xp  ]);
                double y   (particles[part][Bunch::y   ]);
                double yp  (particles[part][Bunch::yp  ]);
                double cdt (particles[part][Bunch::cdt ]);
                double dpop(particles[part][Bunch::dpop]);

                if (ledge)
                {
                    // slot
                    FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce1, se1, pref_b, m_b);

                    // edge
                    // FF_algorithm::edge_unit(y, yp, us_edge_k);                    // chef fixed angle (only kicks yp)
                    // FF_algorithm::edge_unit(y, xp, yp, dpop, us_edge_k_p);        // chef per-particle angle
                    FF_algorithm::edge_unit(y, xp, yp, us_edge_k_x, us_edge_k_y, 0); // ref particle angle (kicks both xp and yp)
                }

                // bend
                FF_algorithm::bend_unit(x, xp, y, yp, cdt, dpop,
                           dphi, strength, pref_b, m_b, ref_cdt,
                           phase, term);

                if (redge)
                {
                    // edge
                    // FF_algorithm::edge_unit(y, yp, ds_edge_k);
                    // FF_algorithm::edge_unit(y, xp, yp, dpop, ds_edge_k_p);
                    FF_algorithm::edge_unit(y, xp, yp, ds_edge_k_x, ds_edge_k_y, 0);

                    // slot
                    FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce2, se2, pref_b, m_b);
                }

                particles[part][Bunch::x]  = x;
                particles[part][Bunch::xp] = xp;
                particles[part][Bunch::y]  = y;
                particles[part][Bunch::yp] = yp;
                particles[part][Bunch::cdt] = cdt;
            }
        }

        // bunch spectator particles
        {
            #pragma omp parallel for
            for (int part = 0; part < local_s_num; ++part)
            {
                double x   (s_particles[part][Bunch::x   ]);
                double xp  (s_particles[part][Bunch::xp  ]);
                double y   (s_particles[part][Bunch::y   ]);
                double yp  (s_particles[part][Bunch::yp  ]);
                double cdt (s_particles[part][Bunch::cdt ]);
                double dpop(s_particles[part][Bunch::dpop]);

                if (ledge)
                {
                    // slot
                    FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce1, se1, pref_b, m_b);

                    // edge
                    // FF_algorithm::edge_unit(y, yp, us_edge_k);                    // chef fixed angle (only kicks yp)
                    // FF_algorithm::edge_unit(y, xp, yp, dpop, us_edge_k_p);        // chef per-particle angle
                    FF_algorithm::edge_unit(y, xp, yp, us_edge_k_x, us_edge_k_y, 0); // ref particle angle (kicks both xp and yp)
                }

                // bend
                FF_algorithm::bend_unit(x, xp, y, yp, cdt, dpop,
                           dphi, strength, pref_b, m_b, ref_cdt,
                           phase, term);

                if (redge)
                {
                    // edge
                    // FF_algorithm::edge_unit(y, yp, ds_edge_k);
                    // FF_algorithm::edge_unit(y, xp, yp, dpop, ds_edge_k_p);
                    FF_algorithm::edge_unit(y, xp, yp, ds_edge_k_x, ds_edge_k_y, 0);

                    // slot
                    FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce2, se2, pref_b, m_b);
                }

                s_particles[part][Bunch::x]  = x;
                s_particles[part][Bunch::xp] = xp;
                s_particles[part][Bunch::y]  = y;
                s_particles[part][Bunch::yp] = yp;
                s_particles[part][Bunch::cdt] = cdt;
            }
        }
    }
    else
    {
        std::complex<double> phase_e1 = FF_algorithm::bend_edge_phase(e1);
        std::complex<double> phase_e2 = FF_algorithm::bend_edge_phase(e2);

        std::complex<double> phase_unit = FF_algorithm::bend_unit_phase(angle);
        std::complex<double>  term_unit = FF_algorithm::bend_unit_term(r0, angle);

        double step_angle = angle / steps;
        double step_length = length / steps;

        double step_kl_l[6] = 
                { k_l[0] * step_length, k_l[1] * step_length,
                  k_l[2] * step_length, k_l[3] * step_length,
                  k_l[4] * step_length, k_l[5] * step_length };

        double step_kl_b[6] = 
                { k_b[0] * step_length, k_b[1] * step_length,
                  k_b[2] * step_length, k_b[3] * step_length,
                  k_b[4] * step_length, k_b[5] * step_length };

#if 0
        double ref_cdt = get_reference_cdt(length, angle, strength, ledge, redge,
                e1, e2, us_edge_k_p/scale, ds_edge_k_p/scale, phase_e1, phase_e2, kl, ref_l);
#endif

        // propagate the reference particle, and set the edge kick strength 
        // from the reference particle
        double    x_l = ref_l.get_state()[Bunch::x];
        double   xp_l = ref_l.get_state()[Bunch::xp];
        double    y_l = ref_l.get_state()[Bunch::y];
        double   yp_l = ref_l.get_state()[Bunch::yp];
        double  cdt_l = 0.0;
        double dpop_l = ref_l.get_state()[Bunch::dpop];

        if (ledge)
        {
            // slot
            FF_algorithm::slot_unit(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l, ce1, se1, pref_l, m_l);

            double p_l = 1.0 + dpop_l;
            double zp_l = sqrt(p_l*p_l - xp_l*xp_l - yp_l*yp_l);

            us_edge_k_x = us_edge_k_p * (xp_l/zp_l);
            us_edge_k_y = us_edge_k_p * (yp_l/zp_l);

            // edge
            //FF_algorithm::edge_unit(y_l, yp_l, us_edge_k/scale);
            //FF_algorithm::edge_unit(y_l, xp_l, yp_l, dpop_l, us_edge_k_p/scale);
            FF_algorithm::edge_unit(y_l, xp_l, yp_l, us_edge_k_x/scale, us_edge_k_y/scale, 0);

            // bend edge (thin)
            FF_algorithm::bend_edge(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l, e1, phase_e1, strength, pref_l, m_l);
        }

        FF_algorithm::bend_yoshida6< double, 
                                     FF_algorithm::thin_cf_kick_2<double>, 
                                     FF_algorithm::sbend_unit_phase,
                                     FF_algorithm::sbend_unit_term,
                                     FF_algorithm::sbend_dphi,
                                     2 >
            ( x_l, xp_l, y_l, yp_l, cdt_l, dpop_l,
              pref_l, m_l, 0.0 /* step ref_cdt */,
              step_angle, step_kl_l,
              r0, strength, steps );

        if (redge)
        {
            // bend edge (thin)
            FF_algorithm::bend_edge(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l, e2, phase_e2, strength, pref_l, m_l);

            double p_l = 1.0 + dpop_l;
            double zp_l = sqrt(p_l*p_l - xp_l*xp_l - yp_l*yp_l);

            ds_edge_k_x = ds_edge_k_p * (xp_l/zp_l);
            ds_edge_k_y = ds_edge_k_p * (yp_l/zp_l);

            // edge
            //FF_algorithm::edge_unit(y_l, yp_l, ds_edge_k);
            //FF_algorithm::edge_unit(y_l, xp_l, yp_l, dpop_l, ds_edge_k_p/scale);
            FF_algorithm::edge_unit(y_l, xp_l, yp_l, ds_edge_k_x/scale, ds_edge_k_y/scale, 0);

            // slot
            FF_algorithm::slot_unit(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l, ce2, se2, pref_l, m_l);
        }

        ref_l.set_state(x_l, xp_l, y_l, yp_l, cdt_l, dpop_l);
        double ref_cdt = cdt_l;
        double step_ref_cdt = ref_cdt / steps;

        // with combined function
        // bunch particles
        {
            #pragma omp parallel for
            for (int part = 0; part < local_num; ++part)
            {
                double x   (particles[part][Bunch::x   ]);
                double xp  (particles[part][Bunch::xp  ]);
                double y   (particles[part][Bunch::y   ]);
                double yp  (particles[part][Bunch::yp  ]);
                double cdt (particles[part][Bunch::cdt ]);
                double dpop(particles[part][Bunch::dpop]);

                if (ledge)
                {
                    // slot
                    FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce1, se1, pref_b, m_b);

                    // edge
                    //FF_algorithm::edge_unit(y, yp, us_edge_k);
                    //FF_algorithm::edge_unit(y, xp, yp, dpop, us_edge_k_p);
                    FF_algorithm::edge_unit(y, xp, yp, us_edge_k_x, us_edge_k_y, 0);

                    // bend edge (thin, but with face angle)
                    FF_algorithm::bend_edge(x, xp, y, yp, cdt, dpop, e1, phase_e1, strength, pref_b, m_b);
                }

                // bend body
                FF_algorithm::bend_yoshida6< double, 
                                             FF_algorithm::thin_cf_kick_2<double>, 
                                             FF_algorithm::sbend_unit_phase,
                                             FF_algorithm::sbend_unit_term,
                                             FF_algorithm::sbend_dphi,
                                             2 >
                    ( x, xp, y, yp, cdt, dpop,
                      pref_b, m_b, step_ref_cdt,
                      step_angle, step_kl_b,
                      r0, strength, steps );

                if (redge)
                {
                    // bend edge (thin, but with face angle)
                    FF_algorithm::bend_edge(x, xp, y, yp, cdt, dpop, e2, phase_e2, strength, pref_b, m_b);

                    // edge
                    //FF_algorithm::edge_unit(y, yp, ds_edge_k);
                    //FF_algorithm::edge_unit(y, xp, yp, dpop, ds_edge_k_p);
                    FF_algorithm::edge_unit(y, xp, yp, ds_edge_k_x, ds_edge_k_y, 0);

                    // slot
                    FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce2, se2, pref_b, m_b);
                }

                particles[part][Bunch::x]  = x;
                particles[part][Bunch::xp] = xp;
                particles[part][Bunch::y]  = y;
                particles[part][Bunch::yp] = yp;
                particles[part][Bunch::cdt] = cdt;
            }
        }

        // bunch spectator particles
        {
            #pragma omp parallel for
            for (int part = 0; part < local_s_num; ++part)
            {
                double x   (s_particles[part][Bunch::x   ]);
                double xp  (s_particles[part][Bunch::xp  ]);
                double y   (s_particles[part][Bunch::y   ]);
                double yp  (s_particles[part][Bunch::yp  ]);
                double cdt (s_particles[part][Bunch::cdt ]);
                double dpop(s_particles[part][Bunch::dpop]);

                if (ledge)
                {
                    // slot
                    FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce1, se1, pref_b, m_b);

                    // edge
                    //FF_algorithm::edge_unit(y, yp, us_edge_k);
                    //FF_algorithm::edge_unit(y, xp, yp, dpop, us_edge_k_p);
                    FF_algorithm::edge_unit(y, xp, yp, us_edge_k_x, us_edge_k_y, 0);

                    // bend edge (thin, but with face angle)
                    FF_algorithm::bend_edge(x, xp, y, yp, cdt, dpop, e1, phase_e1, strength, pref_b, m_b);
                }

                // bend body
                FF_algorithm::bend_yoshida6< double, 
                                             FF_algorithm::thin_cf_kick_2<double>, 
                                             FF_algorithm::sbend_unit_phase,
                                             FF_algorithm::sbend_unit_term,
                                             FF_algorithm::sbend_dphi,
                                             2 >
                    ( x, xp, y, yp, cdt, dpop,
                      pref_b, m_b, step_ref_cdt,
                      step_angle, step_kl_b,
                      r0, strength, steps );

                if (redge)
                {
                    // bend edge (thin, but with face angle)
                    FF_algorithm::bend_edge(x, xp, y, yp, cdt, dpop, e2, phase_e2, strength, pref_b, m_b);

                    // edge
                    //FF_algorithm::edge_unit(y, yp, ds_edge_k);
                    //FF_algorithm::edge_unit(y, xp, yp, dpop, ds_edge_k_p);
                    FF_algorithm::edge_unit(y, xp, yp, ds_edge_k_x, ds_edge_k_y, 0);

                    // slot
                    FF_algorithm::slot_unit(x, xp, y, yp, cdt, dpop, ce2, se2, pref_b, m_b);
                }

                s_particles[part][Bunch::x]  = x;
                s_particles[part][Bunch::xp] = xp;
                s_particles[part][Bunch::y]  = y;
                s_particles[part][Bunch::yp] = yp;
                s_particles[part][Bunch::cdt] = cdt;
            }
        }
    }

    bunch.get_reference_particle().increment_trajectory(length);

    t = simple_timer_show(t, "libff-sbend");
}

template<class Archive>
    void
    FF_sbend::serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(FF_element);
    }

template
void
FF_sbend::serialize<boost::archive::binary_oarchive >(
        boost::archive::binary_oarchive & ar, const unsigned int version);

template
void
FF_sbend::serialize<boost::archive::xml_oarchive >(
        boost::archive::xml_oarchive & ar, const unsigned int version);

template
void
FF_sbend::serialize<boost::archive::binary_iarchive >(
        boost::archive::binary_iarchive & ar, const unsigned int version);

template
void
FF_sbend::serialize<boost::archive::xml_iarchive >(
        boost::archive::xml_iarchive & ar, const unsigned int version);

FF_sbend::~FF_sbend()
{

}

