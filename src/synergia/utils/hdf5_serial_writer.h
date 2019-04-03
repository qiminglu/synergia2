#ifndef HDF5_SERIAL_WRITER_H_
#define HDF5_SERIAL_WRITER_H_
#include <vector>
#include <string>

#include "hdf5.h"

#include "synergia/utils/hdf5_file.h"
#include "synergia/utils/cereal.h"

template<typename T>
    class Hdf5_serial_writer
    {
    private:
        std::vector<hsize_t > dims, max_dims, size, offset;
        int data_rank;
        std::string name;

        Hdf5_file_sptr file_sptr;
        Hdf5_handler dataset;
        Hdf5_handler atomic_type;

        bool have_setup;
        bool resume;
        size_t data_size;
        void
        setup(std::vector<int > const& data_dims);
    public:
        Hdf5_serial_writer(Hdf5_file_sptr file_sptr, std::string const& name,
                bool resume = false);
        // Default constructor for serialization use only
        Hdf5_serial_writer();
        void
        append(T & data);
        template<class Archive>
            void
            save(Archive & ar, const unsigned int version) const
            {
                ar & CEREAL_NVP(name);
                ar & CEREAL_NVP(offset);
                ar & CEREAL_NVP(file_sptr);
                ar & CEREAL_NVP(data_rank);
                ar & CEREAL_NVP(data_size);
            }
        template<class Archive>
            void
            load(Archive & ar, const unsigned int version)
            {
                ar & CEREAL_NVP(name);
                ar & CEREAL_NVP(offset);
                ar & CEREAL_NVP(file_sptr);
                ar & CEREAL_NVP(data_rank);
                ar & CEREAL_NVP(data_size);
                resume = true;
                have_setup = false;
            }
        ~Hdf5_serial_writer();
    };

#include "synergia/utils/hdf5_serial_writer.tcc"
#endif /* HDF5_SERIAL_WRITER_H_ */
