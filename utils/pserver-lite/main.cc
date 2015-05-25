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

/* used namespaces */
namespace po = boost::program_options;

/* program name and version string constant */
const std::string PROGRAM_NAME("pserver-lite");
const std::string VERSION("1.0alpha");

/**
 * This is the main() function.
 */
int main (int argc, char **argv)
{
    try {
        /*** parse given arguments using boost::program_options (po) ***/

        /* defaults */
        int debug_level = 2;
        bool daemonize = true;

        /* command-line options */
        po::options_description opts("Startup");
        opts.add_options()
            ("help,h", "print this help.")
            ("version,V", "display the version of pserverLITE and exit.")
            ("debug,d", po::value<int>(&debug_level), "set debug level.")
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
                << "pserverLITE " << VERSION
                << ", SZARP Probes Server LITE.\n"
                << "Usage: " << PROGRAM_NAME << " [OPTION]...\n\n"
                << opts << "\n"
                // TODO: config file info
                << "Mail bug reports and suggestions to <coders@newterm.pl>."
                << std::endl;
            return EXIT_SUCCESS;
        }
        /* -V, --version: print program version and copyright info and exit */
        if (vm.count("version")) {
            std::cout
                << PROGRAM_NAME << " " << VERSION
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
        if (debug_level < 0 || debug_level > 10) {
            throw po::error("debug level must be between 0 and 10");
        }
        if (vm.count("no-daemon")) {
            daemonize = false;
        }

        // TODO: read configuration from szarp.cfg

        // TODO: start probes server service
        std::cout << "Starting " << PROGRAM_NAME << "...\n"
                  << "debug_level = " << debug_level << "\n"
                  << "daemonize = " << daemonize
                  << std::endl;
    }
    catch (po::error& e) {
        std::cerr << PROGRAM_NAME << ": ERROR: " << e.what() << "\n"
                  << "Usage: " << PROGRAM_NAME << " [OPTION]...\n\n"
                  << "Try `pserver-lite --help' for more options."
                  << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception& e) {
        // TODO: write to log
        std::cerr << PROGRAM_NAME << ": ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        // TODO write to log
        std::cerr << PROGRAM_NAME
                  << ": ERROR: exception of unknown type!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}   /* end of main() */

