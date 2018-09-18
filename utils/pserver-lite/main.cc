/**
 * Probes Server LITE is a part of SZARP SCADA software.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <cstring>
#include <iostream>
#include <boost/program_options.hpp>

#include "szarp.h"
#include "daemonize.h"
#include "liblog.h"
#include "pserver_service.h"

/* used namespaces */
namespace po = boost::program_options;

/* program name and version string constant */
const std::string PROGRAM_NAME("pserver-lite");
const std::string VERSION_STR("1.0alpha");

/**
 * This is the main() function.
 */
int main (int argc, char **argv)
{
	try {
		/*** parse given program options ***/

		/* defaults */
		int loglevel = 2;
		bool no_daemon = false;

		/* command-line options */
		po::options_description opts("Startup");
		opts.add_options()
			("help,h", "print this help.")
			("version,V", "display the version of pserverLITE and exit.")
			("debug,d", po::value<int>(&loglevel), "set debug level.")
			("no-daemon,n", "do not daemonize.")
			;

		/* positional arguments */
		const po::positional_options_description pos; // empty!!!

		/** parse **/
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
				options(opts).positional(pos).run(), vm);
		po::notify(vm);

		/* -h, --help: print help message and exit */
		if (vm.count("help")) {
			std::cout
				<< "pserverLITE " << VERSION_STR
				<< ", SZARP Probes Server LITE.\n"
				<< "Usage: " << PROGRAM_NAME << " [OPTION]...\n\n"
				<< opts << "\n"
				<< "Configuration (read from file "
				<< "/etc/" << PACKAGE_NAME << "/" << PACKAGE_NAME <<".cfg):\n"
				<< "  * Section 'probes_server':\n"
				<< "      port\tport on which server should listen. [mandatory]\n"
				<< "      address\taddress of interface on which server should listen\n"
				<< "             \t(empty means 'any'). [mandatory]\n\n"
				<< "Mail bug reports and suggestions to <coders@newterm.pl>."
				<< std::endl;

			return EXIT_SUCCESS;
		}
		/* -V, --version: print program version and copyright info and exit */
		if (vm.count("version")) {
			std::cout
				<< PROGRAM_NAME << " " << VERSION_STR
				<< ", SZARP Probes Server LITE.\n\n"
				<< "Copyright (C) 2015 Newterm\n"
				<< "License GPLv3: GNU GPL version 3 or later\n"
				<< "<http://www.gnu.org/licenses/gpl.html>.\n"
				<< "This is free software: you are free to change and redistribute it.\n"
				<< "There is NO WARRANTY, to the extent permitted by law.\n\n"
				<< "Written by Aleksy Barcz, Marcin Harasimczuk and Tomasz Pieczerak."
				<< std::endl;

			return EXIT_SUCCESS;
		}

		/* validate other options values */
		if (loglevel < 0 || loglevel > 10) {
			throw po::error("debug level must be between 0 and 10");
		}
		if (vm.count("no-daemon")) {
			no_daemon = true;
		}

		/*** start pserverLITE service ***/

		/* set logging */
		sz_loginit(loglevel, NULL, SZ_LIBLOG_FACILITY_DAEMON);
		sz_log(1, "starting pserverLITE...");

		/* read configuration (section "probes_server") */
		libpar_init_with_filename("/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg", 1);

		char* pserver_address_cstr = libpar_getpar("probes_server", "address", 1);
		char* pserver_port_cstr = libpar_getpar("probes_server", "port", 1);

		libpar_done();

		/* convert configuration variables to something better... */
		std::string pserver_address;
		unsigned short pserver_port;

		if (strlen(pserver_address_cstr) == 0)
			pserver_address = std::string("*");
		else
			pserver_address = std::string(pserver_address_cstr);

		pserver_port = atoi(pserver_port_cstr);

		/* remove uneeded variables */
		free(pserver_address_cstr);
		free(pserver_port_cstr);
		pserver_address_cstr = nullptr;
		pserver_port_cstr = nullptr;

		/* daemonize pserver-lite process */
		if (!no_daemon) {
			if (daemonize(DMN_SIGPIPE_IGN) != 0) {
				sz_log(1, "failed to daemonize process, exiting...");
				return EXIT_FAILURE;
			}
		}

		/** start pserver-lite service **/
		std::shared_ptr<PServerService> pserver =
			std::make_shared<PServerService>(pserver_address, pserver_port);

		sz_log(2, "starting pserverLITE service on address %s, port %d",
				pserver_address.c_str(), pserver_port);

		pserver->run();
	}
	catch (po::error& e) {
		std::cerr
			<< PROGRAM_NAME << ": ERROR: " << e.what() << "\n"
			<< "Usage: " << PROGRAM_NAME << " [OPTION]...\n\n"
			<< "Try `pserver-lite --help' for more options."
			<< std::endl;

		return EXIT_FAILURE;
	}
	catch (std::exception& e) {
		sz_log(1, "ERROR: %s, exiting...", e.what());
		std::cerr << PROGRAM_NAME << ": ERROR: " << e.what() << std::endl;

		return EXIT_FAILURE;
	}
	catch (...) {
		sz_log(1, "ERROR: exception of unknown type! Exiting...");
		std::cerr
			<< PROGRAM_NAME
			<< ": ERROR: exception of unknown type!" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

}   /* end of main() */

