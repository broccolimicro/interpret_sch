#include "import.h"
#include <limits>

using namespace std;

namespace sch {

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

bool import_device(const parse_spice::device &syntax, Subckt &ckt, tokenizer *tokens) {
	if (not syntax.valid) {
		return false;
	}

	string devType = string(1, (char)tolower(syntax.name[0]));
	string devName = syntax.name.substr(1);

	// DESIGN(edward.bingham) Since we're focused on digital design, we'll only support transistor layout for now.
	if (string("mx").find(devType) == string::npos) {
		printf("not transistor or subckt %s%s\n", devType.c_str(), devName.c_str());
		return false;
	}

	// MOS must have the following ports
	// drain gate source base
	if (syntax.ports.size() != 4) {
		printf("transistors only have 4 ports\n");
		return false;
	}

	int modelIdx = ckt.tech.findModel(syntax.type);
	// if the modelName isn't in the model list, then this is a non-transistor subckt
	if (modelIdx < 0) {
		printf("model not found %s\n", syntax.type.c_str());
		return false;
	}

	int type = ckt.tech.models[modelIdx].type;
	ckt.mos.push_back(Mos(modelIdx, type));
	for (int i = 0; i < (int)syntax.ports.size(); i++) {
		int net = ckt.findNet(syntax.ports[i], true);
		if (i == 1) {
			ckt.nets[net].gates[type]++;
			ckt.mos.back().gate = net;
		} else if (i == 3) {
			ckt.mos.back().bulk = net;
		} else {
			ckt.nets[net].ports[type]++;
			ckt.mos.back().ports.push_back(net);
		}
	}

	for (auto param = syntax.params.begin(); param != syntax.params.end(); param++) {
		vector<double> values(1, import_value(param->value, tokens));
		if (param->name == "w") {
			ckt.mos.back().size[1] = int(values[0]/ckt.tech.dbunit);
		} else if (param->name == "l") {
			ckt.mos.back().size[0] = int(values[0]/ckt.tech.dbunit);
		} else {
			ckt.mos.back().params.insert(pair<string, vector<double> >(param->name, values));
		}
	}

	return true;
}

void import_subckt(const parse_spice::subckt &syntax, Subckt &ckt, tokenizer *tokens) {
	ckt.name = syntax.name;
	for (int i = 0; i < (int)syntax.ports.size(); i++) {
		ckt.nets.push_back(Net(syntax.ports[i], true));
	}

	for (int i = 0; i < (int)syntax.devices.size(); i++) {
		if (not import_device(syntax.devices[i], ckt, tokens)) {
			printf("unrecognized device\n");
		}
	}
}

// load a spice AST into the layout engine
void import_netlist(const parse_spice::netlist &syntax, Netlist &lib, tokenizer *tokens) {
	for (int i = 0; i < (int)syntax.subckts.size(); i++) {
		lib.subckts.push_back(Subckt(*lib.tech));
		import_subckt(syntax.subckts[i], lib.subckts.back(), tokens);
	}
}

}
