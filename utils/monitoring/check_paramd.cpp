#include <iostream>
#include <string>
#include <regex>
#include "config_info.h"
#include "httpcl.h"

constexpr int errorExitCode = 2;

class CheckDaemonArgs: public DefaultArgs {
public:
	po::options_description get_options() const override {
		po::options_description desc{"This script pools local paramd server for a given parameter info.\nOptions"};
		desc.add_options()
		("help,h", "Print this help messages")
		("version,V", "Output version")
		("nothresholds,n", "Run without thresholds")
		("thresholds,t", po::value<std::string>(), "THRESHOLDS, default 0,0,0")
		("param,p", po::value<std::string>(), "Full parameter name")
		("override,D", po::value<std::vector<std::string>>()->multitoken(), "Override default arguments (-Dparam=val)");
		return desc;
	}
};
void printErrorAndClose(const std::string message) {
	std::cout << "Error: " << message << std::endl;
	exit(errorExitCode);
}

std::string getParamFromHTML(const std::string htmlSource) {
	const std::string prefValue = "<attribute name=\"value\">";
	
	std::string regc(prefValue);
	regc.append(".*</attribute>");
	std::regex reg(regc.c_str());
	std::smatch ret;
	if (!regex_search(htmlSource, ret, reg)) {
		printErrorAndClose("unable get value from html");
	}

	int startPoint = htmlSource.find(prefValue) + prefValue.size();
	std::string val = htmlSource.substr(startPoint);
	val = val.substr(0, val.find('<'));
	return val;
}

std::string checkParam(std::string param) {
	std::replace(param.begin(), param.end(), ':', '/');

	std::string localPort = libpar_getpar("local_paramd", "port", 1);
	std::string localHost = libpar_getpar("local_paramd", "allowed_ip", 1);
	std::string url = std::string("http://").append(localHost).append(std::string(":")).append(localPort).append("/").append(param).append("@value");
	szHTTPCurlClient szHTTPCC;
	char* htmlSource;
	if ((htmlSource = szHTTPCC.Get(url.c_str(), NULL) ) == NULL) {
		printErrorAndClose("unable to get html source - check if paramd is running");
	}
	std::string val = getParamFromHTML(std::string(htmlSource));
	return val;
}

struct thresholds_t {
	int min;
	int max;
	int timeout;
};

struct thresholds_t readThresholds(const std::string& unitedThresholds) {
	thresholds_t thresholds{-1,-1,-1};
	std::regex reg("\\d*,\\d*,\\d*");
	std::smatch ret;
	if (!regex_search(unitedThresholds, ret, reg)) {
		throw std::runtime_error("incorrect thresholds");
	}
	int firstComma = unitedThresholds.find(',');
	thresholds.min = stoi(unitedThresholds.substr(0,firstComma));

	int secondComma = unitedThresholds.rfind(',');
	thresholds.max = stoi(unitedThresholds.substr(firstComma + 1, secondComma));

	thresholds.timeout = stoi(unitedThresholds.substr(secondComma + 1));
	return thresholds;
}

int main(int argc, char* argv[]) {
	ArgsManager args_mgr("check_paramd");
	args_mgr.parse(argc, argv, CheckDaemonArgs());
	args_mgr.initLibpar();

	boost::optional<std::string> param;
	if ((param = args_mgr.get<std::string>("param")) == boost::none) {
		throw std::runtime_error("param is required!");
	}

	thresholds_t thresholds = {-1, -1, -1};
	boost::optional<std::string> unitedThresholds;
	
	bool hasNoThresholds = args_mgr.has("nothresholds"); 
	if (!hasNoThresholds) {
		unitedThresholds = args_mgr.get<std::string>("thresholds");
		if (!unitedThresholds){ 
			throw std::runtime_error("Set thresholds");
		} else {
			thresholds = readThresholds(*unitedThresholds);
		}
	}
	const std::string svalue = checkParam(*param);
	
	int exitCode{0};
	if (svalue == "unknown") {
		exitCode = errorExitCode;
	} else {
		double value = boost::lexical_cast<double>(svalue);
		if (value < thresholds.min || value > thresholds.max || hasNoThresholds) {
			exitCode = errorExitCode;
		}
	}
	std::cout << "value = " << svalue << std::endl;
	return exitCode;
}
