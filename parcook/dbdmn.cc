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
 * Daemon for reading data from szbase database.
 * 
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 * 
 * $Id$
 */

/*

 @description_start

 @class 4

 @devices Daemon reads parameters from SZARP base.
 @devices.pl Demon czyta warto¶æ parametrów z bazy danych SZARP.

 @comment Usually the same effect can be obtained using Lua scriptable parameters.
 @comment.pl Zazwyczaj ten sam efekt mo¿na osi±gn±æ za pomoc± parametrów w jêzyku skryptowym Lua.


 @config_example 
 <device 
	xmlns:db="http://www.praterm.com.pl/SZARP/ipk-extra"
  	daemon="/opt/szarp/bin/dbdmn" 
  	path="/opt/szarp/trgr"
		path to szbase main directory
	db:expire="600"
		time (in seconds) of data expiration - if last available data
		is older then given ammount of seconds, NO_DATA is send;
		set 0 to turn expiration off
	<unit id="1" ...>
		<param name="..." 
			db:param="...:...:..."
				name of parameter to read from database
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

#include <assert.h>
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
#include "szbase/szbbase.h"
#include "conversion.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

const int DEFAULT_EXPIRE = 660;	/* 11 minutes */
const int DAEMON_INTERVAL = 10;

/**
 * Modbus communication config.
 */
class DbDaemon {
public :
	/**
	 * @param params number of params to read
	 */
	DbDaemon(DaemonConfig *cfg);
	~DbDaemon();

	/** 
	 * Parses XML 'device' element.
	 * @return - 0 on success, 1 on error 
	 */
	int ParseConfig(DaemonConfig * cfg);

	/**
	 * Read data from database.
	 * @param ipc IPCHandler
	 * @return 0 on success, 1 on error
	 */
	int Read(IPCHandler *ipc);

	/** Wait for next cycle. */
	void Wait();
	
protected :
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckExpire(xmlXPathContextPtr xp_ctx, int dev_num);
	
	int m_single;		/**< are we in 'single' mode */
	public:
	int m_params_count;	/**< size of params array */
	vector<TParam*> m_dbparams;
				/**< names of params to read from db */
	protected:
	int m_expire;		/**< expire time in seconds */
	time_t m_last;		/**< time of last read */
	szb_buffer_t* m_szb;	/**< pointer to szbase buffer */
	TSzarpConfig* m_target_ipk;
				/**< IPK object for database configuration */
};

/**
 * @param params number of params to read
 * @param sends number of params to send (write)
 */
DbDaemon::DbDaemon(DaemonConfig* cfg) 
{
	m_params_count = cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetParamsCount();
	assert(m_params_count >= 0);
	m_expire = DEFAULT_EXPIRE;
	m_single = 0;
	m_last = 0;
	m_szb = NULL;
	m_target_ipk = NULL;
}

DbDaemon::~DbDaemon() 
{
	if (m_szb != NULL) {
		szb_free_buffer(m_szb);
		m_szb = NULL;
	}
	if (m_target_ipk != NULL) {
		delete m_target_ipk;
		m_target_ipk = NULL;
	}
}


