
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/lexical_cast.hpp>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <map>
#include <set>
#include <cstdint>
#include <algorithm>

#include "conversion.h"
#include "liblog.h"
#include "xmlutils.h"

#include "../base_daemon.h"

#include "s7dmn.h"
#include "datatypes.h"

int S7Daemon::ParseConfig(DaemonConfig * cfg)
{	
	sz_log(5, "S7Dmn::ParseConfig");
	
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(m_cfg->GetXMLDoc());
	xmlNodePtr node = m_cfg->GetXMLDevice();
	xp_ctx->node = node;

	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST("ipk"), SC::S2U(IPK_NAMESPACE_STRING).c_str());
	if (-1 == ret) {
		sz_log(1, "Cannot register namespace %s", SC::S2U(IPK_NAMESPACE_STRING).c_str());
		return 1;
	}

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST("extra"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (-1 == ret) {
		sz_log(1, "Cannot register namespace %s", IPKEXTRA_NAMESPACE_STRING);
		return 1;
	}

	_client.ConfigureFromXml(node);

	TUnit* u = cfg->GetDevice()->GetFirstUnit();
	TParam *p = u->GetFirstParam();
	for (int i = 1; i <= u->GetParamsCount(); i++) {
		char *expr;
		ret = asprintf(&expr, ".//ipk:param[position()=%d]", i);
		if (-1 == ret) {
			sz_log(1, "Cannot allocate XPath expression for param number %d", i);
			return 1;
		}
		xmlNodePtr pnode = uxmlXPathGetNode(BAD_CAST(expr), xp_ctx, false);
		assert(pnode);
		free(expr);

		if (!_client.ConfigureParamFromXml(i-1, p, pnode))
			return 1;

		if (!_pmap.ConfigureParamFromXml(i-1, p, pnode))
			return 1;

		p = p->GetNext();
	}
	
	TSendParam *s = u->GetFirstSendParam();
	for (int i = 1; i <= u->GetSendParamsCount(); i++) {
		char *expr;
		ret = asprintf(&expr, ".//ipk:send[position()=%d]", i);
		if (-1 == ret) {
			sz_log(1, "Cannot allocate XPath expression for param number %d", i);
			return 1;
		}
		xmlNodePtr pnode = uxmlXPathGetNode(BAD_CAST(expr), xp_ctx, false);
		assert(pnode);
		free(expr);

		if (!_client.ConfigureParamFromXml(u->GetParamsCount()+i-1, s, pnode))
			return 1;

		if (!_pmap.ConfigureParamFromXml(u->GetParamsCount()+i-1, s, pnode))
			return 1;

		s = s->GetNext();
	}

	/** Sort and merge all available read/write queries */
	_client.BuildQueries();

	return 0;
}

int S7Daemon::Read()
{
	sz_log(5, "S7Dmn::Read");

	/* Fill read queries with data from PLC */
	if( _client.Connect() ) {
		_client.AskAll();
		return 0;
	}
	return 1;
}

void S7Daemon::Transfer()
{
	sz_log(5, "S7Dmn::Transfer");

	/** Write SZARP DB with values from read queries */ 
	_pmap.clearWriteBuffer();
	_client.ProcessResponse([&](unsigned long int idx, DataBuffer data, bool no_data) {
		if(no_data) { Set(idx, SZARP_NO_DATA); return; }

		_pmap.WriteData(idx, data, [&](unsigned long int pid, int16_t value) {
			Set(pid, value);
		});
	});	
	
	/** 
	 * Assume all write queries have data.
	 * If part of merged query will lack data, whole query will not be send to device.
	 */
	_client.ClearWriteNoDataFlags();

	/** Read SZARP DB and write values to write queries */
	_pmap.clearReadBuffer();
	_client.AccessData([&](unsigned long int idx, DataDescriptor desc) {
		SzInteger send_value = SZARP_NO_DATA;
		return _pmap.ReadData(idx, desc, [&](unsigned long int pid) {
			send_value = Send(pid - Count());
			return send_value;
		});
	});

	/** Send write queries to PLC */
	if( _client.Connect() ) _client.TellAll();

	if(isDumpHex()) _client.DumpAll();

	/** Go Parcook */
	BaseDaemon::Transfer();
}


int main(int argc, const char *argv[])
{
	S7Daemon dmn;

	for (int i = 1; i < argc; i++) {
		if (std::string(argv[i]) == "--dumphex")
			dmn.setDumpHex(true);
	}

	if( dmn.Init( argc , argv ) ) {
		sz_log(1,"Cannot start %s daemon", dmn.Name());
		return 1;
	}

	sz_log(1, "Initialization done, Starting...");

	for(;;)
	{
		dmn.Read();
		dmn.Transfer();
		dmn.Wait();
	}

	return 0;
}
