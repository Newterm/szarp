#include "cmdlineparser.h"
#include <iostream>

namespace po = boost::program_options;

void CmdLineParser::parse(int argc, const char ** argv, const std::vector<ArgsHolder*>& args_holders) {
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
		po::parsed_options parsed_opts{ cmdlineparser.options(opts).positional(p_opts).allow_unregistered().run() };
		po::store(parsed_opts, vm);

		for (auto r_it = args_holders.rbegin(); r_it != args_holders.rend(); ++r_it) {
			(*r_it)->parse(parsed_opts, vm);
		}

	} catch(po::error& e) { 
		std::cerr << "Options parse error: " << e.what() << std::endl << std::endl; 
		exit(1);
	}
}
