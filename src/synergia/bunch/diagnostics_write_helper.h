#ifndef DIAGNOSTICS_WRITE_HELPER_H_
#define DIAGNOSTICS_WRITE_HELPER_H_
#include <string>
#include <list>
#include "H5Cpp.h"
#include <boost/shared_ptr.hpp>

#include "synergia/utils/commxx.h"

/// The Diagnostics_write_helper is a helper class for Diagnostics objects.
/// Serial Diagnostics_write_helpers write many updates to a single file.
/// Non-serial Diagnostics_write_helpers write each update to a new file.
class Diagnostics_write_helper
{
public:
    static const int default_rank = -999;
private:
    int writer_rank;
    std::string filename;
    bool serial;
    Commxx commxx;
    boost::shared_ptr<H5::H5File> file_sptr;
    bool have_file;
    int count;
    std::string filename_base, filename_suffix;
    void
    open_file();
public:
    /// Construct Diagnostics_write_helper
    Diagnostics_write_helper(std::string const& filename, bool serial, Commxx const& commxx,
            int writer_rank = default_rank);

    /// Get the count for non-serial writers
    int
    get_count() const;

    /// Set the count for non-serial writers
    /// @param count the count
    void
    set_count(int count);

    bool
    write_locally();

    int
    get_writer_rank();

    H5::H5File &
    get_file();

    void
    finish_write();

    ~Diagnostics_write_helper();
};

typedef boost::shared_ptr<Diagnostics_write_helper > Diagnostics_write_helper_sptr;

#endif /* DIAGNOSTICS_WRITE_HELPER_H_ */
