#ifndef FF_ELEMENT_H
#define FF_ELEMENT_H

#include "synergia/bunch/bunch.h"
#include "synergia/lattice/lattice_element_slice.h"
#include "synergia/lattice/lattice_element.h"
#include "synergia/lattice/lattice.h"
#include "synergia/foundation/trigon_particle.h"

class JetParticle;

inline bool close_to_zero(double v)
{ return fabs(v) < 1e-13; }

class FF_element
{
public:
    FF_element()
    : steps(default_steps), order(default_order) { };

    virtual ~FF_element() = default;

#if 0
    template<size_t I>
    virtual void apply(Lattice_element_slice const& slice, 
            Trigon_particle<I> & trigon)
    { }
#endif

    virtual void apply(Lattice_element_slice const& slice, 
            Bunch & bunch)
    { }

#if 0
    Reference_particle &
        get_ref_particle_from_slice(Lattice_element_slice & slice) const
    { return slice.get_lattice_element().get_lattice().get_reference_particle(); }
#endif

    Reference_particle const &
        get_ref_particle_from_slice(Lattice_element_slice const & slice) const
    { return slice.get_lattice_element().get_lattice().get_reference_particle(); }

    void set_yoshida_steps(int s)
    { steps = s; }

    void set_yoshida_order(int o)
    { order = o; }

    int get_yoshida_steps() const
    { return steps; }

    int get_yoshida_order() const
    { return order; }


    // static members
    static void set_default_yoshida_steps(int s)
    { default_steps = s; }

    static void set_default_yoshida_order(int o)
    { default_order = o; }

    static int get_default_yoshdia_steps()
    { return default_steps; }

    static int get_default_yoshida_order()
    { return default_order; }

    template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

protected:

    static int default_steps;
    static int default_order;

    int steps;
    int order;
};

#endif // FF_ELEMENT_H
