#include "madx_reader.h"
#include "mx_parse.h"
#include "synergia/utils/digits.h"
#include "synergia/foundation/physical_constants.h"
#include "synergia/foundation/four_momentum.h"
#include "synergia/foundation/reference_particle.h"

#include <stdexcept>
#include <algorithm>

using namespace synergia;

void
MadX_reader::extract_reference_particle(Lattice & lattice)
{
    const int not_found = -1;
    int beam_index = not_found;
    std::vector < std::string > command_names(madx_sptr->commands());
    int current_index = 0;
    for (std::vector<std::string >::const_iterator it = command_names.begin();
            it != command_names.end(); ++it) {
        if (*it == "beam") {
            beam_index = current_index;
        }
        ++current_index;
    }
    if (beam_index > not_found) {
        synergia::MadX_command command(madx_sptr->command(beam_index));
        std::vector < std::string > attributes(command.attribute_names());
        double mass = 0, charge = 0, energy = 0, pc = 0, gamma = 0;
        for (std::vector<std::string >::const_iterator it = attributes.begin();
                it != attributes.end(); ++it) {
            if (*it == "particle") {
                std::string particle(command.attribute_as_string(*it));
                if (particle == "proton") {
                    mass = pconstants::mp;
                    charge = pconstants::proton_charge;
                } else if (particle == "antiproton") {
                    mass = pconstants::mp;
                    charge = pconstants::antiproton_charge;
                } else if (particle == "electron") {
                    mass = pconstants::me;
                    charge = pconstants::electron_charge;
                } else if (particle == "positron") {
                    mass = pconstants::me;
                    charge = pconstants::positron_charge;
                } else if (particle == "negmuon") {
                    mass = pconstants::mmu;
                    charge = pconstants::muon_charge;
                } else if (particle == "posmuon") {
                    mass = pconstants::mmu;
                    charge = pconstants::antimuon_charge;
                }
            } else if (*it == "mass") {
                mass = command.attribute_as_number(*it);
            } else if (*it == "charge") {
                charge = command.attribute_as_number(*it);
            } else if (*it == "energy") {
                energy = command.attribute_as_number(*it);
            } else if (*it == "pc") {
                pc = command.attribute_as_number(*it);
            } else if (*it == "gamma") {
                gamma = command.attribute_as_number(*it);
            }
        }
        Four_momentum four_momentum(mass);
        if (energy > 0) {
            four_momentum.set_total_energy(energy);
        }
        if (pc > 0) {
            four_momentum.set_momentum(pc);
        }
        if (gamma > 0) {
            four_momentum.set_gamma(gamma);
        }
        Reference_particle reference_particle(charge, four_momentum);
        lattice.set_reference_particle(reference_particle);
    }
}

MadX_reader::MadX_reader()
{
}

void
MadX_reader::parse(std::string const& string)
{
    madx_sptr = boost::shared_ptr<MadX >(new MadX());
    synergia::parse_madx(string, *madx_sptr);
}

void
MadX_reader::parse_file(std::string const& filename)
{
    madx_sptr = boost::shared_ptr<MadX >(new MadX());
    synergia::parse_madx_file(filename, *madx_sptr);
}

std::vector<std::string >
MadX_reader::get_line_names()
{
    if (!madx_sptr) {
        throw std::runtime_error(
                "MadX_reader::get_line_names: nothing has been parsed");
    }
    std::vector < std::string > retval(madx_sptr->line_labels());
    std::vector < std::string > sequence_labels(madx_sptr->sequence_labels());
    for (std::vector<std::string >::const_iterator it = sequence_labels.begin();
            it != sequence_labels.end(); ++it) {
        retval.push_back(*it);
    }
    return retval;
}

Lattice
MadX_reader::get_lattice(std::string const& line_name)
{
    if (!madx_sptr) {
        throw std::runtime_error(
                "MadX_reader::get_lattice: nothing has been parsed");
    }

    Lattice lattice(line_name);

    bool found_line(false), found_sequence(false);

    std::vector < std::string > lines(madx_sptr->line_labels());
    std::vector<std::string >::const_iterator it = std::find(lines.begin(),
            lines.end(), line_name);
    if (it != lines.end()) {
        found_line = true;
    } else {
        std::vector < std::string > sequences(madx_sptr->sequence_labels());
        std::vector<std::string >::const_iterator it = std::find(
                sequences.begin(), sequences.end(), line_name);
        if (it != lines.end()) {
            found_sequence = true;
        } else {
            throw std::runtime_error(
                    "cannot find line with label " + line_name);
        }
    }
    if (found_line) {
        // to be completed
        throw std::runtime_error(
                "MadX_reader::get_lattice does not currently handle lines");
    }
    if (found_sequence) {
        MadX_sequence sequence(madx_sptr->sequence(line_name));
        double total_length = sequence.length();
        double current_pos = 0.0;
        const double min_drift_length = 1.0e-9;
        int drift_count = 0;
        int drift_digits = digits(sequence.element_count());
        for (int i = 0; i < sequence.element_count(); ++i) {
            double at = sequence.element(i, false).attribute_as_number("at");
            double drift_length = at - current_pos;
            if (drift_length > min_drift_length) {
                std::stringstream name_stream;
                name_stream << "auto_drift";
                name_stream << "_";
                name_stream << std::setw(drift_digits);
                name_stream << std::setfill('0');
                name_stream << drift_count;
                Lattice_element drift("drift", name_stream.str());
                drift.set_double_attribute("l", drift_length, false);
                lattice.append(drift);
                ++drift_count;
            }
            std::string name(sequence.element(i, false).label());
            if (name == "") {
                name = sequence.element(i, true).label();
            }
            std::string type(sequence.element(i, true).name());
            Lattice_element element(type, name);
            std::vector<string_t > attribute_names(
                    sequence.element(i, true).attribute_names());
            for (std::vector<string_t >::iterator it = attribute_names.begin();
                    it != attribute_names.end(); ++it) {
                element.set_double_attribute(*it,
                        sequence.element(i, true).attribute_as_number(*it));
            }
            lattice.append(element);
            current_pos = at + element.get_length();
        }
    }
    extract_reference_particle(lattice);
    return lattice;
}

Lattice
MadX_reader::get_lattice(std::string const& line_name,
        std::string const& filename)
{
    parse_file(filename);
    return get_lattice(line_name);
}

MadX_reader::~MadX_reader()
{

}
