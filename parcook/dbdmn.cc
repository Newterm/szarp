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
	db:expire="600"
		time (in seconds) of data expiration - if last available data
		is older than given amount of seconds, NO_DATA is send;
		set 0 to turn expiration off
	<unit id="1" ...>
		<param name="..." 
			db:param="database_name:...:...:..."
				name of parameter to read from database, preceded by database name
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
#include "libpar.h"
#include "xmlutils.h"
#include "szbase/szbbase.h"
#include "conversion.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

const int DEFAULT_EXPIRE = 660;	/* 11 minutes */
const int DAEMON_INTERVAL = 10;

struct ParamDesc {
    TParam *param; // target param
    std::wstring dbparam; // param to get
    SZARP_PROBE_TYPE probe_type; // type of probe to get
};

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
	int XMLCheckExpire(xmlNodePtr device_node);

	SZARP_PROBE_TYPE GetProbeType(TParam * param, xmlNodePtr param_node);
	struct ParamDesc * ConfigureParam(TParam * param, xmlNodePtr param_node, Szbase * szbase);
	int ConfigureProbers();

	int m_single;		/**< are we in 'single' mode */
public:
	int m_params_count;	/**< size of params array */
	std::vector<struct ParamDesc *> m_dbparams;
				/**< names of params to read from db */
protected:
	int m_expire;		/**< expire time in seconds */
	time_t m_last;		/**< time of last read */
	TSzarpConfig* m_target_ipk;
				/**< IPK object for database configuration */
	std::map<std::string, SZARP_PROBE_TYPE> m_probe_type_map;
};

/**
 * @param params number of params to read
 * @param sends number of params to send (write)
 */
DbDaemon::DbDaemon(DaemonConfig* cfg) 
{
	m_single = cfg->GetSingle();
	m_params_count = cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetParamsCount();
	assert(m_params_count >= 0);
	m_expire = DEFAULT_EXPIRE;
	m_last = 0;
	m_target_ipk = NULL;
	
	m_probe_type_map["PT_SEC10"] = PT_SEC10;
	m_probe_type_map["PT_MIN10"] = PT_MIN10;
	m_probe_type_map["PT_HOUR"] = PT_HOUR;
	m_probe_type_map["PT_HOUR8"] = PT_HOUR8;
	m_probe_type_map["PT_DAY"] = PT_DAY;
	m_probe_type_map["PT_WEEK"] = PT_WEEK;
	m_probe_type_map["PT_MONTH"] = PT_MONTH;
	m_probe_type_map["PT_YEAR"] = PT_YEAR;
}

DbDaemon::~DbDaemon() 
{
    for (size_t i = 0; i < m_dbparams.size(); i++) {
	    delete m_dbparams[i];
    }
}


int DbDaemon::ConfigureProbers()
{
	char *_servers = libpar_getpar("available_probes_servers", "servers", 0);
	if (_servers == NULL)
		return 0;

	Szbase* szbase = Szbase::GetObject();

	std::stringstream ss(_servers);
	std::string section;
	while (ss >> section) {
		char *_prefix = libpar_getpar(section.c_str(), "prefix", 0);
		char *_address = libpar_getpar(section.c_str(), "address", 0);
		char *_port = libpar_getpar(section.c_str(), "port", 0);
		if (_prefix != NULL && _address != NULL && _port  != NULL) {
			std::wstring address = SC::L2S(_address);
			std::wstring port = SC::L2S(_port);
			std::wstring prefix = SC::L2S(_prefix);

			szbase->SetProberAddress(prefix, address, port);
		}
		std::free(_prefix);
		std::free(_address);
		std::free(_port);
	}
	std::free(_servers);

	return 0;
}


