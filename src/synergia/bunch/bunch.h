#ifndef BUNCH_H_
#define BUNCH_H_

#include <sstream>
#include <iomanip>
#include <vector>
#include <memory>

#include <mpi.h>

#include "synergia/utils/multi_array_typedefs.h"
#include "synergia/foundation/reference_particle.h"

#include "synergia/bunch/diagnostics.h"
#include "synergia/bunch/diagnostics_loss.h"

#include "synergia/utils/commxx.h"
#include "synergia/utils/hdf5_file.h"
#include "synergia/utils/restrict_extension.h"
#include "synergia/utils/logger.h"

#include <Kokkos_Core.hpp>

typedef Kokkos::View<double*[7], Kokkos::LayoutLeft> Particles;
typedef Kokkos::View<const double*[7], Kokkos::LayoutLeft> ConstParticles;

typedef Particles::HostMirror HostParticles;
typedef ConstParticles::HostMirror ConstHostParticles;

/// Represents a macroparticle bunch distributed across the processors
/// in a comm_sptrunicator.
class Bunch
{
public:
    /*! \enum State The state of the bunch is captured at a fixed  s (or z, longitudinal coordinate)
     or at a fixed time.  In the former case, particles are found within a range of different time
     coordinates while in the later case particles position along the beam axis do vary.
     A change of state is accomplish via the fixed_t_z_converter class.
     */
    enum State
    {   
        fixed_z_lab = 1,
        fixed_t_lab = 2,
        fixed_t_bunch = 3,       
        fixed_z_bunch = 4
    };

    constexpr static const int x    = 0;
    constexpr static const int xp   = 1;
    constexpr static const int y    = 2;
    constexpr static const int yp   = 3;
    constexpr static const int z    = 4;
    constexpr static const int zp   = 5;
    constexpr static const int cdt  = 4;
    constexpr static const int dpop = 5;
    constexpr static const int id   = 6;

    const static int particle_alignment;

    constexpr static const int particle_index_null = -1;

private:

    double longitudinal_extent;    
    bool z_periodic;
    bool longitudinal_aperture;

    Reference_particle reference_particle;
    Reference_particle design_reference_particle;
    int particle_charge;

    /*
     * Local Particle Array Memory Layout:
     *
     *   P: particle, O: padding, L: lost particle
     *
     *   +=====+
     *   |  P  |
     *   +-----+
     *   |  P  |
     *   +-----+
     *   | ... |
     *   +=====+  <- local_num
     *   |  O  |
     *   +-----+  <- local_num_aligned
     *   |  O  |
     *   +-----+
     *   | ... |
     *   +=====+  <- local_num_padded
     *   |  L  |
     *   +-----+
     *   |  L  |
     *   +-----+
     *   | ... |
     *   +=====+  <- local_num_slots
     *
     *   * number of padding slots  = local_num_padded - local_num
     *   * number of lost particles = local_num_slots - local_num_padded
     *
     *   At bunch construction the size of padding (num_padding) is decided 
     *   such that the local_num_slots is always aligned (depending on the 
     *   vector specification, e.g., SSE or AVX or AVX512). 
     *
     *   local_num_aligned is initialized in the range [local_num, 
     *   local_num_paded], and gets adjusted everytime the local_num 
     *   changes such tht local_num_aligned is always aligned.
     *
     */

    int local_num, local_num_aligned, local_num_padded, local_num_slots;
    int local_s_num, local_s_num_aligned, local_s_num_padded, local_s_num_slots;

    int total_num, total_s_num;

    double real_num;

    Particles parts;
    HostParticles hparts;

    Particles sparts;
    HostParticles hsparts;

    int bucket_index;
    bool bucket_index_assigned;

    Commxx comm;    

    std::map<std::string, std::unique_ptr<Diagnostics>> diags;

    std::unique_ptr<Diagnostics_loss> diag_aperture;
    std::unique_ptr<Diagnostics_loss> diag_zcut;

private:

    void assign_ids(int local_offset);
    void assign_spectator_ids(int local_offset);

