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
#if GCC_VERSION < 40600
	srand(time(NULL));
#endif

	po::options_description desc("Options"); 

	desc.add_options() 
		("help,h", "Print this help messages")
		("config_file", po::value<std::string>()->default_value("iks.ini"), "Custom configuration file.")
		("no_daemon", "If specified server will not daemonize.")
		("name", po::value<std::string>()->default_value(ba::ip::host_name()), "Servers name -- defaults to hostname.")
		("prefix,P", po::value<std::string>()->default_value(PREFIX), "Szarp prefix")
		("port,p", po::value<unsigned>()->default_value(9002), "Server port on which we will listen");

	po::variables_map vm; 
	CfgPairs pairs;
	CfgSections locs_cfg;

	try 
	{ 
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
	} 
	catch(po::error& e) 
	{ 
		std::cerr << "Options parse error: " << e.what() << std::endl << std::endl; 
		std::cerr << desc << std::endl; 
		return 1; 
	} 

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
		daemonize( io_service );

	io_service.run();

	return 0; 
}

