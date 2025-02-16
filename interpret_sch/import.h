#pragma once

#include <parse_spice/netlist.h>
#include <sch/Netlist.h>

namespace sch {

string import_name(string name);
double import_value(string str, tokenizer *tokens);
bool import_device(const Tech &tech, Subckt &ckt, const parse_spice::device &syntax, tokenizer *tokens);
bool import_instance(const Tech &tech, Subckt &ckt, const parse_spice::device &syntax, tokenizer *tokens, const Netlist *lst);
void import_subckt(const Tech &tech, Subckt &ckt, const parse_spice::subckt &syntax, tokenizer *tokens, const Netlist *lst);
void import_netlist(const Tech &tech, Netlist &lst, const parse_spice::netlist &syntax, tokenizer *tokens);

}
