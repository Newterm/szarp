#ifndef CMDLINEPARSER__H__
#define CMDLINEPARSER__H__

#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include "../libSzarp/include/libpar.h"

namespace basedmn {

namespace po = boost::program_options;

struct ArgsHolder {
	virtual po::options_description get_options() = 0;
	virtual void add_positional_options(po::positional_options_description&) {}
	virtual void parse(const po::parsed_options&, const po::variables_map&) {}
};

struct DefaultArgs: public ArgsHolder {
	po::options_description get_options() override {
		po::options_description desc{"Options"};
		desc.add_options()
			("help,h", "Print this help messages")
			("version,v", "Output version")
			("debug,d", po::value<unsigned int>(), "Enables verbose logging")
			("base,b", po::value<std::string>(), "Base to read configuration from")
			("override,D", po::value<std::vector<std::string>>()->multitoken(), "Override default arguments");

		return std::move(desc);
	}

	void parse(const po::parsed_options& parsed_opts, const po::variables_map& vm) override {
		if ( vm.count("help") || vm.count("version") ) { 
			std::cout << *parsed_opts.description << std::endl;
			exit(0);
		} 
	}
};

struct DaemonArgs: public ArgsHolder {
	po::options_description get_options() override {
		po::options_description desc{"General daemon arguments"};
		desc.add_options()
			("single,s", "Forbid writing via IPC")
			("use-cfgdealer", "Enables configuring via config dealer")
			("cfgdealer-address", po::value<std::string>()->default_value("localhost:5555"), "Config dealer's address")
			("device-no", po::value<unsigned int>(), "Device number in config file")
			("device-path", po::value<std::string>()->default_value("/dev/null"), "Device path attribute");

		return std::move(desc);
	}

	void add_positional_options(po::positional_options_description& p_opts) override {
		p_opts.add("device-no", 1);
		p_opts.add("device-path", 1);
	}

	void parse(const po::parsed_options&, const po::variables_map& vm) override {
		if (vm.count("device-no") == 0) throw std::runtime_error("Device number not specified! Cannot process!");
		// device-path is not necessary for most daemons
	}
};

// add log options


struct CmdLineParser {
	std::string name;
	po::variables_map vm; 

public:
	CmdLineParser(const std::string& name): name(name), vm() {}

	void parse(int argc, char ** argv, const std::vector<ArgsHolder*>& args_holders = {new DefaultArgs()});

	template <typename T>
	boost::optional<T> get(const std::string& arg) const { if (vm.count(arg) > 0) return vm[arg].as<T>(); else return boost::none; }

	bool has(const std::string& arg) const { return vm.count(arg) != 0; }

	size_t count(const std::string& arg) const { return vm.count(arg); }
};

class ArgsManager {
public:
	ArgsManager(const std::string& _name): name(_name), parser(name) {}
	ArgsManager(std::string&& _name): name(std::move(_name)), parser(name) {}

	~ArgsManager() {
		libpar_done();
	}

	void parse(int argc, char ** argv, const std::vector<ArgsHolder*>& opt_args);

	template <typename T = char*>
	boost::optional<T> get(const std::string& section, const std::string& arg) const;

	template <typename T = char*>
	boost::optional <T> get(const std::string& arg) const { 
		return get<T>(name, arg);
	}

	bool has(const std::string& arg) const {
		return parser.count(arg) > 0 || has(name, arg);
	}

	bool has(const std::string& section, const std::string& arg) const {
		return libpar_getpar(section.c_str(), arg.c_str(), 0) != NULL;
	}

	size_t count(const std::string& arg) const {
		return parser.count(arg);
	}

private:
	template <typename T>
	boost::optional<T> get_overriden(const std::string& section, const std::string& arg) const;

	template <typename T>
	boost::optional<T> get_libparval(const std::string& section, const std::string& arg) const;

	std::string name;
	CmdLineParser parser;

	static constexpr char * SZARP_CFG = "/etc/szarp/szarp.cfg";
};

template <typename T>
boost::optional<T> ArgsManager::get(const std::string& section, const std::string& arg) const { 
	auto parser_ret = parser.get<T>(arg);
	if (parser_ret) return *parser_ret;

	auto ov_ret = get_overriden<T>(section, arg);
	if (ov_ret) return *ov_ret;

	auto lp_ret = get_libparval<T>(section, arg);
	if (lp_ret) return *lp_ret;
}

template <typename T>
boost::optional<T> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	auto ret = get_libparval<std::string>(section, arg);
	if (!ret) return boost::none;
	try {
		auto val = boost::lexical_cast<T>(*ret);
		return val;
	} catch(const boost::bad_lexical_cast& e) {
		return boost::none;
	}
}


template <>
boost::optional<const char*> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const;

template <>
boost::optional<char*> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const;

template <>
boost::optional<std::string> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const;

template <>
boost::optional<const std::string> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const;

template <typename T>
boost::optional<T> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const { 
	auto ret = get_overriden<std::string>(section, arg);
	if (!ret) return boost::none;
	try {
		auto val = boost::lexical_cast<T>(*ret);
		return val;
	} catch(const boost::bad_lexical_cast& e) {
		return boost::none;
	}
}


template <>
boost::optional<const std::string> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const;

template <>
boost::optional<const char*> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const;

template <>
boost::optional<std::string> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const;

template <>
boost::optional<char*> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const;

} // namespace basedmn

#endif
