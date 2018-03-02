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
 * Demon do odczytywania danych generowanych przez zewnêtrzny program.
 * 
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 * 
 * $Id$
 */

/*
 SZARP daemon description block.

 @description_start

 @class 4

 @devices Daemon reads and parses standard output of external program.
 @devices.pl Sterownik czyta wyj¶cie generowane przez zewnêtrzny program.

 @protocol Running program output is parsed as raw parameter values (without precision), one
 parameter per line.
 @protocol.pl Sterownik parsuje wyj¶cie uruchamianego programu, spodziewaj±c siê surowych
 (bez precyzji) warto¶ci kolejnych parametrów w kolejnych liniach.

 @config 'path' attribute of device element in params.xml contains path of program to run. Optional
 'frequency' attribute from extra namespace contains number of seconds between program runs (
 default is 10 seconds). 'options' attribute is splitted on white spaces and passed as options to program.

 @config.pl Atrybut 'path' elementu device w pliku params.xml zawiera ¶cie¿kê do uruchamianego
 programu. Opcjonalny atrybut 'frequency' z dodatkowej przestrzeni nazw okre¶la co ile sekund uruchamiaæ
 program (domy¶lnie co 10). Zawarto¶æ atrybutu 'options' jest dzielona na s³owa i przekazywana jako
 opcje do uruchamianego programu.

 @config_example
 <device 
      xmlns:exec="http://www.praterm.com.pl/SZARP/ipk-extra"
      daemon="/opt/szarp/bin/execdmn" 
      path="/opt/szarp/bin/some_script"
      exec:frequency="30"
      options="--some-option -f some-argument">
      ...

 @description_end
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <signal.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include <libxml/parser.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "ipchandler.h"
#include "liblog.h"
#include "xmlutils.h"
#include "execute.h"

#include "conversion.h"
#include "custom_assert.h"

#include <string>
#include <boost/tokenizer.hpp>
using std::string;

/**
 * Modbus communication config.
 */
class ExecDaemon {
public :
	/**
	 * @param params number of params to read
	 */
	ExecDaemon(int params);

	/** 
	 * Parses XML 'device' element.
	 * @return - 0 on success, 1 on error 
	 */
	int ParseConfig(DaemonConfig * cfg, const ArgsManager& args_mgr);

	/**
	 * Tries to execute program.
	 * @param ipc IPCHandler
	 * @return 0 on success, 1 on error
	 */
	int Exec(IPCHandler *ipc);

	/** Wait for next cycle. */
	void Wait();
	
private:
	int m_params_count;	/**< size of params array */

	bool m_single{false};		/**< are we in 'single' mode */
	char ** m_argvp{nullptr};	/**< command to execute */
	int m_freq{10};		/**< execute frequency */
	time_t m_last{0};
};

/**
 * @param params number of params to read
 * @param sends number of params to send (write)
 */
ExecDaemon::ExecDaemon(int params) 
{
	ASSERT(params >= 0);
	m_params_count = params;
}

int ExecDaemon::ParseConfig(DaemonConfig * cfg, const ArgsManager& args_mgr)
{
	std::vector<std::string> script_opts{};
	const auto& cmdline = args_mgr.get_cmdlineparser();
	if (cmdline.has("script-options")) {
		script_opts = cmdline.get<std::vector<std::string>>("script-options").get();
	}

	if (!args_mgr.has("device-path")) {
		sz_log(0, "No script path specified!");
		return 1;
	}

	std::string options;
	const std::string path = *args_mgr.get<std::string>("device-path");

	TDevice* device = cfg->GetDevice();
	m_freq = device->getAttribute<int>("extra:frequency", 10);

	m_single = args_mgr.has("single");

	m_argvp = (char **) malloc(sizeof(char *) * script_opts.size() + 2); // +2 for path and NULL terminator
	m_argvp[0] = strdup(path.c_str());

	int j = 1;
	for (const auto& opt: script_opts) {
		m_argvp[j++] = strdup(opt.c_str());
		if (m_single) {
			options += opt;
		}
	}

	m_argvp[j] = NULL;

	if (m_single) {
		std::cerr << "Using command " << path << " " << options << " with frequency " << m_freq << std::endl;
	}

	return 0;
}


void ExecDaemon::Wait() 
{
	time_t t;
	t = time(NULL);
	
	
	if (t - m_last < m_freq) {
		int i = m_freq - (t - m_last);
		while (i > 0) {
			i = sleep(i);
		}
	}
	m_last = time(NULL);
	return;
}

int ExecDaemon::Exec(IPCHandler *ipc)
{
	char *ret;

	if (!m_single) {
		for (int i = 0; i < m_params_count; i++) {
			ipc->m_read[i] = SZARP_NO_DATA;
		}
	}
			
	ret = execute_substv(m_argvp[0], m_argvp);
	if (ret == NULL)
		return 1;

	unsigned b=0 , e=0;

	for (int i = 0; i < m_params_count ; ++i )
	{
		b = e++;
		if( ret[b] == '\0' || i >= m_params_count ) continue;
		while( ret[e] != '\0' && ret[e] != '\n' && ret[e] != ' ' ) ++e;

		char *errptr;
		int d;
		char t = ret[e];
		ret[e] = '\0';
		d = (int)strtod(ret+b, &errptr);
		if (*errptr != 0) {
			sz_log(2, "Got incorrect string for param i: '%s'", ret+b);
			d = SZARP_NO_DATA;
		} else {
			sz_log(10, "Got string for param i: '%s', converted to value %d", ret+b, d);
		}

		if (m_single) {
			std::cout << "Got value " << d << " for param " << i << "\n";
		} else {
			ipc->m_read[i] = d;
		}

		ret[e]= t;
	}

	free(ret);
	
	return 0;
}

RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d caught, exiting", signum);
	signal(signum, SIG_DFL);
	raise(signum);
}

void init_signals()
{
	int ret;
	struct sigaction sa;
	sigset_t block_mask;

	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = terminate_handler;
	ret = sigaction(SIGTERM, &sa, NULL);
	ASSERT(ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	ASSERT(ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	ASSERT(ret == 0);
}

class ScriptExecDaemonArgs: public ArgsHolder {
public:
	po::options_description get_options() const override {
		po::options_description desc{"Script options"};
		desc.add_options()
			("script-options", po::value<std::vector<std::string>>()->multitoken(), "Set script options on execution");

		return desc;
	}
};


int main(int argc, char *argv[])
{
	ArgsManager args_mgr("execdmn");
	args_mgr.parse(argc, argv, DefaultArgs(), DaemonArgs(), ScriptExecDaemonArgs());

	DaemonConfig* cfg = new DaemonConfig("execdmn");
	ASSERT(cfg != NULL);

	if (cfg->Load(args_mgr)) {
		sz_log(0, "Error loading configuration, exiting.");
		return 1;
	}

	ExecDaemon* dmn = new ExecDaemon(cfg->GetDevice()->GetFirstUnit()->GetParamsCount());

	ASSERT(dmn != NULL);

	if (dmn->ParseConfig(cfg, args_mgr)) {
		sz_log(0, "Error parsing config, exiting.");
		return 1;
	}

	IPCHandler *ipc;

	try {
		ipc = new IPCHandler(cfg);
	} catch(const std::exception& e) {
		sz_log(0, "Error initializing IPC: %s", e.what());
		return 1;
	}

	init_signals();

	sz_log(2, "Starting ExecDaemon");

	while (true) {
		dmn->Wait();
		dmn->Exec(ipc);
		/* send data from parcook segment */
		ipc->GoParcook();
	}
	return 0;
}
