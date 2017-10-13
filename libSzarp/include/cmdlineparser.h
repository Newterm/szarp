#ifndef CMDLINEPARSER__H__
#define CMDLINEPARSER__H__

#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include "liblog.h"

namespace po = boost::program_options;

class ArgsHolder {
public:
	virtual po::options_description get_options() const = 0;
	virtual void add_positional_options(po::positional_options_description&) const {}
	virtual void parse(const po::parsed_options&, const po::variables_map&) const {}
};

class DefaultArgs: public ArgsHolder {
public:
	po::options_description get_options() const override {
		po::options_description desc{"Options"};
		desc.add_options()
			("help,h", "Print this help messages")
			("version,V", "Output version")
			("base,b", po::value<std::string>(), "Base to read configuration from")
			("override,D", po::value<std::vector<std::string>>()->multitoken(), "Override default arguments (-Dparam=val)");

		po::options_description log_desc{"Logger Options"};
		log_desc.add_options()
			("debug,d", po::value<unsigned int>(), "Enables verbose logging")
			("log_file", po::value<std::string>(), "Specifies file to log into")
			("logger", po::value<std::string>(), "Specifies logger type (journald, file, cout)");

		return desc.add(log_desc);
	}

	void parse(const po::parsed_options& parsed_opts, const po::variables_map& vm) const override {
		if ( vm.count("help") || vm.count("version") ) { 
			std::cout << *parsed_opts.description << std::endl;
			exit(0);
		} 
	}
};


class IgnoreRemainigOptions: public ArgsHolder {
public:
	po::options_description get_options() const override {
		po::options_description desc{"General daemon arguments"};
		desc.add_options()
			("ignored", po::value<std::vector<std::string>>()->multitoken(), "ignored options");

		return desc;
	}

	void add_positional_options(po::positional_options_description& p_opts) const override {
		p_opts.add("ignored", -1);
	}
};


class DaemonArgs: public ArgsHolder {
public:
	po::options_description get_options() const override {
		po::options_description desc{"General daemon arguments"};
		desc.add_options()
			("single,s", "Forbid writing via IPC")
			("use-cfgdealer", "Enables configuring via config dealer")
			("cfgdealer-address", po::value<std::string>()->default_value("tcp://localhost:5555"), "Config dealer's address")
			("sniff", po::value<bool>()->default_value(false), "Do not initiate transmision, sniff traffic")
			("scan", po::value<std::string>(), "range of ID to use from config (e.g. 1-7)")
			("speed", po::value<unsigned int>(), "Port communication speed")
			("noconf", po::value<bool>()->default_value(false), "Do not load configuration")
			("askdelay", po::value<unsigned int>(), "Sets delay between queries to different units")
			("dumphex", po::value<bool>()->default_value(false), "Print trasmitted bytes in hex-terminal format")
			("device-no", po::value<unsigned int>(), "Device number in config file")
			("device-path", po::value<std::string>()->default_value("/dev/null"), "Device path attribute");

		return desc;
	}

	void add_positional_options(po::positional_options_description& p_opts) const override {
		p_opts.add("device-no", 1);
		p_opts.add("device-path", 1);
	}

	void parse(const po::parsed_options&, const po::variables_map& vm) const override {
		if (vm.count("device-no") == 0) throw std::runtime_error("Device number not specified! Cannot process!");
		// device-path is not necessary for most daemons
	}
};


class CmdLineParser {
	friend class ArgsManager;
	std::string name;
	po::variables_map vm; 

public:
	CmdLineParser(const std::string& name): name(name), vm() {}
	CmdLineParser(CmdLineParser&&) = default;
	CmdLineParser& operator=(CmdLineParser&&) = default;

	CmdLineParser(const CmdLineParser&) = default;
	CmdLineParser& operator=(const CmdLineParser&) = default;

	template <typename... As>
	void parse(int argc, const char ** argv, const ArgsHolder& main_args, const As&... as);

	template <typename T = std::string>
	boost::optional<T> get(const std::string& arg) const {
		if (vm.count(arg) > 0) {
			return vm[arg].as<T>();
		}
		
		return boost::none;
	}

	bool has(const std::string& arg) const { return vm.count(arg) != 0; }

	size_t count(const std::string& arg) const { return vm.count(arg); }

private:
	template <typename... As>
	void add_options(po::options_description& opts, po::positional_options_description& p_opts, const ArgsHolder& a, const As&... as);
	void add_options(po::options_description&, po::positional_options_description&) {}

	template <typename... As>
	void _parse(const po::parsed_options&, const po::variables_map& vm, const ArgsHolder& a, const As&... as);
	void _parse(const po::parsed_options&, const po::variables_map&) {};

};

template <typename... As>
void CmdLineParser::parse(int argc, const char ** argv, const ArgsHolder& main_args, const As&... as) {
	try {
		po::options_description opts = main_args.get_options();
		po::positional_options_description p_opts;
		main_args.add_positional_options(p_opts);

		add_options(opts, p_opts, as...);

		auto cmdlineparser = po::command_line_parser(argc, argv);
		po::parsed_options parsed_opts{ cmdlineparser.options(opts).positional(p_opts).allow_unregistered().run() };
		po::store(parsed_opts, vm);

		_parse(parsed_opts, vm, main_args, as...);
	} catch(po::error& e) { 
		sz_log(0, "Options parse error: %s", e.what());
		exit(1);
	}
}

template <typename... As>
void CmdLineParser::add_options(po::options_description& opts, po::positional_options_description& p_opts, const ArgsHolder& a, const As&... as) {
	opts.add(a.get_options());
	a.add_positional_options(p_opts);

	add_options(opts, p_opts, as...);
}

template <typename... As>
void CmdLineParser::_parse(const po::parsed_options& parsed_opts, const po::variables_map& vm, const ArgsHolder& a, const As&... as) {
	a.parse(parsed_opts, vm);
	_parse(parsed_opts, vm, as...);
}


#endif
