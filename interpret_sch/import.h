#pragma once

#include <parse_spice/netlist.h>
#include <sch/Netlist.h>

namespace sch {

string import_name(string name);
double import_value(string str, tokenizer *tokens);
bool import_device(const parse_spice::device &syntax, Subckt &ckt, const Tech &tech, tokenizer *tokens);
Subckt import_subckt(const parse_spice::subckt &syntax, const Tech &tech, tokenizer *tokens);
void import_netlist(const parse_spice::netlist &syntax, Netlist &lib, tokenizer *tokens);

}