int DbDaemon::XMLCheckExpire(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;
	long l;

	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@db:expire",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL)
		return 0;
	l = strtol((char *)c, &e, 0);
	if ((*c == 0) || (*e != 0)) {
		sz_log(0, "incorrect value '%s' for db:expire, number expected",
				(char *)c);
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	if (l < 0) {
		sz_log(0, "value '%ld' for db:expire must be non-negative", l);
		return 1;
	}
	m_expire = (int)l;
	sz_log(2, "using expire frequency %d", m_expire);
	return 0;
}

int DbDaemon::ParseConfig(DaemonConfig * cfg)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xp_ctx;
	int dev_num;
	int ret;
	char *e;
	
	/* get config data */
	assert (cfg != NULL);
	dev_num = cfg->GetLineNumber();
	assert (dev_num > 0);
	doc = cfg->GetXMLDoc();
	assert (doc != NULL);

	/* prepare xpath */
	xp_ctx = xmlXPathNewContext(doc);
	assert (xp_ctx != NULL);

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "db",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);

	XMLCheckExpire(xp_ctx, dev_num);

	TSzarpConfig* m_target_ipk = new TSzarpConfig();
	asprintf(&e, "%s/config/%s", cfg->GetDevicePath(), "params.xml");
	assert (e != NULL);
	if (m_target_ipk->loadXML(SC::A2S(e))) {
		sz_log(0, "Error loading file '%s'", e);
		free(e);
		return 1;
	}
	free(e);

	for (int i = 0; i < m_params_count; i++) {
		xmlChar *c;
		asprintf(&e, "/ipk:params/ipk:device[position()=%d]/ipk:unit[position()=1]/ipk:param[position()=%d]/@db:param",
				dev_num, i + 1);
		assert (e != NULL);
		c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
		free(e);
		if (c == NULL) {
			sz_log(0, "No db:param attribute for device %d, parameter %d",
					dev_num, i + 1);
			return 1;
		}
		TParam *p = m_target_ipk->getParamByName(SC::U2S(c));
		if (p == NULL) {
			sz_log(0, "Param with name '%s' not found for device %d, parameter %d",
					c, dev_num, i + 1);
			return 1;
		}
		sz_log(3, "Using database parameter '%s' for device %d, parameter %d",
				c, dev_num, i + 1);
		m_dbparams.push_back(p);
	}
	xmlXPathFreeContext(xp_ctx);
	
	m_single = cfg->GetSingle();

	IPKContainer::Init(SC::A2S(PREFIX), SC::A2S(PREFIX), L"");
	Szbase::Init(SC::A2S(PREFIX), NULL);

	asprintf(&e, "%s/szbase", cfg->GetDevicePath());
	assert (e != NULL);
	m_szb = szb_create_buffer(Szbase::GetObject(), SC::A2S(e), m_params_count, m_target_ipk);
	free(e);
	
	return 0;
}


void DbDaemon::Wait() 
{
	time_t t;
	t = time(NULL);
	
	if (t - m_last < DAEMON_INTERVAL) {
		int i = DAEMON_INTERVAL - (t - m_last);
		while (i > 0) {
			i = sleep(i);
		}
	}
	m_last = time(NULL);
	return;
}

int DbDaemon::Read(IPCHandler *ipc)
{
	for (int i = 0; i < m_params_count; i++) {
		ipc->m_read[i] = SZARP_NO_DATA;
	}

	for (size_t i = 0; i < m_dbparams.size(); i ++) {
		sz_log(10, "Searching for data for parameter %zd (%ls)",
			i + 1, m_dbparams[i]->GetName().c_str());
		time_t t = szb_search_last(m_szb, m_dbparams[i]);
		if (t == -1) {
			sz_log(10, "No data found for parameter");
			continue;
		}
		t = szb_search(m_szb, m_dbparams[i], t, 
			m_last - m_expire, -1);
		if (t == -1) {
			sz_log(10, "Data is too old");
			continue;
		}
		SZBASE_TYPE data = szb_get_data(m_szb, m_dbparams[i], t);
		if (data == SZB_NODATA) {
			sz_log(10, "Value is NO_DATA");
			continue;
		}
		sz_log(10, "Setting data to %d",
			m_dbparams[i]->ToIPCValue(data));
		ipc->m_read[i] = m_dbparams[i]->ToIPCValue(data); 
	}
	
	return 0;
}

RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d cought", signum);
	exit(1);
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
	assert (ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	assert (ret == 0);
}

int main(int argc, char *argv[])
{
	DaemonConfig	*cfg;
	IPCHandler	*ipc;
	DbDaemon	*dmn;

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	cfg = new DaemonConfig("dbdmn");
	assert (cfg != NULL);
	
	if (cfg->Load(&argc, argv))
		return 1;
	
	dmn = new DbDaemon(cfg);
	assert (dmn != NULL);
	
	if (dmn->ParseConfig(cfg)) {
		return 1;
	}

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}
	cfg->CloseXML(1);

	init_signals();

	sz_log(2, "Starting DbDaemon");

	while (true) {
		dmn->Wait();
		Szbase::GetObject()->NextQuery();
		dmn->Read(ipc);
		/* send data from parcook segment */
		ipc->GoParcook();
	}
	return 0;
}

