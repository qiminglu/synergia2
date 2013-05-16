#ifndef CHEF_ELEMENTS_H_
#define CHEF_ELEMENTS_H_

#if __GNUC__ > 4 && __GNUC_MINOR__ > 5
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wsequence-point"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <beamline/beamline.h>
#if __GNUC__ > 4 && __GNUC_MINOR__ > 5
#pragma GCC diagnostic pop
#endif

typedef std::list<ElmPtr > Chef_elements; // syndoc:include

#endif /* CHEF_ELEMENTS_H_ */
