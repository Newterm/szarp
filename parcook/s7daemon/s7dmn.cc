
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

int S7Daemon::ParseConfig(DaemonConfig * cfg)
{	
	sz_log(10, "S7Dmn::ParseConfig");

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(m_cfg->GetXMLDoc());
	xmlNodePtr node = m_cfg->GetXMLDevice();
	xp_ctx->node = node;

	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST("ipk"), SC::S2U(IPK_NAMESPACE_STRING).c_str());
	if (-1 == ret) {
		sz_log(0, "Cannot register namespace %s", SC::S2U(IPK_NAMESPACE_STRING).c_str());
		return 1;
	}

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST("extra"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (-1 == ret) {
		sz_log(0, "Cannot register namespace %s", IPKEXTRA_NAMESPACE_STRING);
		return 1;
	}

	_client.ConfigureFromXml(node);

	TUnit* u = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit();
	TParam *p = u->GetFirstParam();
	for (int i = 1; i <= u->GetParamsCount(); i++) {
		char *expr;
		ret = asprintf(&expr, ".//ipk:param[position()=%d]", i);
		if (-1 == ret) {
			sz_log(0, "Cannot allocate XPath expression for param number %d", i);
			return 1;
		}
		xmlNodePtr pnode = uxmlXPathGetNode(BAD_CAST(expr), xp_ctx);
		assert(pnode);
		free(expr);

		if (!_client.ConfigureParamFromXml(i-1, pnode))
			return 1;

		if (!_pmap.ConfigureParamFromXml(i-1, p, pnode))
			return 1;

		p = p->GetNext();
	}

	_client.BuildQueries();

	return 0;
}

int S7Daemon::Read()
{
	sz_log(10, "S7Dmn::Read");

	if( _client.Connect() ) {
		_client.QueryAll();
		_client.DumpAll();
		return 0;
	}
	return 1;
}

void S7Daemon::Transfer()
{
	sz_log(10, "S7Dmn::Transfer");

	_pmap.clearData();

	_client.ProcessData([&](int idx, std::vector<uint8_t> data) {
		_pmap.WriteData(idx, data, [&](int pid, uint16_t value) {
			Set(pid, value);
		});
	});	
		
	/** Go Parcook */
	BaseDaemon::Transfer();
}


int main(int argc, const char *argv[])
{
	S7Daemon dmn;

	if( dmn.Init( argc , argv ) ) {
		sz_log(0,"Cannot start %s daemon", dmn.Name());
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
