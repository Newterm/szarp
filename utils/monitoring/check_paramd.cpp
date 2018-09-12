#include <iostream>
#include <string>
#include <fstream>
#include "config_info.h"
#include "httpcl.h"

constexpr char localHost[] = "127.0.0.1";
constexpr char pref[] = "xmlns=\"http://www.praterm.com.pl/ISL/params\"><attribute name=\"value\">";
constexpr char filePath[] = "/opt/szarp/resources/szarp_in.cfg";

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

auto getLocalPort() {
	std::string port;

	std::ifstream file;
	file.open(filePath);
	
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
auto getParamFromHTML(const std::string htmlSource) {
	std::string spref(pref);
	int i = htmlSource.find(spref) + spref.size();
	std::string val = htmlSource.substr(i);
	val = val.substr(0, val.find('<'));
	return val;
}
auto checkParam(std::string param) {
	for (unsigned int i = 0; i < param.size(); i++) {
		if ( param[i] == ':') {
			param[i] = '/';
		}
	}

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

auto readThresholds(std::string sqzdThresholds) {
	thresholds_t thresholds{-1,-1,-1};
	int p = sqzdThresholds.find(',');
	thresholds.min = stoi(sqzdThresholds.substr(0, p));

	int d = sqzdThresholds.rfind(',');
	thresholds.max = stoi(sqzdThresholds.substr(p+1, d));

	thresholds.timeout = stoi(sqzdThresholds.substr(d+1));
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
	boost::optional<std::string> sqzdThresholds;
	bool hasNoThresholds = args_mgr.has("nothresholds"); 
	if ((sqzdThresholds = args_mgr.get<std::string>("thresholds")) == boost::none && !hasNoThresholds) {
		throw std::runtime_error("Set thresholds");
	} else if (!hasNoThresholds) {
		thresholds = readThresholds(*sqzdThresholds);
	}
	std::string svalue = checkParam(*param);
	
	int exitCode(0);
	if (svalue == "unknown") {
		exitCode = 2;
	} else {
		double value = stod(svalue);
		if ((value >= thresholds.min && value <= thresholds.max) || hasNoThresholds) {
			exitCode = 0;
		} else {
			exitCode = 2;
		}
	}
	std::cout << "value = " << svalue << std::endl;
	return exitCode;
}
