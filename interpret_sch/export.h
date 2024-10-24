#pragma once

#include <parse_spice/netlist.h>
#include <sch/Netlist.h>
#include <ucs/variable.h>

namespace sch {

string export_name(string name);
string export_name(const Subckt &ckt, int net);
string export_value(double value);
parse_spice::device export_instance(const Netlist &lib, const Subckt &ckt, const Instance &inst, int index);
parse_spice::device export_device(const Tech &tech, const Subckt &ckt, const Mos &mos, int index);
parse_spice::subckt export_subckt(const Netlist &lib, const Subckt &ckt);
parse_spice::netlist export_netlist(const Netlist &lib);

}
