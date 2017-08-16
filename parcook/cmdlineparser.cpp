#include "cmdlineparser.h"
#include <iostream>

namespace basedmn {

namespace po = boost::program_options;

void CmdLineParser::parse(int argc, char ** argv, const std::vector<ArgsHolder*>& args_holders) {
	try {

		if (args_holders.size() == 0) throw po::error("No options specified");

		auto r_it = args_holders.rbegin();

		po::options_description opts = (*r_it)->get_options();
		po::positional_options_description p_opts;
		(*r_it)->add_positional_options(p_opts);


		for (std::advance(r_it, 1); r_it != args_holders.rend(); ++r_it) {
			opts.add((*r_it)->get_options());
			(*r_it)->add_positional_options(p_opts);
		}

		auto cmdlineparser = po::command_line_parser(argc, argv);
		po::parsed_options parsed_opts{ cmdlineparser.options(opts).positional(p_opts).run() };
		po::store(parsed_opts, vm);

		for (auto r_it = args_holders.rbegin(); r_it != args_holders.rend(); ++r_it) {
			(*r_it)->parse(parsed_opts, vm);
		}

	} catch(po::error& e) { 
		std::cerr << "Options parse error: " << e.what() << std::endl << std::endl; 
		exit(1);
	}
}

void ArgsManager::parse(int argc, char ** argv, const std::vector<ArgsHolder*>& opt_args) {
	parser.parse(argc, argv, opt_args);

	if (parser.has("base")) {
		#ifndef MINGW32
			libpar_init_from_folder("/opt/szarp/"+*parser.get<std::string>("base")+"/");
		#else
			// ignore
		#endif
	} else {
		#ifndef MINGW32
			libpar_init_with_filename(SZARP_CFG, 0);
		#else
			// ignore
		#endif
	}

	libpar_read_cmdline(&argc, argv);
}

template <>
boost::optional<const std::string> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	if (parser.vm.count("override") > 0) {
		auto overwritten_params = parser.vm["override"].as<std::vector<std::string>>();
		auto found_it = std::find(overwritten_params.begin(), overwritten_params.end(), arg);
		if (found_it != overwritten_params.end()) return *found_it;
	}

	return boost::none;
}

template <>
boost::optional<char*> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	char* libpar_val;
	if (section.empty()) 
		libpar_val = libpar_getpar(name.c_str(), arg.c_str(), 0);
	else
		libpar_val = libpar_getpar(section.c_str(), arg.c_str(), 0);
	
	if (libpar_val == NULL) return boost::none;

	return libpar_val;
}

template <>
boost::optional<const char*> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	auto val = get<char*>(section, arg);
	if (val) return *val;
	return boost::none;
}

template <>
boost::optional<std::string> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const {
	auto val = get<char*>(section, arg);
	if (val) return std::string(std::move(*val));
	return boost::none;
}

template <>
boost::optional<const std::string> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const {
	auto ret = move(get<std::string>(section, arg));

	// RVO won't help here
	if (ret) return std::move(*ret);
	return boost::none;
}


template <>
boost::optional<const char*> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = get<const std::string>(section, arg);
	if (ret) return ret->c_str();
	return boost::none;
}

template <>
boost::optional<std::string> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = std::move(get<const std::string>(section, arg));
	if (ret) return *ret;
	return boost::none;
}

template <>
boost::optional<char*> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = std::move(get<const char*>(section, arg));
	if (ret) return const_cast<char*>(*ret);
	return boost::none;
}

} // namespace basedmn


