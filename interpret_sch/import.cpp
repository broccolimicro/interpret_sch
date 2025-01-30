#include "import.h"
#include <limits>

using namespace std;

namespace sch {

string import_name(string name) {
	// Do name mangling
	static map<char, char> replace = {
		{'_', '_'},
		{'0', '.'},
		{'1', '['},
		{'2', ']'},
		{'3', '\''},
		{'4', '('},
		{'5', ')'},
		{'6', '<'},
		{'7', '>'},
	};

	string result;
	bool escaped = false;
	for (auto c = name.begin(); c != name.end(); c++) {
		if (escaped) {
			auto pos = replace.find(*c);
			if (pos != replace.end()) {
				result += pos->second;
			} else {
				result.push_back(*c);
			}
			escaped = false;
		} else if (*c == '_') {
			escaped = true;
		} else {
			result.push_back(*c);
		}
	}
	return result;
}


double import_value(string str, tokenizer *tokens) {
	if (str.empty()) {
		return 0.0;
	}

	double value = stod(str);
	int unit = (int)(string("afpnumkxg").find((char)tolower(str.back())));
	if (unit >= 0) {
		int exp = 3*unit - 18;
		value *= pow(10.0, (double)exp);
	}

	return value;
}

bool import_device(const parse_spice::device &syntax, Subckt &ckt, const Tech &tech, tokenizer *tokens) {
	if (not syntax.valid) {
		return false;
	}

	string devType = string(1, (char)tolower(syntax.name[0]));
	string devName = syntax.name.substr(1);

	// DESIGN(edward.bingham) Since we're focused on digital design, we'll only support transistor layout for now.
	if (string("mx").find(devType) == string::npos) {
		return false;
	}

	int modelIdx = tech.findModel(syntax.type);
	// if the modelName isn't in the model list, then this is a non-transistor subckt
	if (modelIdx < 0) {
		return false;
	}

	// MOS must have the following ports
	// drain gate source base
	if (syntax.ports.size() != 4) {
		printf("transistors only have 4 ports\n");
		return false;
	}

	int type = tech.models[modelIdx].type;
	int drain = ckt.findNet(import_name(syntax.ports[0]), true);
	int gate = ckt.findNet(import_name(syntax.ports[1]), true);
	int source = ckt.findNet(import_name(syntax.ports[2]), true);
	int base = ckt.findNet(import_name(syntax.ports[3]), true);

	vec2i size(0,0);
	vec2i area(0,0), perim(0,0);

	double coeff = tech.dbunit*tech.scale*1e-6;
	double coeff2 = tech.dbunit*tech.dbunit*tech.scale*1e-6;

	ckt.push(Mos(modelIdx, type, drain, gate, source, base));
	for (auto param = syntax.params.begin(); param != syntax.params.end(); param++) {
		vector<double> values(1, import_value(param->value, tokens));
		if (param->name == "w") {
			size[1] = int(values[0]/coeff + 0.5);
		} else if (param->name == "l") {
			size[0] = int(values[0]/coeff + 0.5);
		} else if (param->name == "as") {
			area[1] = int(values[0]/coeff2 + 0.5);
		} else if (param->name == "ad") {
			area[0] = int(values[0]/coeff2 + 0.5);
		} else if (param->name == "ps") {
			perim[1] = int(values[0]/coeff + 0.5);
		} else if (param->name == "pd") {
			perim[0] = int(values[0]/coeff + 0.5);
		} else {
			ckt.mos.back().params.insert(pair<string, vector<double> >(param->name, values));
		}
	}

	ckt.mos.back().setSize(tech, size);
	if (area[0] > 0) {
		ckt.mos.back().area[0] = area[0];
	}
	if (area[1] > 0) {
		ckt.mos.back().area[1] = area[1];
	}
	if (perim[0] > 0) {
		ckt.mos.back().perim[0] = perim[0];
	}
	if (perim[1] > 0) {
		ckt.mos.back().perim[1] = perim[1];
	}

	return true;
}

bool import_instance(const Netlist &lst, const parse_spice::device &syntax, Subckt &ckt, const Tech &tech, tokenizer *tokens) {
	if (not syntax.valid) {
		return false;
	}

	string instType = string(1, (char)tolower(syntax.name[0]));
	string instName = syntax.name.substr(1);

	// DESIGN(edward.bingham) Since we're focused on digital design, we'll only support transistor layout for now.
	if (string("x").find(instType) == string::npos) {
		return false;
	}

	int modelIdx = -1;
	for (int i = 0; i < (int)lst.subckts.size(); i++) {
		if (lst.subckts[i].name == syntax.type) {
			modelIdx = i;
			break;
		}
	}
	// if the subckt isn't in the netlist, then we can't instantiate it.
	if (modelIdx < 0) {
		return false;
	}

	Instance inst(modelIdx);
	inst.name = instName;
	for (int i = 0; i < (int)syntax.ports.size(); i++) {
		int port = ckt.findNet(import_name(syntax.ports[i]), true);
		inst.ports.push_back(port);
	}

	ckt.push(inst);
	return true;
}

void import_subckt(Subckt &ckt, const parse_spice::subckt &syntax, const Tech &tech, tokenizer *tokens, const Netlist *lst) {
	ckt.name = syntax.name;
	for (int i = 0; i < (int)syntax.ports.size(); i++) {
		ckt.push(Net(import_name(syntax.ports[i]), true));
	}

	for (int i = 0; i < (int)syntax.devices.size(); i++) {
		if (import_device(syntax.devices[i], ckt, tech, tokens)) {
			continue;
		}

		if (lst != nullptr and import_instance(*lst, syntax.devices[i], ckt, tech, tokens)) {
			continue;
		}

		printf("unrecognized device/subckt\n");
		printf("%s\n", syntax.devices[i].to_string().c_str());
	}
}

// load a spice AST into the layout engine
void import_netlist(const parse_spice::netlist &syntax, Netlist &lst, tokenizer *tokens) {
	int start = (int)lst.subckts.size();
	lst.subckts.resize(lst.subckts.size()+syntax.subckts.size());
	// preload subckt names so that we can look them up out of order
	for (int i = 0; i < (int)syntax.subckts.size(); i++) {
		lst.subckts[start+i].name = syntax.subckts[i].name;
	}

	for (int i = 0; i < (int)syntax.subckts.size(); i++) {
		import_subckt(lst.subckts[start+i], syntax.subckts[i], *lst.tech, tokens, &lst);
	}
}

}
