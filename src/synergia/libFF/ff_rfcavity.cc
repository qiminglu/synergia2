#include "ff_rfcavity.h"
#include "ff_algorithm.h"
#include "synergia/lattice/chef_utils.h"

FF_rfcavity::FF_rfcavity()
{

}

double get_reference_cdt(double length, Reference_particle & reference_particle)
{
    double x(reference_particle.get_state()[Bunch::x]);
    double xp(reference_particle.get_state()[Bunch::xp]);
    double y(reference_particle.get_state()[Bunch::y]);
    double yp(reference_particle.get_state()[Bunch::yp]);
    double cdt(reference_particle.get_state()[Bunch::cdt]);
    double dpop(reference_particle.get_state()[Bunch::dpop]);
    double reference_momentum = reference_particle.get_momentum();
    double m = reference_particle.get_mass();

    double cdt_orig = cdt;
    FF_algorithm::drift_unit(x, xp, y, yp, cdt, dpop, length, reference_momentum, m, 0.0);

    return cdt - cdt_orig;
}


void FF_rfcavity::apply(Lattice_element_slice const& slice, JetParticle& jet_particle)
{
    double length = slice.get_right() - slice.get_left();
    double freq = 0.0;

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

    rfcavity_unit(x, xp, y, yp, cdt, dpop,
                  length, freq,
                  reference_momentum, m, reference_brho);
}

void FF_rfcavity::apply(Lattice_element_slice const& slice, Bunch& bunch)
{
    double length = slice.get_right() - slice.get_left();
    double freq = 0.0;

    int local_num = bunch.get_local_num();
    MArray2d_ref particles = bunch.get_local_particles();

    double reference_momentum = bunch.get_reference_particle().get_momentum();
    double reference_brho     = reference_momentum / PH_CNV_brho_to_p;
    double m = bunch.get_mass();

    double reference_cdt = get_reference_cdt(length, bunch.get_reference_particle());

    for (int part = 0; part < local_num; ++part) {
        double x   (particles[part][Bunch::x   ]);
        double xp  (particles[part][Bunch::xp  ]);
        double y   (particles[part][Bunch::y   ]);
        double yp  (particles[part][Bunch::yp  ]);
        double cdt (particles[part][Bunch::cdt ]);
        double dpop(particles[part][Bunch::dpop]);

#if 0
        rfcavity_unit(x, xp, y, yp, cdt, dpop,
                      length, freq,
                      reference_momentum, m, reference_brho);
#endif

        FF_algorithm::drift_unit(x, xp, y, yp, cdt, dpop, 
                length, reference_momentum, m, reference_cdt);

        particles[part][Bunch::x]  = x;
        particles[part][Bunch::xp] = xp;
        particles[part][Bunch::y]  = y;
        particles[part][Bunch::cdt] = cdt;
    }

    bunch.get_reference_particle().increment_trajectory(length);
}

template<class Archive>
    void
    FF_rfcavity::serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(FF_element);
    }

template
void
FF_rfcavity::serialize<boost::archive::binary_oarchive >(
        boost::archive::binary_oarchive & ar, const unsigned int version);

template
void
FF_rfcavity::serialize<boost::archive::xml_oarchive >(
        boost::archive::xml_oarchive & ar, const unsigned int version);

template
void
FF_rfcavity::serialize<boost::archive::binary_iarchive >(
        boost::archive::binary_iarchive & ar, const unsigned int version);

template
void
FF_rfcavity::serialize<boost::archive::xml_iarchive >(
        boost::archive::xml_iarchive & ar, const unsigned int version);

FF_rfcavity::~FF_rfcavity()
{

}