    std::string get_local_particles_serialization_path() const;
    void construct(int total_num, double real_num, int total_s_num);

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
            Commxx comm,
            int bucket_index = 0 );

    Bunch(  Reference_particle const& reference_particle, 
            int total_num, 
            int total_spectator_num,
            double real_num, 
            Commxx comm );
    
    // non-copyable but moveable
    Bunch(Bunch const&) = delete;
    Bunch(Bunch &&) = default;

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
    void set_local_num(int local_num);
    void set_local_spectator_num(int local_s_num);

    void expand_local_num(int num, int added_lost);
    void expand_local_spectator_num(int s_num, int added_lost);

    ///
    /// Update the total number and real number of particles after the local
    /// number has been changed. Requires comm_sptrunication.
    void update_total_num();
    
    ///
    /// Set the total number (and the real number) of particles
    void set_total_num(int totalnum);
    
    /// For a given number of particles, returns the next alignement position
    /// @param num number of particles
    int calculate_aligned_pos(int num);

    /// For a given number of particles, returns the needed size of padding
    /// for the particle array to be aligned
    /// @param num number of particles
    int calculate_padding_size(int num);

    /// Return the reference particle
    Reference_particle       & get_reference_particle();
    Reference_particle const & get_reference_particle() const;

    Reference_particle       & get_design_reference_particle();
    Reference_particle const & get_design_reference_particle() const;

    void set_design_reference_particle(Reference_particle const & ref_part);

    /// Get the array containing the macroparticles on this processor.
    /// The array has length (length,7), where length of the array may be
    /// larger local_num. The macroparticle state vectors are stored in
    /// array[0:local_num,0:6] and the macroparticle IDs are stored in
    /// array[0:local_num,6]. Use get_local_num() to obtain local_num.
    Particles get_local_particles()
    { return parts; }

    ConstParticles get_local_particles() const
    { return parts; }

    HostParticles get_host_particles()
    { return hparts; }

    ConstHostParticles get_host_particles() const
    { return hparts; }


    Particles get_local_spectator_particles()
    { return sparts; }

    ConstParticles get_local_spectator_particles() const
    { return sparts; }

    HostParticles get_host_spectator_particles()
    { return hsparts; }

    ConstHostParticles get_host_spectator_particles() const
    { return hsparts; }


    // checkout (deep_copy) the entire particle array from device
    // memory to the host memory for user to access the latest
    // particle data
    void checkout_particles()
    { Kokkos::deep_copy(hparts, parts); }

    void checkin_particles()
    { Kokkos::deep_copy(parts, hparts); }

    // checkout (deep_copy) num particles starting from idx, and
    // store them in a host array
    // TODO: when compiled for host, it returns a subview to the original
    // particle data -- overhead for the operation should be minimal
    karray2d_row get_particles_in_range(int idx, int num) const;
    karray1d_row get_particle(int idx) const;

    // find the index of the given particle_id (pid)
    // if last_idx is provided, it does the search form the last_idx first
    // returns particle_index_null if the given particle_id is not found
    int search_particle(int pid, int last_idx = particle_index_null) const;


    void print_particle(size_t idx, Logger & logger) const;

    std::array<size_t, 2> get_particle_strides() const
    { 
        std::array<size_t, 2> strides;
        parts.stride(strides.data());
        return strides;
    }


    /// Get the particle charge in units of e.
    int get_particle_charge() const;

    /// Get the particle mass in units GeV/c^2.
    double get_mass() const;

    /// Get the real number of particles represented by the bunch.
    double get_real_num() const;

    /// Get the period length of the bunch
    double get_z_period_length() const;
    
     /// Set the period length of the bunch and make the bunch z_periodic     
    void set_z_period_length(double z_period_length) ;
       
    /// Is the bunch periodic?
    bool is_z_periodic() const; 
    
    /// Get the the bunch extent if the longitudinal aperture is present
    double get_longitudinal_aperture_length() const;

    /// Set the longitudinal_extent of the bunch and make the longitudinal aperture true
    void set_longitudinal_aperture_length(double longitudinal_length);
    
    
    /// True when the longitudinal aperture is present and the bunch is cut after every  operation
    /// longitudinally outside the extent [-longitudinal_extent/2,  longitudinal_extent/2]    
    bool has_longitudinal_aperture() const; 


    /// Get the number of macroparticles stored on this processor.
    int get_local_num() const;

    /// Get the number of padded macroparticles (first dimension of the particles[][] array)
    int get_local_num_slots() const;
    int get_local_num_aligned() const;
    int get_local_num_padded() const;
    int get_local_num_padding() const;
    int get_local_num_lost() const;

    /// Get the total number of macroparticles.
    int get_total_num() const;

    /// Get the number of spectator particles stored on this processor.
    int get_local_spectator_num() const;
    int get_local_spectator_num_slots() const;
    int get_local_spectator_num_aligned() const;
    int get_local_spectator_num_padded() const;
    int get_local_spectator_num_padding() const;
    int get_local_spectator_num_lost() const;

    /// Get the total number of spectator particles.
    int get_total_spectator_num() const;

    void set_bucket_index(int index);
    int  get_bucket_index() const;
    bool is_bucket_index_assigned() const;

    /// Get the (fixed-t or fixed-z) state.
    State get_state() const;

    /// Get the communicator
    Commxx const& get_comm() const;

    // Diagnostics
    template<typename Diag>
    void add_diagnostics(std::string const& name, Diag&& diag)
    { diags.emplace(name, std::make_unique<Diag>(std::move(diag)))
        .first->second->set_bunch(*this); }

    Diagnostics & get_diag(std::string const & name);

    void set_diag_loss_aperture(Diagnostics_loss && diag)
    { diag_aperture = std::make_unique<Diagnostics_loss>(std::move(diag)); }

    void set_diag_loss_zcut(Diagnostics_loss && diag)
    { diag_zcut = std::make_unique<Diagnostics_loss>(std::move(diag)); }

    Diagnostics_loss * get_diag_loss_aperture()
    { return diag_aperture.get(); }

    Diagnostics_loss * get_diag_loss_zcut()
    { return diag_zcut.get(); }

    /// Add a copy of the particles in bunch to the current bunch. The
    /// injected bunch must have the same macroparticle weight, i.e.,
    /// real_num/total_num. If the state vectors of the reference particles
    /// of the two bunches differ, the particles will be shifted accordingly.
    void inject(Bunch const& bunch);
    
    void read_file(std::string const &);

    void check_pz2_positive();
    
    /// serialization
    template<class Archive> 
    void save(Archive & ar, const unsigned int version) const;

    template<class Archive>
    void load(Archive & ar, const unsigned int version);
};

#endif /* BUNCH_H_ */
