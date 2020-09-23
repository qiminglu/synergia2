#ifndef BUNCH_H_
#define BUNCH_H_

#include <sstream>
#include <vector>
#include <memory>

#include "synergia/foundation/reference_particle.h"

#include "synergia/bunch/bunch_particles.h"
#include "synergia/bunch/diagnostics_worker.h"
#include "synergia/bunch/diagnostics_loss.h"

#include "synergia/utils/commxx.h"
#include "synergia/utils/hdf5_file.h"
#include "synergia/utils/restrict_extension.h"
#include "synergia/utils/logger.h"

#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>

enum class ParticleGroup
{
    regular = 0,
    spectator = 1
};

enum class LongitudinalBoundary
{
    open = 0,
    periodic = 1,
    aperture = 2,
    bucket_barrier = 3
};

/// Represents a macroparticle bunch distributed across the processors
/// in a comm_sptrunicator.
class Bunch
{
private:

    using PG = ParticleGroup;
    using LB = LongitudinalBoundary;

public:
    /*! \enum State The state of the bunch is captured at a fixed  s (or z, longitudinal coordinate)
     or at a fixed time.  In the former case, particles are found within a range of different time
     coordinates while in the later case particles position along the beam axis do vary.
     A change of state is accomplish via the fixed_t_z_converter class.
     */
    constexpr static const int x    = 0;
    constexpr static const int xp   = 1;
    constexpr static const int y    = 2;
    constexpr static const int yp   = 3;
    constexpr static const int z    = 4;
    constexpr static const int zp   = 5;
    constexpr static const int cdt  = 4;
    constexpr static const int dpop = 5;
    constexpr static const int id   = 6;

    constexpr static const int particle_index_null = 
        BunchParticles::particle_index_null;

private:

    std::shared_ptr<Commxx> comm;    

    // meaning of bounary_param for each boundary condition:
    // open (N/A), periodic (z-period), z-cut (longitudinal_extent),
    // bucket_barrier (bucket_length)
    LongitudinalBoundary boundary;
    double boundary_param;

    // reference particle and design reference particle
    Reference_particle ref_part;
    Reference_particle design_ref_part;

    int particle_charge;

    double real_num;

    // parts[0]: PG::regular particles
    // parts[1]: spectator particles
    std::array<BunchParticles, 2> parts;

    // diagnostics
    std::vector<Diagnostics_worker> diags;

    // diagnostics for particle losses
    std::unique_ptr<Diagnostics_worker> diag_aperture;
    std::unique_ptr<Diagnostics_worker> diag_zcut;

    // bunch indicies
    int bunch_index;    // index in the train
    int bucket_index;  // which bucket its occupying
    int array_index;   // array index in the train's bunch array

private:

    //std::string get_local_particles_serialization_path() const;

public:
    //!
    //! Constructor:
    //! Allocates memory for the particles and assigns particle ID's,
    //!    but does not fill the phase space values in any way.
    //!
    //! To fill the bunch with particles, use the populate methods.
    /// @param reference_particle the reference particle for the bunch.
    /// @param total_num the total number of macroparticles in the bunch
    /// @param real_num the number of real particles represented by the bunch.
    /// @param bucket_index the bucket number the  bunch occupies, used for multi-bunch simulations
    /// @param comm_sptr the comm_sptrunicator.
    Bunch(  Reference_particle const& reference_particle, 
            int total_num,
            double real_num, 
            Commxx comm = Commxx(),
            int total_spectator_num = 0,
            int bunch_index = 0,
            int bucket_index = 0,
            int array_index = 0 );

    // default ctor for serialization only
    Bunch();
    
    // non-copyable but moveable
    Bunch(Bunch const&) = delete;
    Bunch(Bunch &&) = default;

    // indicies
    int get_array_index()  const { return array_index; }
    int get_bunch_index()  const { return bunch_index; }
    int get_bucket_index() const { return bucket_index; }

    ///
    /// Set the particle charge
    /// @param particle_charge in units of e.
    void set_particle_charge(int charge)
    { particle_charge = charge; }

    ///
    /// Set the number of real particles represented by the bunch.
    /// @param real_num the new real number of particles
    void set_real_num(double num)
    { real_num = num; }

