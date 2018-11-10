#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include "config_info.h"
#include "httpcl.h"

constexpr char localHost[] = "127.0.0.1";
constexpr char szarpInPath[] = "/opt/szarp/resources/szarp_in.cfg";
constexpr int errorStatus = 2;

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

std::string getLocalPort() {
	std::string port{"8081"};

	std::ifstream file;
	file.open(szarpInPath);
	
	bool correctSegment = false;
	for (std::string line; getline(file, line);) {
		if (!correctSegment && line.find(":local_paramd") != std::string::npos) {
			correctSegment = true;
		} else if (correctSegment && line.find("port") != std::string::npos) {
				int i = line.find('=');
				port = line.substr(++i);
				break;
		}
	}
	file.close();
	return port;
}
std::string getParamFromHTML(const std::string htmlSource) {
	constexpr char prefValue[] = "xmlns=\"http://www.praterm.com.pl/ISL/params\"><attribute name=\"value\">";
	std::string spref(prefValue);
	int i = htmlSource.find(spref) + spref.size();
	std::string val = htmlSource.substr(i);
	val = val.substr(0, val.find('<'));
	return val;
}

std::string checkParam(std::string param) {
	std::replace(param.begin(), param.end(), ':', '/');

	std::string url = std::string("http://").append(localHost).append(std::string(":")).append(getLocalPort()).append("/").append(param).append("@value");
	
	szHTTPCurlClient szHTTPCC;
	char* uri = (char*)url.c_str();
	size_t size;
	char* htmlSource = szHTTPCC.Get(uri, &size, NULL, -1);
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
	int p = unitedThresholds.find(',');
	thresholds.min = stoi(unitedThresholds.substr(0, p));

	int d = unitedThresholds.rfind(',');
	thresholds.max = stoi(unitedThresholds.substr(p+1, d));

	thresholds.timeout = stoi(unitedThresholds.substr(d+1));
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
		exitCode = errorStatus;
	} else {
		double value = boost::lexical_cast<double>(svalue);
		if (value < thresholds.min || value > thresholds.max || hasNoThresholds) {
			exitCode = errorStatus;
		}
	}
	std::cout << "value = " << svalue << std::endl;
	return exitCode;
}
