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

#ifdef LIBXML_XPATH_ENABLED

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

#define DAEMON_INTERVAL 10

/**
 * Modbus communication config.
 */
class ExecDaemon {
public :
	/**
	 * @param params number of params to read
	 */
	ExecDaemon(int params);
	~ExecDaemon();

	/** 
	 * Parses XML 'device' element.
	 * @return - 0 on success, 1 on error 
	 */
	int ParseConfig(DaemonConfig * cfg);

	/**
	 * Tries to execute program.
	 * @param ipc IPCHandler
	 * @return 0 on success, 1 on error
	 */
	int Exec(IPCHandler *ipc);

	/** Wait for next cycle. */
	void Wait();
	
protected :
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckFreq(xmlXPathContextPtr xp_ctx, int dev_num);
	
	int m_single;		/**< are we in 'single' mode */
	public:
	int m_params_count;	/**< size of params array */
	protected:
	char ** m_argvp;	/**< command to execute */
	int m_freq;		/**< execute frequency */
	time_t m_last;
};

/**
 * @param params number of params to read
 * @param sends number of params to send (write)
 */
ExecDaemon::ExecDaemon(int params) 
{
	ASSERT(params >= 0);

	m_params_count = params;
	m_freq = DAEMON_INTERVAL;
	m_single = 0;
	m_last = 0;
	m_argvp = NULL;
}

ExecDaemon::~ExecDaemon() 
{
}


int ExecDaemon::XMLCheckFreq(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;
	long l;

	int ret = asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@exec:frequency",
			dev_num);
	(void)ret;
	ASSERT(e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx, false);
	free(e);
	if (c == NULL)
	{
		sz_log(2, "using exec default frequency %d", m_freq);
		return 0;
	}
	l = strtol((char *)c, &e, 0);
	if ((*c == 0) || (*e != 0)) {
		sz_log(1, "incorrect value '%s' for exec:frequency, number expected",
				(char *)c);
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	if (l < DAEMON_INTERVAL) {
		sz_log(1, "value '%ld' for exec:frequency to small", l);
		return 1;
	}
	m_freq = (int)l;
	sz_log(2, "using exec frequency %d", m_freq);
	return 0;
}

int ExecDaemon::ParseConfig(DaemonConfig * cfg)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xp_ctx;
	int dev_num;
	int ret;
	
	/* get config data */
	ASSERT(cfg != NULL);
	dev_num = cfg->GetLineNumber();
	ASSERT(dev_num > 0);
	doc = cfg->GetXMLDoc();
	ASSERT(doc != NULL);

	/* prepare xpath */
	xp_ctx = xmlXPathNewContext(doc);
	ASSERT(xp_ctx != NULL);

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "exec",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	ASSERT(ret == 0);

	if (XMLCheckFreq(xp_ctx, dev_num))
		return 1;

	xmlXPathFreeContext(xp_ctx);
	
	m_single = cfg->GetSingle();
	
	std::wstring path = cfg->GetDevice()->GetPath();
	std::wstring options = cfg->GetDevice()->GetOptions();
	if (m_single) {
		printf("Using command '%s'\n", SC::S2A(path + L" " + options).c_str());
	}

	using namespace boost;
        typedef escaped_list_separator<wchar_t, std::char_traits<wchar_t> > wide_sep;
        typedef tokenizer<wide_sep, std::wstring::const_iterator, std::wstring> wide_tokenizer;
        wide_sep esp(L"\\", L" ", L"\"'");
        wide_tokenizer tok(options, esp);
        std::vector<char *> argv_v;
        for (wide_tokenizer::iterator i = tok.begin(); i != tok.end(); i++) {
                argv_v.push_back(strdup(SC::S2A(*i).c_str()));
        }
        m_argvp = (char **) malloc(sizeof(char *) * argv_v.size() + 2);
        m_argvp[0] = strdup(SC::S2A(path).c_str());
        int j = 1;
        for (std::vector<char *>::iterator i = argv_v.begin(); i != argv_v.end(); i++, j++) {
                m_argvp[j] = *i;
        }
        m_argvp[j] = NULL;

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

	for (int i = 0; i < m_params_count; i++) {
		ipc->m_read[i] = SZARP_NO_DATA;
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
		ipc->m_read[i] = d;
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

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	IPCHandler     *ipc;
	ExecDaemon *dmn;

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	cfg = new DaemonConfig("execdmn");
	ASSERT(cfg != NULL);

	if (cfg->Load(&argc, argv)) {
		sz_log(0, "Error loading configuration, exiting.");
		return 1;
	}

	dmn = new ExecDaemon(cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetParamsCount());
	ASSERT(dmn != NULL);

	if (dmn->ParseConfig(cfg)) {
		sz_log(0, "Error parsing config, exiting.");
		return 1;
	}

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

#else /* LIBXML_XPATH_ENABLED */

int main(void)
{
	printf("libxml2 XPath support not enabled!\n");
	return 1;
}

#endif /* LIBXML_XPATH_ENABLED */