    /// Return the reference particle
    Reference_particle &      get_reference_particle()       
    { return ref_part; }

    Reference_particle const& get_reference_particle() const 
    { return ref_part; }

    Reference_particle &      get_design_reference_particle()       
    { return design_ref_part; }

    Reference_particle const& get_design_reference_particle() const 
    { return design_ref_part; }

    void set_design_reference_particle(Reference_particle const & ref_part)
    { design_ref_part = ref_part; }

    // BunchParticles
    BunchParticles &      get_bunch_particles(ParticleGroup pg = PG::regular)
    { return parts[(int)pg]; }

    BunchParticles const& get_bunch_particles(ParticleGroup pg = PG::regular) const
    { return parts[(int)pg]; }

    /// Get the array containing the macroparticles on this processor.
    /// The array has length (length,7), where length of the array may be
    /// larger local_num. The macroparticle state vectors are stored in
    /// array[0:local_num,0:6] and the macroparticle IDs are stored in
    /// array[0:local_num,6]. Use get_local_num() to obtain local_num.
    Particles          get_local_particles(ParticleGroup pg = PG::regular)       
    { return get_bunch_particles(pg).parts; }

    ConstParticles     get_local_particles(ParticleGroup pg = PG::regular) const 
    { return get_bunch_particles(pg).parts; }

    HostParticles      get_host_particles(ParticleGroup pg = PG::regular)       
    { return get_bunch_particles(pg).hparts; }

    ConstHostParticles get_host_particles(ParticleGroup pg = PG::regular) const 
    { return get_bunch_particles(pg).hparts; }

    ParticleMasks      get_local_particle_masks(ParticleGroup pg = PG::regular)
    { return get_bunch_particles(pg).masks; }

    ConstParticleMasks get_local_particle_masks(ParticleGroup pg = PG::regular) const
    { return get_bunch_particles(pg).masks; }

    int get_total_num(ParticleGroup pg = PG::regular) const 
    { return get_bunch_particles(pg).num_total(); }

    int get_local_num(ParticleGroup pg = PG::regular) const
    { return get_bunch_particles(pg).num_valid(); }

    int size(ParticleGroup pg = PG::regular) const 
    { return get_bunch_particles(pg).size(); }

    int capacity(ParticleGroup pg = PG::regular) const 
    { return get_bunch_particles(pg).capacity(); }

    void reserve(int n, ParticleGroup pg = PG::regular)
    { get_bunch_particles(pg).reserve(n, *comm); }

    void reserve_local(int n, ParticleGroup pg = PG::regular)
    { get_bunch_particles(pg).reserve_local(n); }

#if 0
    ///
    /// Reduce (set) the number of particles on this processor. The number
    /// of particles can only be lowered by this member function. (In order
    /// to add new particles, create another Bunch and use the inject member.)
    /// The total number and real number for the bunch will not be correct
    /// until update_total_num() is called. The real number will scale to
    /// reflect the change in the total number. n.b.: The only way to change
    /// the total number after the bunch has been created is to change the
    /// local numbers on each processor.
    /// @param local_num the new number of particles on this processor
    void set_local_num(int num, ParticleGroup pg = PG::regular)
    { get_bunch_particles(pg).set_local_num(num); }

    ///
    /// Set the total number (and the real number) of particles
    void set_total_num(int num, ParticleGroup pg = PG::regular)
    { get_bunch_particles(pg).set_total_num(num); }
#endif
 
    ///
    /// Update the total number and real number of particles after the local
    /// number has been changed. Requires comm_sptrunication.
    int update_total_num()
    { 
        get_bunch_particles(PG::spectator).update_total_num(*comm); 

        auto & bp = get_bunch_particles(PG::regular);
        int old_total = bp.update_total_num(*comm);
        real_num = old_total ? bp.num_total() * real_num / old_total : 0.0;

        return old_total;
    }

    // aperture operation
    // discard the particles (by moving them to the tail of the array) filtered out
    // by the aperture, returns the number of particles discarded
    template<typename AP>
    int apply_aperture(AP const& ap, ParticleGroup pg = PG::regular);

