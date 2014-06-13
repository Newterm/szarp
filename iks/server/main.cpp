#include <iostream>
#include <string>
#include <functional>

#if GCC_VERSION < 40600
#include <cstdlib>
#include <ctime>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <liblog.h>

#include "net/tcp_server.h"

#include "locations/manager.h"

#include "global_service.h"
#include "daemon.h"

#include "data/szbase_wrapper.h"

#include "utils/config.h"

namespace po = boost::program_options;
namespace ba = boost::asio;

using boost::asio::ip::tcp;

using std::bind;
namespace p = std::placeholders;

typedef std::vector<std::string> LocsList;

int main( int argc , char** argv )
{
	srand(time(NULL));

	po::options_description desc("Options"); 

	desc.add_options() 
		("help,h", "Print this help messages")
		("config_file", po::value<std::string>()->default_value("iks.ini"), "Custom configuration file.")
		("no_daemon", "If specified server will not daemonize.")
		("log_level", po::value<unsigned>()->default_value(2), "Level how verbose should server be. Convention is: 0 - errors , 1 - warnings , 2 - info output , 3 and more - debug")
		("name", po::value<std::string>()->default_value(ba::ip::host_name()), "Servers name -- defaults to hostname.")
		("prefix,P", po::value<std::string>()->default_value(PREFIX), "Szarp prefix")
		("port,p", po::value<unsigned>()->default_value(9002), "Server port on which we will listen");

	po::variables_map vm; 
	CfgPairs pairs;
	CfgSections locs_cfg;

	try { 
		po::store(po::parse_command_line(argc, argv, desc),  vm);

		if( boost::filesystem::exists(vm["config_file"].as<std::string>()) ) {
			auto parsed = po::parse_config_file<char>(vm["config_file"].as<std::string>().c_str(), desc, true);

			for( const auto& o : parsed.options )
				if( o.unregistered )
					pairs[boost::erase_all_copy(o.string_key," ")] = o.value[0];

			locs_cfg.from_flat( pairs );
			po::store(parsed,vm);
		}

		if ( vm.count("help")  ) { 
			std::cout << desc << std::endl; 
			return 0; 
		} 

		po::notify(vm);

	} catch(po::error& e) { 
		std::cerr << "Options parse error: " << e.what() << std::endl << std::endl; 
		std::cerr << desc << std::endl; 
		return 1; 

	} 

	const char* logname = vm.count("no_daemon") ? NULL : "iks-server" ;

	sz_loginit( vm["log_level"].as<unsigned>() , logname , SZ_LIBLOG_FACILITY_DAEMON );
	sz_log(2,"Welcome, milord");

	try {
		SzbaseWrapper::init( vm["prefix"].as<std::string>() );

		boost::asio::io_service& io_service = GlobalService::get_service();

		tcp::endpoint endpoint(tcp::v4(), vm["port"].as<unsigned>() );
		TcpServer ts(io_service, endpoint);

		LocationsMgr lm;

		lm.add_locations( locs_cfg );

		ts.on_connected   ( bind(&LocationsMgr::on_new_connection,&lm,p::_1) );
		ts.on_disconnected( bind(&LocationsMgr::on_disconnected  ,&lm,p::_1) );

		ba::signal_set signals(io_service, SIGINT, SIGTERM);
		signals.async_wait( bind(&ba::io_service::stop, &io_service) );

		if( !vm.count("no_daemon") )
			if( !daemonize( io_service ) )
				throw init_error("Cannot become proper daemon");

		sz_log(2,"Start serving on port %d", vm["port"].as<unsigned>() );

		io_service.run();

	} catch( std::exception& e ) {
		sz_log(0,"Exception occurred: %s" , e.what() );
	}

	/**
	 * Quit with polite message
	 */
	switch( rand() % 23 ) {
	case  0 : sz_log(2 , "Au revoir"); break;
	case  1 : sz_log(2 , "Arrivederci"); break;
	case  2 : sz_log(2 , "Auf Wiedersehen"); break;
	case  3 : sz_log(2 , "See you later"); break;
	case  4 : sz_log(2 , "So long!"); break;
	default : sz_log(2 , "Goodbye"); break;
	}

	return 0; 
}

