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
 * MA 02110-1301, USA. """
 *
 */

#include <iostream>
#include <boost/program_options.hpp>

#include "szarp.h"
#include "daemon.h"
#include "liblog.h"

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
		/*** parse given arguments using boost::program_options (po) ***/

		/* defaults */
		int loglevel = 2;
		bool daemonize = true;

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
				<< "License GPLv2+: GNU GPL version 2 or later\n"
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
			daemonize = false;
		}

		/*** start pserverLITE service ***/

		/* set logging */
		sz_loginit(10, NULL, SZ_LIBLOG_FACILITY_DAEMON);

		char* logfile = strdup("pserver-lite");
		if (sz_loginit(loglevel, logfile) < 0) {
			sz_log(0, "cannot open log file '%s', exiting", logfile);
			return EXIT_FAILURE;
		}
		sz_log(0, "starting pserverLITE...");

		/* read configuration (section "probes_server") */
		libpar_init_with_filename("/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg", 1);

		char* pserver_port = libpar_getpar("probes_server", "port", 1);
		char* pserver_address = libpar_getpar("probes_server", "address", 1);

		libpar_done();

		/* daemonize pserver-lite process */
		if (daemonize) {
			if (go_daemon() != 0) {
				sz_log(0, "failed to daemonize process, exiting...");
				return EXIT_FAILURE;
			}
		}

		// TODO: start probes server service
		std::cout
			<< "pserverLITE started.\n"
			<< "pserver_port = " << atoi(pserver_port) << "\n"
			<< "pserver_address = " << pserver_address // empty by default
			<< std::endl;
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
		sz_log(0, "ERROR: %s, exiting...", e.what());
		std::cerr << PROGRAM_NAME << ": ERROR: " << e.what() << std::endl;

		return EXIT_FAILURE;
	}
	catch (...) {
		sz_log(0, "ERROR: exception of unknown type! Exiting...");
		std::cerr
			<< PROGRAM_NAME
			<< ": ERROR: exception of unknown type!" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

}   /* end of main() */