    template<typename AP>
    int apply_zcut(AP const& ap, ParticleGroup pg = PG::regular);

    // retrieve the array holding lost particles from last aperture operation
    karray2d_row
    get_particles_last_discarded(ParticleGroup pg = PG::regular) const
    { return get_bunch_particles(pg).get_particles_last_discarded(); }
  
    // checkout (deep_copy) the entire particle array from device
    // memory to the host memory for user to access the latest
    // particle data
    void checkout_particles(ParticleGroup pg = PG::regular) 
    { get_bunch_particles(pg).checkout_particles(); }

    void checkin_particles (ParticleGroup pg = PG::regular)
    { get_bunch_particles(pg).checkin_particles(); }

    // checkout (deep_copy) num particles starting from idx, and
    // store them in a host array
    // TODO: when compiled for host, it returns a subview to the original
    // particle data -- overhead for the operation should be minimal
    karray2d_row 
    get_particles_in_range(int idx, int num, ParticleGroup pg = PG::regular) const
    { return get_bunch_particles(pg).get_particles_in_range(idx, num).first; }

    karray1d_row 
    get_particle(int idx, ParticleGroup pg = PG::regular) const
    { return get_bunch_particles(pg).get_particle(idx).first; }

    // find the index of the given particle_id (pid)
    // if last_idx is provided, it does the search form the last_idx first
    // returns particle_index_null if the given particle_id is not found
    int search_particle(int pid, int last_idx = particle_index_null, 
            ParticleGroup pg = PG::regular) const
    { return get_bunch_particles(pg).search_particle(pid, last_idx); }

    void print_particle(size_t idx, Logger& logger, 
            ParticleGroup pg = PG::regular) const
    { get_bunch_particles(pg).print_particle(idx, logger); }

    // print statistics
    // spectator particles are not included
    void print_statistics(Logger& logger) const;

    /// Get the particle charge in units of e.
    int get_particle_charge() const { return particle_charge; }

    /// Get the particle mass in units GeV/c^2.
    double get_mass() const { return ref_part.get_four_momentum().get_mass(); }

    /// Get the real number of particles represented by the bunch.
    double get_real_num() const { return real_num; }

    /// longitudinal boundary conditions
    void set_longitudinal_boundary(LB lb, double param = 0.0)
    { boundary = lb; boundary_param = param; }

    std::pair<LB, double> get_longitudinal_boundary() const
    { return std::make_pair(boundary, boundary_param); }

    // bucket index
    void set_bucket_index(int index)      { bucket_index = index; }
    bool is_bucket_index_assigned() const { return bucket_index >= 0; }

    /// Get the communicator
    Commxx const& get_comm() const { return *comm; }

    // Diagnostics
    template<class Diag>
    std::pair<Diagnostics_handler, int>
    add_diagnostics(Diag const& diag)
    { 
        diags.emplace_back(diag, comm); 
        return std::make_pair(
                Diagnostics_handler(diags.back(), *this), 
                diags.size() - 1);
    }

#if 0
    Diagnostics_worker& 
    get_diag(std::string const & name);

    std::string diag_type(std::string const& name)
    { return get_diag(name).type(); }

    void diag_update(std::string const& name)
    { get_diag(name).update(*this); }

    void diag_update_and_write(std::string const& name)
    { get_diag(name).update_and_write(*this); }
#endif

    Diagnostics_handler get_diag(int id)
    { return Diagnostics_handler(diags[id], *this); }

    std::string diag_type(int id) const
    { return diags[id].type(); }

    void diag_update(int id)
    { get_diag(id).update(); }

    void diag_update_and_write(int id)
    { get_diag(id).update_and_write(); }

    void set_diag_loss_aperture(std::string const& filename)
    { diag_aperture.reset( new Diagnostics_worker(
                Diagnostics_loss(filename), comm)); }

    void set_diag_loss_zcut(std::string const& filename)
    { diag_zcut.reset( new Diagnostics_worker(
                Diagnostics_loss(filename), comm)); }

