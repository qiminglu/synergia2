#ifndef MAPPING_FIXTURE_H_
#define MAPPING_FIXTURE_H_
#include "fast_mapping_term_fixture.h"
#include "chef_elements_fixture.h"
#include "bunch_fixture.h"

struct Mapping_fixture
{
    Mapping_fixture()
    {
        BOOST_TEST_MESSAGE("setup Mapping fixture");
        JetParticle jet_particle = reference_particle_to_chef_jet_particle(
                b.reference_particle, order);
        for (Chef_elements::const_iterator it = c.chef_elements.begin(); it
                != c.chef_elements.end(); ++it) {
            (*it)->propagate(jet_particle);
        }
        mapping = jet_particle.State();
    }
    ~Mapping_fixture()
    {
        BOOST_TEST_MESSAGE("teardown Mapping fixture");
    }
    Bunch_fixture b;
    Chef_elements_fixture c;
    Mapping mapping;
};

#endif /* MAPPING_FIXTURE_H_ */
