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
  	daemon="/opt/szarp/bin/dbdmn" 
	extra:expire="600"
		time (in seconds) of data expiration - if last available data
		is older than given amount of seconds, NO_DATA is send;
		set 0 to turn expiration off
	<unit id="1" ...>
		<param name="..." 
			db:param="database_name:...:...:..."
				name of parameter to read from database, preceded by database name
			extra:probe-type="..."
				type of database value: PT_MIN10 - 10 min probes, PT_SEC10 - 10 sec probes
			prec="..."
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
#include "libpar.h"
#include "xmlutils.h"
#include "szbase/szbbase.h"
#include "conversion.h"
#include "custom_assert.h"

#include "base_daemon.h"

#include <string>
#include <vector>

struct ParamDesc {
    IPCParamInfo *param; // target param
    std::wstring dbparam; // param to get
    SZARP_PROBE_TYPE probe_type; // type of probe to get

	int16_t toIPCValue(SZBASE_TYPE value)
	{
		for (int i = 0; i < param->GetPrec(); i++) {
			value *= 10;
		}
		return (int16_t) nearbyint(value);
	}
};

/**
 * Modbus communication config.
 */
class DbDaemon {
public :
	/**
	 * @param params number of params to read
	 */
	DbDaemon(BaseDaemon& base_dmn);
	~DbDaemon();

	void ParseConfig(const DaemonConfigInfo &dmn);

	void Read(BaseDaemon& base_dmn);

protected :
	SZARP_PROBE_TYPE GetProbeType(IPCParamInfo *param);
	void ConfigureParam(IPCParamInfo *param_node, Szbase *szbase);
	int ConfigureProbers();

public:
	int m_params_count;	/**< size of params array */
	std::vector<struct ParamDesc*> m_dbparams;
				/**< names of params to read from db */
protected:
	int m_expire;		/**< expire time in seconds */
	time_t m_last;		/**< time of last read */
	std::map<std::string, SZARP_PROBE_TYPE> m_probe_type_map;
};

DbDaemon::DbDaemon(BaseDaemon& base_dmn)
{
	auto& cfg = base_dmn.getDaemonCfg();
	m_params_count = cfg.GetParamsCount();
	ASSERT(m_params_count >= 0);
	m_expire = DEFAULT_EXPIRE;
	m_last = 0;
	
	m_probe_type_map["PT_SEC10"] = PT_SEC10;
	m_probe_type_map["PT_MIN10"] = PT_MIN10;
	m_probe_type_map["PT_HOUR"] = PT_HOUR;
	m_probe_type_map["PT_HOUR8"] = PT_HOUR8;
	m_probe_type_map["PT_DAY"] = PT_DAY;
	m_probe_type_map["PT_WEEK"] = PT_WEEK;
	m_probe_type_map["PT_MONTH"] = PT_MONTH;
	m_probe_type_map["PT_YEAR"] = PT_YEAR;

	ParseConfig(base_dmn.getDaemonCfg());

	base_dmn.setCycleHandler([this](BaseDaemon& base_dmn){ Read(base_dmn); });
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

SZARP_PROBE_TYPE DbDaemon::GetProbeType(IPCParamInfo *param)
{
	auto prop = param->getAttribute<std::string>("extra:probe-type", "PT_MIN10");

	std::map<std::string, SZARP_PROBE_TYPE>::iterator it = m_probe_type_map.find(prop);
	if (it != m_probe_type_map.end())
		return it->second;

	sz_log(2, "Unsupported probe-type value in param %s, using default PT_MIN10", SC::S2U(param->GetName()).c_str());
	return PT_MIN10;
}

void DbDaemon::ConfigureParam(IPCParamInfo* param, Szbase *szbase) {
	auto db_param = param->getOptAttribute<std::string>("extra:param");
	if (!db_param) {
		sz_log(1, "No extra:param attribute for parameter %s", SC::S2U(param->GetName()).c_str());
		return;
	}

	struct ParamDesc * pd = new ParamDesc();
	pd->param = param;
	pd->dbparam = SC::L2S(*db_param);

	std::pair<szb_buffer_t*, TParam*> bp;
	if (!szbase->FindParam(pd->dbparam, bp)) {
		std::string err_string = "Could not configure param ";
		err_string += SC::S2L(param->GetName());
		err_string += ": extra:param ";
		err_string += SC::S2L(pd->dbparam);
		err_string += " not found in base";
		throw std::runtime_error(err_string);
	}

	pd->probe_type = GetProbeType(param);

	sz_log(3, "Using database parameter '%s' for parameter %s",
	       	SC::S2U(pd->dbparam).c_str(), SC::S2U(param->GetName()).c_str());
	m_dbparams.push_back(pd);
}

void DbDaemon::ParseConfig(const DaemonConfigInfo &dmn)
{
	auto device = dmn.GetDeviceInfo();
	m_expire = device->getAttribute<int>("extra:expire", 0);
	if (m_expire < 0)
		throw std::runtime_error("extra:expire cannot be negative");

	ParamCachingIPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"");
	Szbase::Init(SC::L2S(PREFIX));

	Szbase* szbase = Szbase::GetObject();
	szbase->NextQuery();

	for (auto unit: dmn.GetUnits()) {
		for (auto param: unit->GetParams()) {
			ConfigureParam(param, szbase);
		}
	}

	ConfigureProbers();
}

void DbDaemon::Read(BaseDaemon& base_dmn)
{
	m_last = time(NULL);

	sz_log(10, "DbDaemon::Read: Setting NO_DATA");
	for (int i = 0; i < m_params_count; i++) {
		base_dmn.getIpc().setRead<int16_t>(i, SZARP_NO_DATA);
	}

	Szbase* szbase = Szbase::GetObject();
	szbase->NextQuery();

	sz_log(10, "DbDaemon::Read: Querying: m_last: %ld (%s), m_expire: %d", m_last, asctime(localtime(&m_last)), m_expire);
	for (size_t i = 0; i < m_dbparams.size(); i++) {
		struct ParamDesc * pd = m_dbparams[i];

		time_t first_time = m_last - m_expire;
		std::string first_time_str = asctime(localtime(&first_time));
		std::string last_time_str = asctime(localtime(&m_last));
		sz_log(10, "Searching for data for parameter (%s) on interval %ld (%s), %ld (%s)",
			SC::S2L(pd->dbparam).c_str(),
			first_time,
			first_time_str.c_str(),
			m_last,
			last_time_str.c_str());

		bool ok;
		std::wstring error;
		time_t t = szbase->Search(pd->dbparam, m_last, m_last - m_expire, -1, pd->probe_type, ok, error);
		if (!ok) {
			sz_log(3, "No data found for parameter %s, error: %s",
				SC::S2U(pd->dbparam).c_str(), SC::S2U(error).c_str());
			continue;
		}

		sz_log(10, "DbDaemon::Read: time found: %ld, %s", t, asctime(localtime(&t)));

		bool is_fixed;
		SZBASE_TYPE data = szbase->GetValue(pd->dbparam, t, pd->probe_type, 0, &is_fixed, ok, error);
	
		if (!ok || IS_SZB_NODATA(data)) {
			sz_log(10, "Value is NO_DATA: %s", SC::S2U(error).c_str());
			continue;
		}

		base_dmn.getIpc().setRead<int16_t>(i, pd->toIPCValue(data));
	}

	base_dmn.getIpc().publish();
}

int main(int argc, const char *argv[])
{
	BaseDaemonFactory::Go<DbDaemon>("dbdmn", argc, argv);
	return 0;
}

