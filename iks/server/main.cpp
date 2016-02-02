#include <iostream>
#include <string>
#include <functional>
#include <fstream>

#if GCC_VERSION < 40600
#include <cstdlib>
#include <ctime>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <liblog.h>

#include "../../config.h"

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
		("config_file", po::value<std::string>()->default_value(PREFIX "/iks/iks-server.ini"), "Custom configuration file.")
		("no_daemon", "If specified server will not daemonize.")
		("force_syslog", "Log to syslog even if no_daemon is specified")
		("pid_file", po::value<std::string>(), "Specify destination of pid file that should be used. If not set no file is created.")
		("log_level", po::value<unsigned>()->default_value(2), "Level how verbose should server be. Convention is: 0 - errors , 1 - warnings , 2 - info output , 3 and more - debug")
		("name", po::value<std::string>()->default_value(ba::ip::host_name()), "Servers name -- defaults to hostname.")
		("prefix,P", po::value<std::string>()->default_value(PREFIX), "Szarp prefix")
		("port,p", po::value<unsigned>()->default_value(9002), "Server port on which we will listen");

	po::variables_map vm; 
	CfgPairs pairs;
	CfgPairs server_config;
	CfgSections locs_cfg;

	int ret_code = 0;
	std::string pid_path;

	try { 
		po::store(po::parse_command_line(argc, argv, desc),  vm);

		if( boost::filesystem::exists(vm["config_file"].as<std::string>()) ) {
			auto parsed = po::parse_config_file<char>(vm["config_file"].as<std::string>().c_str(), desc, true);

			for( const auto& o : parsed.options )
				if( o.unregistered )
					pairs[boost::erase_all_copy(o.string_key," ")] = o.value[0];

			std::copy_if(pairs.begin(), pairs.end(),
				std::inserter(server_config, server_config.end()),
				[] (std::pair<std::string, std::string> p) { return p.first.find('.') == std::string::npos; });

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

	const char* logname = NULL;
	if (vm.count("force_syslog") || !vm.count("no_daemon")) 
		logname = "iks-server";
   

	sz_loginit( vm["log_level"].as<unsigned>() , logname , SZ_LIBLOG_FACILITY_DAEMON );
	sz_log(2,"Welcome, milord");

	try {
		boost::asio::io_service& io_service = GlobalService::get_service();

		tcp::endpoint endpoint(tcp::v4(), vm["port"].as<unsigned>() );
		TcpServer ts(io_service, endpoint);

		ba::signal_set signals(io_service, SIGINT, SIGTERM);
		signals.async_wait( bind(&ba::io_service::stop, &io_service) );

		/**
		 * Check if pid file can be written before daemonizing
		 * and write current pid.
		 */
		if( vm.count("pid_file") ) {
			pid_path = vm["pid_file"].as<std::string>();
			if( boost::filesystem::exists( pid_path ) ) {
				pid_path.clear(); /** prevent from removing not our pidfile */
				throw init_error("pid file " + pid_path + " already exists");
			}
			std::ofstream pf( vm["pid_file"].as<std::string>() );
			pf << get_pid();
		}

		/**
		 * Daemonize
		 */
		if( !vm.count("no_daemon") )
			if( !daemonize( io_service ) )
				throw init_error("Cannot become proper daemon");

		/**
		 * Write new pid after daemonizing
		 */
		if( !pid_path.empty() && !vm.count("no_daemon") ) {
			std::ofstream pf( pid_path , std::ofstream::trunc );
			pf << get_pid();
		}

		SzbaseWrapper::init( vm["prefix"].as<std::string>() );

		LocationsMgr lm;

		lm.add_locations( locs_cfg );
		lm.add_config( server_config );

		ts.on_connected   ( bind(&LocationsMgr::on_new_connection,&lm,p::_1) );
		ts.on_disconnected( bind(&LocationsMgr::on_disconnected  ,&lm,p::_1) );

		sz_log(2,"Start serving on port %d", vm["port"].as<unsigned>() );

		io_service.run();

	} catch( std::exception& e ) {
		sz_log(0,"Exception occurred: %s" , e.what() );
		ret_code = 1; /**< return error code */
	}

	/** Remove pid file at exit */
	if( !pid_path.empty() )
		boost::filesystem::remove( pid_path );

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

	return ret_code; 
}

