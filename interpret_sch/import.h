#pragma once

#include <parse_spice/netlist.h>
#include <sch/Netlist.h>

namespace sch {

string import_name(string name);
double import_value(string str, tokenizer *tokens);
bool import_device(const parse_spice::device &syntax, Subckt &ckt, const Tech &tech, tokenizer *tokens);
bool import_instance(const Netlist &lst, const parse_spice::device &syntax, Subckt &ckt, const Tech &tech, tokenizer *tokens);
void import_subckt(Subckt &ckt, const parse_spice::subckt &syntax, const Tech &tech, tokenizer *tokens, const Netlist *lst);
void import_netlist(const parse_spice::netlist &syntax, Netlist &lst, tokenizer *tokens);

}