int DbDaemon::XMLCheckExpire(xmlNodePtr device_node)
{
	xmlChar* c = xmlGetNsProp(device_node, BAD_CAST "expire", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (c == NULL)
		return 0;

	char *e;
	long l = strtol((char *)c, &e, 0);
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

SZARP_PROBE_TYPE DbDaemon::GetProbeType(TParam * param, xmlNodePtr param_node)
{
	xmlChar* prop = xmlGetNsProp(param_node, BAD_CAST "probe-type", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (NULL == prop) {
		sz_log(2, "No extra:probe-type attribute for parameter %s, using default PT_MIN10",
		       	SC::S2U(param->GetName()).c_str());
		return PT_MIN10;
	}

	std::string pt;
	std::stringstream ss((char*)prop);
	bool ok = ss >> pt;
	if (!ok && !ss.eof()) {
		sz_log(1, "Invalid value %s for attribute extra:probe-type in line, %ld, using default PT_MIN10",
		       	(char*)prop,  xmlGetLineNo(param_node));
		xmlFree(prop);	
		return PT_MIN10;
	}
	xmlFree(prop);

	std::map<std::string, SZARP_PROBE_TYPE>::iterator it = m_probe_type_map.find(pt);
	if (it != m_probe_type_map.end())
		return it->second;


	sz_log(2, "Unsupported probe-type value, using default PT_MIN10");
	return PT_MIN10;
}

struct ParamDesc * DbDaemon::ConfigureParam(TParam * param, xmlNodePtr param_node, Szbase* szbase)
{
	xmlChar* dbparam = xmlGetNsProp(param_node, BAD_CAST "param", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (dbparam == NULL) {
		sz_log(0, "No extra:param attribute for parameter %s",
		       	SC::S2U(param->GetName()).c_str());
		return NULL;
	}

	struct ParamDesc * pd = new ParamDesc();
	pd->param = param;

	pd->dbparam = SC::U2S(dbparam);
	xmlFree(dbparam);

	std::pair<szb_buffer_t*, TParam*> bp;
	if (!szbase->FindParam(pd->dbparam, bp)) {
		sz_log(0, "Param with name '%s' not found for parameter %s", 
			SC::S2U(pd->dbparam).c_str(), SC::S2U(param->GetName()).c_str());
		delete pd;
		return NULL;
	}

	pd->probe_type = GetProbeType(param, param_node);

	sz_log(3, "Using database parameter '%s' for parameter %s",
	       	SC::S2U(pd->dbparam).c_str(), SC::S2U(param->GetName()).c_str());
	return pd;
}

int DbDaemon::ParseConfig(DaemonConfig * cfg)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xp_ctx;
	int ret;
	
	/* get config data */
	assert (cfg != NULL);
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

	xp_ctx->node = cfg->GetXMLDevice();

	if (XMLCheckExpire(xp_ctx->node)) {
		xmlXPathFreeContext(xp_ctx);
		sz_log(0, "Configuration error (expire attribute), exiting");
		return 1;
	}

	IPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"");
	Szbase::Init(SC::L2S(PREFIX), NULL);

	Szbase* szbase = Szbase::GetObject();
	szbase->NextQuery();


	TParam *p = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetFirstParam();
	for (int i = 1; NULL != p && i <= m_params_count; i++) {
		char *expr;
		asprintf(&expr, ".//ipk:unit[position()=1]/ipk:param[position()=%d]", i);
		assert (expr != NULL);

		xmlNodePtr node = uxmlXPathGetNode(BAD_CAST expr, xp_ctx);
		assert(node);
		free(expr);

		struct ParamDesc * pd = ConfigureParam(p, node, szbase);
		if (NULL == pd) {
			sz_log(0, "Param configuration error for %s, exiting",
			    SC::S2U(p->GetName()).c_str());
			return 1;
		}

		m_dbparams.push_back(pd);
		p = p->GetNext();
	}

	xmlXPathFreeContext(xp_ctx);

	IPKContainer::Init(SC::A2S(PREFIX), SC::A2S(PREFIX), L"");
	Szbase::Init(SC::A2S(PREFIX), NULL);
	ConfigureProbers();

	return 0;
}


void DbDaemon::Wait() 
{
	time_t t;
	t = time(NULL);
	
	if (t - m_last < DAEMON_INTERVAL) {
		int i = DAEMON_INTERVAL - (t - m_last);
		sz_log(10, "DbDaemon::Wait: sec to wait: %d", i);
		while (i > 0) {
			i = sleep(i);
		}
	}
	m_last = time(NULL);
	return;
}

int DbDaemon::Read(IPCHandler *ipc)
{
	sz_log(10, "DbDaemon::Read: Setting NO_DATA");
	for (int i = 0; i < m_params_count; i++) {
		ipc->m_read[i] = SZARP_NO_DATA;
	}

	Szbase* szbase = Szbase::GetObject();
	szbase->NextQuery();

	sz_log(10, "DbDaemon::Read: Querying: m_last: %ld, m_expire: %d", m_last, m_expire);
	for (size_t i = 0; i < m_dbparams.size(); i++) {
		struct ParamDesc * pd = m_dbparams[i];

		sz_log(10, "Searching for data for parameter (%s)",
			SC::S2U(pd->param->GetName()).c_str());

		bool ok;
		std::wstring error;
		time_t t = szbase->Search(pd->dbparam, m_last, m_last - m_expire, -1, pd->probe_type, ok, error);
		if (!ok) {
			sz_log(3, "No data found for parameter %s, error: %s",
				SC::S2U(pd->dbparam).c_str(), SC::S2U(error).c_str());
			continue;
		}

		sz_log(10, "DbDaemon::Read: time found: %ld", t);

		bool is_fixed;
		SZBASE_TYPE data = szbase->GetValue(pd->dbparam, t, pd->probe_type, 0, &is_fixed, ok, error);
	
		if (!ok || IS_SZB_NODATA(data)) {
			sz_log(10, "Value is NO_DATA: %s", SC::S2U(error).c_str());
			continue;
		}

		ipc->m_read[i] = pd->param->ToIPCValue(data);
		sz_log(10, "Data set to %d", ipc->m_read[i]);
	}
	
	return 0;
}

RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d caught", signum);
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
	
	if (cfg->Load(&argc, argv, 0)) // 0 - dont call libpar_done
		return 1;
	
	dmn = new DbDaemon(cfg);
	assert (dmn != NULL);
	
	if (dmn->ParseConfig(cfg)) {
		return 1;
	}
	libpar_done();

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
		dmn->Read(ipc);
		/* send data from parcook segment */
		ipc->GoParcook();
	}
	return 0;
}

