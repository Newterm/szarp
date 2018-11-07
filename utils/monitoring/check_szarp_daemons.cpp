#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "argsmgr.h"
#include "szarp_config.h"
#include "config_info.h"
#include "conversion.h"

constexpr int RED = 31;
constexpr int GREEN = 32;

class CheckDaemonArgs: public DefaultArgs {
public:
	po::options_description get_options() const override {
		po::options_description desc{"Options"};
		desc.add_options()
			("help,h", "Print this help messages")
			("version,V", "Output version")
			("all,a", "Print all process")
			("base,b", po::value<std::string>()->required(), "Base to read configuration from")
			("override,D", po::value<std::vector<std::string>>()->multitoken(), "Override default arguments (-Dparam=val)");
		return desc;
	}
};

struct DaemonInfo {
	DaemonInfo(std::string daemon_name, int nr, std::string path, bool isRunning): isRunning(isRunning), daemon_name(daemon_name), path(path), nr(nr) {}
	bool isRunning;
	std::string daemon_name;
	std::string path;
	int nr;
};

bool isDeviceRunning(std::string daemon, int nr, std::string path) {
	std::string command = "ps ax | grep '" + daemon + " " + std::to_string(nr) + " " + path + "'";
	FILE* pipe = popen(command.c_str(), "r");
	if (!pipe) {
		throw std::runtime_error("cannot run cmdline!");
	}
	int count_lines = 0;
	std::array<char, 512> buffer;
	while (fgets(buffer.data(), buffer.size(), pipe) != NULL ) {
		count_lines++;
	}
	pclose(pipe);

	/* There should be three process after filter:
		- sh ( using '' in grep statement )
		- grep	( current command )
		- current daemon
	*/
	if (count_lines < 3)
		return false;
	else
		return true;
}

int main(int argc, char* argv[]){
	ArgsManager args_mgr("check_szarp_daemon");
	args_mgr.parse(argc, argv, CheckDaemonArgs());
	args_mgr.initLibpar();

	boost::optional<std::string> base;
	if (( base = args_mgr.get<std::string>("base") ) == boost::none) {
		throw std::runtime_error("base is required!");
	}
	bool print_all = false;
	if (args_mgr.has("all")) {
		print_all = true;
	}

std::wstring base_name = SC::L2S(*base);

	std::wstring base_path = L"/opt/szarp/";
	base_path.append(base_name).append(L"/config/params.xml");

	TSzarpConfig sc;
	sc.loadXML(base_path, base_name);

	int nr_current_daemon = 0;
	int n_running_daemons = 0;
	int n_fake_daemons = 0;

	std::vector<struct DaemonInfo> daemons_info;

	for (auto device = sc.GetFirstDevice(); device != nullptr; device = device->GetNext()){
		nr_current_daemon++;

		std::string path = device->getAttribute<std::string>("path");
		std::string daemon = device->getAttribute<std::string>("daemon");

		if (daemon == "/bin/true" || daemon == "/bin/false") {
			n_fake_daemons++;
			continue;
		}

		if (isDeviceRunning(daemon, nr_current_daemon, path)) {
			n_running_daemons++;
			if (print_all) {
				daemons_info.push_back(DaemonInfo(daemon, nr_current_daemon, path, true));
			}
		} else {
			daemons_info.push_back(DaemonInfo(daemon, nr_current_daemon, path, false));
		}
	}

	const int n_real_daemons = nr_current_daemon - n_fake_daemons;
	if (n_running_daemons == n_real_daemons) {
		printf("OK: %d out of %d running\n",n_running_daemons, n_real_daemons);
	} else {
		printf("ERROR: %d out of %d running\n",n_running_daemons, n_real_daemons);
	}

	/* print daemons info */
	for (unsigned int i = 0; i < daemons_info.size(); i++) {
		DaemonInfo device = daemons_info.at(i);

		printf("device %d - %s %s", device.nr, device.daemon_name.c_str(), boost::filesystem::path(device.path).filename().c_str());
		printf("%c[%dm is %srunning\n", 0x1B, device.isRunning ? GREEN : RED, device.isRunning ? "" : "not ");
		printf("%c[%dm", 0x1B, 0);
	}
}
