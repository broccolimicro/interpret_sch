#include "export.h"
#include <common/text.h>

namespace sch {

string export_name(string name) {
	// Do name mangling
	static map<char, string> replace = {
		{'_', "__"},
		{'.', "_0"},
		{'[', "_1"},
		{']', "_2"},
		{'(', "_3"},
		{')', "_4"},
		{'<', "_5"},
		{'>', "_6"},
	};

	string result;
	for (auto c = name.begin(); c != name.end(); c++) {
		auto pos = replace.find(*c);
		if (pos != replace.end()) {
			result += pos->second;
		} else {
			result.push_back(*c);
		}
	}
	return result;
}

string export_name(const Tech &tech, const Subckt &ckt, int net) {
	return export_name(ckt.nets[net].name);
}

parse_spice::device export_device(const Tech &tech, const Subckt &ckt, const Mos &mos, int index) {
	parse_spice::device result;
	result.valid = true;

	// TODO(edward.bingham) some technologies use raw transistor models "m" and some use subckts "x". For now, assume sky130 and use subckts
	result.name = "x" + to_string(index);
	// TODO(edward.bingham) some technologies have different port orderings. For now, just use drain gate source base
	result.ports.push_back(export_name(tech, ckt, mos.ports[1]));
	result.ports.push_back(export_name(tech, ckt, mos.gate));
	result.ports.push_back(export_name(tech, ckt, mos.ports[0]));
	result.ports.push_back(export_name(tech, ckt, mos.bulk));
	result.type = tech.models[mos.model].name;

	result.params.push_back(parse_spice::parameter("w", to_minstring((double)mos.size[1]*tech.dbunit) + "u"));
	result.params.push_back(parse_spice::parameter("l", to_minstring((double)mos.size[0]*tech.dbunit) + "u"));

	return result;
}

parse_spice::subckt export_subckt(const Tech &tech, const Subckt &ckt, int index) {
	parse_spice::subckt result;
	result.valid = true;

	result.name = ckt.name;
	if (result.name.empty()) {
		result.name = "process_" + to_string(index);
	}

	for (int i = 0; i < (int)ckt.mos.size(); i++) {
		result.devices.push_back(export_device(tech, ckt, ckt.mos[i], i));
	}

	// TODO(edward.bingham) maybe we want
	// to sort these for a standard output
	// pattern? We could also prioritize
	// Vdd, GND, Reset, _Reset, pReset,
	// _pReset, clk, _clk
	for (int i = 0; i < (int)ckt.nets.size(); i++) {
		if (ckt.nets[i].isIO) {
			result.ports.push_back(ckt.nets[i].name);
		}
	}

	return result;
}

parse_spice::netlist export_netlist(const Tech &tech, const Netlist &lib) {
	parse_spice::netlist result;
	result.valid = true;

	for (auto ckt = lib.subckts.begin(); ckt != lib.subckts.end(); ckt++) {
		result.subckts.push_back(export_subckt(tech, *ckt, ckt-lib.subckts.begin()));
	}

	return result;
}

}
