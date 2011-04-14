#include "diagnostics_write_helper.h"
#include <sstream>
#include <iomanip>

void
Diagnostics_write_helper::open_file()
{
    if (!have_file) {
        std::stringstream sstream;
        sstream << filename_base;
        if (!serial) {
            sstream << "_";
            sstream << std::setw(4);
            sstream << std::setfill('0');
            sstream << count;
        }
        sstream << filename_suffix;
        file = H5Fcreate(sstream.str().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT,
                H5P_DEFAULT);
        have_file = true;
    }
}

Diagnostics_write_helper::Diagnostics_write_helper(std::string const& filename,
        bool serial, Commxx const& commxx) :
    writer_rank(commxx.get_size() - 1), filename(filename), serial(serial),
            commxx(commxx), count(0), have_file(false)
{
    int idx = filename.rfind('.');
    if (idx == std::string::npos) {
        filename_base = filename;
        filename_suffix = "";
    } else {
        filename_base = filename.substr(0, idx);
        filename_suffix = filename.substr(idx);
    }
    if (serial) {
        open_file();
    }
}

int
Diagnostics_write_helper::get_count() const
{
    return count;
}

void
Diagnostics_write_helper::set_count(int count)
{
    this->count = count;
}

bool
Diagnostics_write_helper::write_locally()
{
    return commxx.get_rank() == writer_rank;
}

int
Diagnostics_write_helper::get_writer_rank()
{
    return writer_rank;
}

hid_t &
Diagnostics_write_helper::get_hdf5_file()
{
    if (!serial) {
        open_file();
    }
    return file;
}

void
Diagnostics_write_helper::finish_write()
{
    if (!serial) {
        H5Fclose(file);
        have_file = false;
    }
    ++count;
}

Diagnostics_write_helper::~Diagnostics_write_helper()
{
    if (serial) {
        H5Fclose(file);
        have_file = false;
    }
}