    /// Add a copy of the particles in bunch to the current bunch. The
    /// injected bunch must have the same macroparticle weight, i.e.,
    /// real_num/total_num. If the state vectors of the reference particles
    /// of the two bunches differ, the particles will be shifted accordingly.
    void inject(Bunch const& bunch);
    
    void check_pz2_positive()
    {
        get_bunch_particles(ParticleGroup::regular).check_pz2_positive();
        get_bunch_particles(ParticleGroup::spectator).check_pz2_positive();
    }

    // read/write particles
    void read_file_legacy(std::string const& filename)
    { 
        Hdf5_file file(filename, Hdf5_file::read_only, comm);
        get_bunch_particles(PG::regular).read_file_legacy(file, *comm); 
    }

    void read_file(std::string const& filename)
    { 
        Hdf5_file file(filename, Hdf5_file::read_only, comm);
        get_bunch_particles(PG::regular).read_file(file, *comm); 
    }

    // num_part = -1 means write all particles
    void write_file(std::string const& filename,
            int num_part = -1, int offset = 0,
            int num_part_spec = 0, int offset_spec = 0 ) const
    {
        Hdf5_file file(filename, Hdf5_file::truncate, comm);
        write_file(file, num_part, offset, num_part_spec, offset_spec);
    }

    void write_file(Hdf5_file const& file,
            int num_part, int offset,
            int num_part_spec, int offset_spec) const
    {
        get_bunch_particles(PG::regular).write_file(file, num_part, offset, *comm);
        get_bunch_particles(PG::spectator).write_file(file, num_part_spec, offset_spec, *comm);
    }

    // checkpoint state
    std::string dump() const
    {
        std::stringstream ss;
        {
            cereal::JSONOutputArchive ar(ss);
            ar(*this);
        }
        return ss.str();
    }

    void load(std::string const& str)
    {
        std::stringstream ss(str);
        cereal::JSONInputArchive ar(ss);
        ar(*this);
    }

    // checkpoint partiles
    void save_checkpoint_particles(Hdf5_file & file, int idx) const
    {
        get_bunch_particles(PG::regular).save_checkpoint_particles(file, idx);
        get_bunch_particles(PG::spectator).save_checkpoint_particles(file, idx);
    }

    void load_checkpoint_particles(Hdf5_file & file, int idx)
    {
        get_bunch_particles(PG::regular).load_checkpoint_particles(file, idx);
        get_bunch_particles(PG::spectator).load_checkpoint_particles(file, idx);
    }

private:

    friend class cereal::access;

    template<class Archive>
    void serialize(Archive & ar)
    {
        ar(CEREAL_NVP(comm));
        ar(CEREAL_NVP(boundary));
        ar(CEREAL_NVP(boundary_param));
        ar(CEREAL_NVP(ref_part));
        ar(CEREAL_NVP(design_ref_part));
        ar(CEREAL_NVP(particle_charge));
        ar(CEREAL_NVP(real_num));
        ar(CEREAL_NVP(parts));
        ar(CEREAL_NVP(diags));
        ar(CEREAL_NVP(diag_aperture));
        ar(CEREAL_NVP(diag_zcut));
        ar(CEREAL_NVP(bunch_index));
        ar(CEREAL_NVP(bucket_index));
        ar(CEREAL_NVP(array_index));
    }
};

template<typename AP>
inline int Bunch::apply_aperture(AP const& ap, ParticleGroup pg)
{ 
    // Particles might get lost here. The update of total particle number is
    // performed at the end of each independent operator on a per-bunch basis.
    // So there will be a short period of time the total_num isnt consistent
    // with the sum of all local_num. It should be OK since the total_num is
    // only important to the space charge solvers
    int ndiscarded = get_bunch_particles(pg).apply_aperture(ap); 

    // diagnostics
    if (ndiscarded && diag_aperture)
        diag_aperture->update_and_write(*this);

    return ndiscarded;
}

template<typename AP>
inline int Bunch::apply_zcut(AP const& ap, ParticleGroup pg)
{ 
    int ndiscarded = get_bunch_particles(pg).apply_aperture(ap); 

    // diagnostics
    if (ndiscarded && diag_zcut)
        diag_zcut->update_and_write(*this);

    return ndiscarded;
}


#endif /* BUNCH_H_ */
