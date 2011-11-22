#include "aperture_operation_extractor.h"

Aperture_operation_extractor::Aperture_operation_extractor()
{
}

Aperture_operation_extractor::~Aperture_operation_extractor()
{
}

Aperture_operation_extractor_map::Aperture_operation_extractor_map()
{
}

void
Aperture_operation_extractor_map::set_extractor(std::string const& name,
        Aperture_operation_extractor_sptr operation_extractor)
{
    extractor_map[name] = operation_extractor;
}

Aperture_operation_extractor_sptr
Aperture_operation_extractor_map::get_extractor(std::string const& name)
{
    return extractor_map[name];
}

std::list<std::string >
Aperture_operation_extractor_map::get_extractor_names() const
{
    std::list<std::string > retval;
    for (std::map<std::string, Aperture_operation_extractor_sptr >::const_iterator
            it = extractor_map.begin(); it != extractor_map.end(); ++it) {
        retval.push_back(it->first);
    }
    return retval;
}

Aperture_operation_extractor_map::~Aperture_operation_extractor_map()
{
}
