/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/*
 * Pawe³ Pa³ucha 2002
 *
 * Main paramd file.
 * $Id$
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "szarp_config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <openssl/ssl.h>

#include <libpar.h>
#include <liblog.h>
#include <daemon.h>

#include "conversion.h"
#include "server.h"
#include "param_tree.h"
#include "auth_server.h"
#include "config_load.h"
#include "ssl_server.h"
#include "access.h"
#include "tokens.h"
#include "ipk2xml.h"
#include "szbase/szbbase.h"

#include <sys/wait.h>

/**
 * SIGINT handler - log and exit.
 * @param signum ignored
 */
void int_handler(int signum)
{
	sz_log(2, "paramd exiting on signal");
	exit(0);
}

/**
 * Main program function. Reads configuration data, initializes connection with
 * parcook, starts HTTP server. Also, initializes xmllib to enter thread-safe
 * mode.
 */
int main(int argc, char **argv)
{
	char *logfile;
	int log_level;

	/* set initial logging, process liblog and libpar command line
	 * params */
	log_level = loginit_cmdline(2, NULL, &argc, argv);

	logfile = libpar_getpar("paramd", "log_level", 0);
	if (logfile) {
		log_level = atoi(logfile);
		free(logfile);
	}
	logfile = libpar_getpar("paramd", "log", 0);
	if (logfile == NULL)
		logfile = strdup("paramd");
	if (sz_loginit(log_level, logfile) < 0) {
		sz_log(0, "paramd: cannot open log file '%s', exiting",
		    logfile);
		return 1;
	}
	sz_log(0, "paramd: started");

	IPKLoader *ploader;
	ParamTree *pt;

	// initialize xml parser for thread-safety
	xmlInitParser();

	// load configuration data from configuration file
	ConfigLoader * cloader = new ConfigLoader("/etc/szarp/szarp.cfg", "paramd",
			&argc, argv);

	PTTParamProxy *ppt = new PTTParamProxy(strdup(cloader->getString("parcook_path", NULL)));


	char * szarp_data_root = libpar_getpar("", "szarp_data_root", 1);
	assert(szarp_data_root != NULL);
	char * ipk_prefix = libpar_getpar("", "config_prefix", 1);
	assert (ipk_prefix != NULL);

	IPKContainer::Init(SC::L2S(szarp_data_root), SC::L2S(PREFIX), L"");
	Szbase::Init(SC::L2S(szarp_data_root), NULL);

	Szbase * szbase = Szbase::GetObject();
	
	// load param information from PTT.act file
	ploader = new IPKLoader(ipk_prefix, ppt, szbase);

	ParamDynamicTreeData treedata = ploader->get_dynamic_tree();
	if (!treedata.tree) {
		sz_log(0, "cannot load information from '%s' file",
				cloader->getString("IPK", NULL));
		return -1;
	}

	TreeProcessor* tp = new TreeProcessor(cloader);
	// build param tree
	pt = new ParamTree(treedata, cloader, tp);
	// initialize content handler
	ContentHandler conh(pt, tp, ploader, strdup(cloader->getString("IPK", NULL)));

	char * sections = cloader->getString("servers", NULL);
	char **toks = NULL;
	int tokc = 0;
	tokenize(sections, &toks, &tokc);
	if (tokc < 0) {
		sz_log(0, "0 servers found, exiting");
		return 1;
	}

	// free memory
	tokenize(NULL, &toks, &tokc);

	char *no_daemon_par = libpar_getpar("paramd", "no_daemon", 0);
	bool should_daemonize = true;
	if (no_daemon_par != nullptr && boost::lexical_cast<bool>(no_daemon_par)) {
		should_daemonize = false;
	}


	if (should_daemonize) {
		// go into background
		go_daemon();
	}
	
	signal(SIGINT, int_handler);

	try {
		Server::StartAll(cloader, &conh, !should_daemonize);
	} catch (const std::exception& e) {
		sz_log(0, e.what());
		exit(1);
	}

	if (!should_daemonize) {
		signal(SIGTERM, int_handler);
		signal(SIGQUIT, int_handler);
		signal(SIGHUP,  int_handler);
		signal(SIGCHLD, int_handler);

		wait(NULL);
	}

	return 0;
}

