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

#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "liblog.h"
#include "execute.h"

#include "custom_assert.h"

#include "base_daemon.h"

/**
 * Modbus communication config.
 */
class ExecDaemon {
public :
	/**
	 * @param params number of params to read
	 */
	ExecDaemon(BaseDaemon& base_dmn);

	/** 
	 * Parses XML 'device' element.
	 * @return - 0 on success, 1 on error 
	 */
	void ParseConfig(BaseDaemon& base_dmn);

	/**
	 * Tries to execute program.
	 * @param ipc IPCHandler
	 * @return 0 on success, 1 on error
	 */
	void Exec(BaseDaemon& base_dmn);
	
private:
	int m_params_count;	/**< size of params array */

	char ** m_argvp{nullptr};	/**< command to execute */
	int m_freq{10};		/**< execute frequency */
};

/**
 * @param params number of params to read
 * @param sends number of params to send (write)
 */
ExecDaemon::ExecDaemon(BaseDaemon& base_dmn) 
{
	ParseConfig(base_dmn);
	base_dmn.setCycleHandler([this](BaseDaemon& base_dmn){ Exec(base_dmn); });
}

void ExecDaemon::ParseConfig(BaseDaemon& base_dmn)
{
	const DaemonConfigInfo& cfg = base_dmn.getDaemonCfg();
	const ArgsManager& args_mgr = base_dmn.getArgsMgr();

	m_params_count = cfg.GetParamsCount();
	ASSERT(m_params_count >= 0);

	if (auto cycle_freq_opt = cfg.GetDeviceInfo()->getOptAttribute<int>("extra:frequency")) {
		base_dmn.setCyclePeriod(*cycle_freq_opt);
		sz_log(2, "Using custom frequency %d", *cycle_freq_opt);
	}

	std::vector<std::string> script_opts{};
	const auto& cmdline = args_mgr.get_cmdlineparser();
	if (cmdline.has("script-options")) {
		script_opts = cmdline.get<std::vector<std::string>>("script-options").get();
	}

	auto path_opt = args_mgr.get<std::string>("device-path");
	if (!path_opt) {
		throw std::runtime_error("No script path specified!");
	}

	m_argvp = (char **) malloc(sizeof(char *) * script_opts.size() + 2); // +2 for path and NULL terminator
	m_argvp[0] = strdup(path_opt->c_str());

	std::string options;

	int j = 1;
	for (const auto& opt: script_opts) {
		m_argvp[j++] = strdup(opt.c_str());
		options += opt;
	}
	m_argvp[j] = NULL;

	sz_log(2, "Using command %s %s", path_opt->c_str(), options.c_str());
}

void ExecDaemon::Exec(BaseDaemon& base_dmn)
{
	char *ret;
	ret = execute_substv(m_argvp[0], m_argvp);
	if (ret == NULL) {
		for (int i = 0; i < m_params_count; i++) {
			base_dmn.getIpc().setNoData(i);
		}
			
		sz_log(2, "Encountered error while executing script.");
		base_dmn.getIpc().publish();
		return;
	}

	unsigned e = 0;

	for (int i = 0; i < m_params_count ; ++i)
	{
		unsigned b = e++;
		if (ret[b] == '\0' || i >= m_params_count) continue;
		while (ret[e] != '\0' && ret[e] != '\n' && ret[e] != ' ') ++e;

		char *errptr;
		char t = ret[e];
		ret[e] = '\0';
		int d = (int)strtod(ret+b, &errptr);
		if (*errptr != 0) {
			sz_log(2, "Got incorrect string for param i: '%s'", ret+b);
			base_dmn.getIpc().setNoData(i);
		} else {
			sz_log(10, "Got string for param i: '%s', converted to value %d", ret+b, d);
			base_dmn.getIpc().setRead<int16_t>(i, d);
		}

		ret[e]= t;
	}

	base_dmn.getIpc().publish();

	free(ret);
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
	args_mgr.initLibpar();

	BaseDaemonFactory::Go<ExecDaemon>("execdmn", std::move(args_mgr));
	return 0;
}
