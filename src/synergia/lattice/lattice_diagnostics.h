#ifndef LATTICE_DIAGNOSTICS_H_
#define LATTICE_DIAGNOSTICS_H_

#include "synergia/foundation/diagnostics_write_helper.h"
#include "synergia/utils/hdf5_serial_writer.h"
#include "synergia/lattice/lattice.h"

class Lattice_diagnostics
{
private:
    Lattice_sptr lattice_sptr;
    std::string filename;
    std::string attribute;
    double default_value;
    bool reduce;
    MPI_Op reduce_op;
    int initial_lattice_size;
    Diagnostics_write_helper write_helper;
    Hdf5_serial_writer<MArray1d_ref > * writer;
    bool first_time;
public:
    Lattice_diagnostics(Lattice_sptr lattice_sptr, std::string const& filename,
            std::string const& attribute, std::string const& local_dir = "");

    void
    set_default_value(double value);

    double
    get_default_value() const;

    void
    set_reduce(bool reduce);

    bool
    get_reduce() const;

    void
    set_reduce_op(MPI_Op op);

    MPI_Op
    get_reduce_op() const;

    /// Multiple serial diagnostics can be written to a single file.
    virtual bool
    is_serial() const;

    /// Update the diagnostics
    virtual void
    update();

    /// Write the diagnostics to the file
    virtual void
    write();

    /// Update the diagnostics and write to file
    virtual void
    update_and_write();

    virtual
    ~Lattice_diagnostics();
};

typedef boost::shared_ptr<Lattice_diagnostics > Lattice_diagnostics_sptr;

#endif /* LATTICE_DIAGNOSTICS_H_ */
