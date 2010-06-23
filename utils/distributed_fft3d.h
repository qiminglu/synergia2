#ifndef DISTRIBUTED_FFT3D_H_
#define DISTRIBUTED_FFT3D_H_

#ifdef USE_FFTW2
#include <rfftw_mpi.h>
#else
#include <fftw3.h>
#include <fftw3-mpi.h>
#endif //USE_FFTW2
#include <vector>
#include <string>
#include "boost/shared_ptr.hpp"
#include "utils/multi_array_typedefs.h"
#include "utils/commxx.h"

class Distributed_fft3d
{
private:
#ifdef USE_FFTW2
    rfftwnd_mpi_plan plan, inv_plan;
    fftw_real *data;
    fftw_real *workspace;
#else
    fftw_plan plan, inv_plan;
    double *data;
    fftw_complex *workspace;
#endif //USE_FFTW2
    int lower, upper, local_size;
    std::vector<int > shape;
    bool have_local_data;
public:
    Distributed_fft3d(std::vector<int > const& shape, Commxx const& comm,
            int planner_flags = FFTW_ESTIMATE,
            std::string const& wisdom_filename = "");
    int
    get_lower() const;
    int
    get_upper() const;
    std::vector<int >
    get_padded_shape_real() const;
    std::vector<int >
    get_padded_shape_complex() const;
    void
    transform(MArray3d_ref & in, MArray3dc_ref & out);
    void
    inv_transform(MArray3dc_ref & in, MArray3d_ref & out);
    ~Distributed_fft3d();
};

typedef boost::shared_ptr<Distributed_fft3d > Distributed_fft3d_sptr;
#endif /* DISTRIBUTED_FFT3D_H_ */
