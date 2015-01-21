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

#include "liblog.h"

#include "global_service.h"

#include "net/tcp_server.h"

#include "daemon.h"
#include "probes_srv.h"

namespace po = boost::program_options;
namespace ba = boost::asio;

using boost::asio::ip::tcp;

using std::bind;
namespace p = std::placeholders;

int main( int argc , char** argv )
{
	srand(time(NULL));

	po::options_description desc("Options"); 

	desc.add_options() 
		("help,h", "Print this help messages")
		("config_file", po::value<std::string>()->default_value(PREFIX "/iks/iks-server.ini"), "Custom configuration file.")
		("no_daemon", "If specified server will not daemonize.")
		("pid_file", po::value<std::string>(), "Specify destination of pid file that should be used. If not set no file is created.")
		("log_level", po::value<unsigned>()->default_value(2), "Level how verbose should server be. Convention is: 0 - errors , 1 - warnings , 2 - info output , 3 and more - debug")
		("name", po::value<std::string>()->default_value(ba::ip::host_name()), "Servers name -- defaults to hostname.")
		("prefix,P", po::value<std::string>()->default_value(PREFIX), "Szarp prefix")
		("port,p", po::value<unsigned>()->default_value(9002), "Server port on which we will listen");

	int ret_code = 0;
        int port = 8090;

	std::string pid_path;

        std::string cache_dir = "/var/cache/szarp";


	sz_loginit( 10 , "probes_server2" , SZ_LIBLOG_FACILITY_DAEMON );
	sz_log(2, "Welcome, milord");

	try {
		boost::asio::io_service& io_service = GlobalService::get_service();

		tcp::endpoint endpoint(tcp::v4(), port );
		TcpServer ts(io_service, endpoint);

                ProbesServer probes_srv(io_service, cache_dir, 4);


		ts.on_connected   ( bind(&ProbesServer::on_new_connection, &probes_srv, p::_1) );
		ts.on_disconnected( bind(&ProbesServer::on_disconnected  , &probes_srv, p::_1) );

		ba::signal_set signals(io_service, SIGINT, SIGTERM);
		signals.async_wait( bind(&ba::io_service::stop, &io_service) );

		sz_log(2,"Start serving on port 8090" );

		io_service.run();

	} catch( std::exception& e ) {
		sz_log(0,"Exception occurred: %s" , e.what() );
		ret_code = 1; /**< return error code */
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

	return ret_code; 
}

