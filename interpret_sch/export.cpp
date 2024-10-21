#include "export.h"
#include <common/text.h>

namespace sch {

string export_name(string name) {
	// Do name mangling
	static map<char, char> replace = {
		{'_', '_'},
		{'.', '0'},
		{'[', '1'},
		{']', '2'},
		{'\'', '3'},
		{'(', '4'},
		{')', '5'},
		{'<', '6'},
		{'>', '7'},
	};

	string result;
	for (auto c = name.begin(); c != name.end(); c++) {
		auto pos = replace.find(*c);
		if (pos != replace.end()) {
			result.push_back('_');
			result.push_back(pos->second);
		} else {
			result.push_back(*c);
		}
	}
	return result;
}

string export_name(const Subckt &ckt, int net) {
	return export_name(ckt.nets[net].name);
}

parse_spice::device export_instance(const Netlist &lib, const Subckt &ckt, const Instance &inst, int index) {
	parse_spice::device result;
	result.valid = true;

	result.name = "x" + to_string(index);
	for (int i = 0; i < (int)inst.ports.size(); i++) {
		result.ports.push_back(export_name(ckt, inst.ports[i]));
	}
	result.type = lib.subckts[inst.subckt].name;

	return result;
}

parse_spice::device export_device(const Tech &tech, const Subckt &ckt, const Mos &mos, int index) {
	parse_spice::device result;
	result.valid = true;

	// TODO(edward.bingham) some technologies use raw transistor models "m" and some use subckts "x". For now, assume sky130 and use subckts
	result.name = "x" + to_string(index);
	// TODO(edward.bingham) some technologies have different port orderings. For now, just use drain gate source base
	result.ports.push_back(export_name(ckt, mos.drain));
	result.ports.push_back(export_name(ckt, mos.gate));
	result.ports.push_back(export_name(ckt, mos.source));
	result.ports.push_back(export_name(ckt, mos.base));
	result.type = tech.models[mos.model].name;

	if (mos.size[1] > 0) {
		result.params.push_back(parse_spice::parameter("w", to_minstring((double)mos.size[1]*(tech.dbunit*tech.scale)) + "u"));
	}
	if (mos.size[0] > 0) {
		result.params.push_back(parse_spice::parameter("l", to_minstring((double)mos.size[0]*(tech.dbunit*tech.scale)) + "u"));
	}
	if (mos.area[0] > 0) {
		result.params.push_back(parse_spice::parameter("ad", to_minstring((double)mos.area[0]*(tech.dbunit*tech.scale*tech.dbunit*tech.scale*1e-6)) + "u"));
	}
	if (mos.area[1] > 0) {
		result.params.push_back(parse_spice::parameter("as", to_minstring((double)mos.area[1]*(tech.dbunit*tech.scale*tech.dbunit*tech.scale*1e-6)) + "u"));
	}
	if (mos.perim[0] > 0) {
		result.params.push_back(parse_spice::parameter("pd", to_minstring((double)mos.perim[0]*(tech.dbunit*tech.scale)) + "u"));
	}
	if (mos.perim[1] > 0) {
		result.params.push_back(parse_spice::parameter("ps", to_minstring((double)mos.perim[1]*(tech.dbunit*tech.scale)) + "u"));
	}

	for (auto param = mos.params.begin(); param != mos.params.end(); param++) {
		result.params.push_back(parse_spice::parameter(param->first, to_minstring(param->second[0])));
	}

	return result;
}

parse_spice::subckt export_subckt(const Netlist &lib, const Subckt &ckt) {
	parse_spice::subckt result;
	result.valid = true;

	result.name = ckt.name;

	for (int i = 0; i < (int)ckt.inst.size(); i++) {
		result.devices.push_back(export_instance(lib, ckt, ckt.inst[i], i));
	}

	for (int i = 0; i < (int)ckt.mos.size(); i++) {
		result.devices.push_back(export_device(*lib.tech, ckt, ckt.mos[i], i));
	}

	// TODO(edward.bingham) maybe we want
	// to sort these for a standard output
	// pattern? We could also prioritize
	// Vdd, GND, Reset, _Reset, pReset,
	// _pReset, clk, _clk
	for (int i = 0; i < (int)ckt.nets.size(); i++) {
		if (ckt.nets[i].isIO) {
			result.ports.push_back(export_name(ckt.nets[i].name));
		}
	}

	return result;
}

parse_spice::netlist export_netlist(const Netlist &lib) {
	parse_spice::netlist result;
	result.valid = true;

	for (int i = 0; i < (int)lib.subckts.size(); i++) {
		result.subckts.push_back(export_subckt(lib, lib.subckts[i]));
		if (result.subckts.back().name.empty()) {
			result.subckts.back().name = "process_" + to_string(i);
		}
	}

	return result;
}

}
