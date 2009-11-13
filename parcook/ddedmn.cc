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
/** Daemon communicating DDE proxy located at windows server*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef NO_XMLRPC_EPI

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <typeinfo>

#include <xmlrpc-epi/xmlrpc.h>

#include <stdarg.h>

#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "liblog.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "tokens.h"
#include "xmlutils.h"
#include "httpcl.h"
#include "conversion.h"

bool g_single;

template<typename T> int get_bit(T val, unsigned i) {
	return ((val & (1 << i))) != 0 ? 1 : 0;
}

void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	char *l;
	va_start(fmt_args, fmt);

	if (g_single) {
		vasprintf(&l, fmt, fmt_args);
		std::cout << l << std::endl;
		sz_log(level, "%s", l);
		free(l);
	} else
		vsz_log(level, "%s", fmt_args);

	va_end(fmt_args);
} 


struct DDEParam {
	enum TYPE {
		FLOAT, 
		SHORT_FLOAT,
		INTEGER,
		NONE
	} type;
	int prec;
	int address;
	DDEParam() {
		type = NONE;
		prec = 0;
		address = -1;
	}
};

template<typename T> T mypow(T val, int e) {
	if (e > 0) 
		while (e--)
			val *= 10;
	else
		while (e++)
			val /= 10;

	return val;
}

class DDEDaemon { 
	szHTTPCurlClient m_http;
	IPCHandler *m_ipc;
	std::string m_uri;
	int m_read_freq;

	std::map<std::string, std::map<std::string, DDEParam> > m_params;

	XMLRPC_REQUEST CreateRequest();
	XMLRPC_REQUEST SendRequest(XMLRPC_REQUEST request);
	void ParseResponse(XMLRPC_REQUEST response);
	bool ConvertValue(XMLRPC_VALUE v, const std::string& topic, std::string item, unsigned short &ret);
public:
	int Configure(DaemonConfig* cfg);
	void Run();
};

int DDEDaemon::Configure(DaemonConfig *cfg) {
	m_ipc = new IPCHandler(cfg);
	if (m_ipc->Init()) {
		dolog(0, "Intialization of IPCHandler failed");
		return 1;
	}

	xmlNodePtr xdev = cfg->GetXMLDevice();

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(xdev->doc);
	xp_ctx->node = xdev;

	int ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);

	xmlChar* _uri = xmlGetNsProp(xdev, BAD_CAST("uri"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_uri== 0) {
		dolog(0, "Error, attribute dde:uri missing in device definition, line(%ld)", xmlGetLineNo(xdev));
		return 1;
	}
	m_uri = (char*) _uri;
	xmlFree(_uri);

	m_read_freq = 0;
	xmlChar* _read_req = xmlGetNsProp(xdev, BAD_CAST("read_freq"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_read_req) {
		m_read_freq = atoi((char*) _read_req);
		xmlFree(_read_req);
	}
	if (m_read_freq < 10)
		m_read_freq = 10;

	TParam *param = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetFirstParam();
	for (int i = 0; i < m_ipc->m_params_count; ++i) {
		char *e;
		asprintf(&e, "/ipk:params/ipk:device[position()=%d]/ipk:unit[position()=1]/ipk:param[position()=%d]",
			cfg->GetLineNumber(), 
			i + 1);
		assert (e != NULL);

		xmlNodePtr n = uxmlXPathGetNode(BAD_CAST e, xp_ctx);
		assert(n != NULL);
		free(e);

		xmlChar* _topic = xmlGetNsProp(n,
				BAD_CAST("topic"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

		if (_topic == 0) {
			dolog(0, "Error, attribute dde:address missing in param definition, line(%ld)", xmlGetLineNo(n));
			return 1;
		}

		xmlChar* _item = xmlGetNsProp(n, 
				BAD_CAST("item"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
			
		if (_item == 0) {
			dolog(0, "Error, attribute dde:item missing in param definition, line(%ld)", xmlGetLineNo(n));
			return 1;
		}

		char *err;
		int item = strtol((const char*)_item, &err, 10);
		if (*err != '\0') {
			xmlFree(_item);
			dolog(0, "Error, invalid value of attribute dde:item in param definition, line(%ld)", xmlGetLineNo(n));
			return 1;
		}

		xmlChar* _type = xmlGetNsProp(n,
				BAD_CAST("type"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_type == NULL) {
			dolog(0, "Error, missing parameter type specification (dde:type) in line(%ld)", xmlGetLineNo(n));
			return 1;
		}

		DDEParam::TYPE val_type;
		if (!xmlStrcmp(_type, BAD_CAST "float"))
			val_type = DDEParam::FLOAT;
		else if (!xmlStrcmp(_type, BAD_CAST "integer"))
			val_type = DDEParam::INTEGER;
		else if (!xmlStrcmp(_type, BAD_CAST "short_float"))
			val_type = DDEParam::SHORT_FLOAT;
		else {
			dolog(0, "Error, invalid parameter type specification (dde:type) in line(%ld), only 'integer' and 'float' supported", xmlGetLineNo(n));
			xmlFree(_type);
			return 1;
		}
		xmlFree(_type);

		std::string topic = (char*) _topic;

		if (m_params.find(topic) == m_params.end()
				|| m_params[topic].find(item) == m_params[topic].end()) {
			DDEParam& mp = m_params[topic][item];
			mp.prec = param->GetPrec();
			mp.address = i;
			mp.type = val_type;
		}

		param = param->GetNext();
	}

	xmlXPathFreeContext(xp_ctx);

	return 0;
}

XMLRPC_REQUEST DDEDaemon::CreateRequest() {
	XMLRPC_REQUEST request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(request, "get_values");
	XMLRPC_RequestSetRequestType(request, xmlrpc_request_call);

	XMLRPC_VALUE v = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);

	XMLRPC_RequestSetData(request, XMLRPC_CreateVector(NULL, xmlrpc_vector_array));
	XMLRPC_AddValueToVector(XMLRPC_RequestGetData(request), v);

	for (std::map<std::string, std::map<std::string, DDEParam> >::iterator i = m_params.begin();
			i != m_params.end();
			i++) {
		XMLRPC_VALUE pv = XMLRPC_CreateVector(i->first.c_str(), xmlrpc_vector_array);
		for (std::map<int, DDEParam>::iterator j = i->second.begin();
				j != i->second.end();
				++j) {
			XMLRPC_VectorAppendString(pv, NULL, j->first.c_str());
			if (j->second.type != DDEParam::INTEGER) {
				XMLRPC_VectorAppendInt(pv, NULL, atoi(j->first.c_str()) + 1);
			}
		}

		XMLRPC_AddValueToVector(v, pv);
	}
 
	return request;
   
}

XMLRPC_REQUEST DDEDaemon::SendRequest(XMLRPC_REQUEST request) {
	int slen;
	size_t rlen;
	char *request_string = XMLRPC_REQUEST_ToXML(request, &slen);
	if (!request_string) 
		return NULL;

	char* response_string = m_http.Post(const_cast<char*>(m_uri.c_str()), request_string, slen, m_read_freq, NULL, &rlen);
	free(request_string);

	if (response_string == NULL) {
		dolog(0, "Failed to fetch data from %s", m_http.GetErrorStr());
		return NULL;	
	}

	XMLRPC_REQUEST response = XMLRPC_REQUEST_FromXML(response_string, rlen, NULL);
	free(response_string);

	return response;
}

bool DDEDaemon::ConvertValue(XMLRPC_VALUE i, const std::string& topic, std::string item, unsigned short &ret) {
	const char *s;
	char *err;
	switch (XMLRPC_GetValueType(i)) {
		case xmlrpc_int:
			ret = XMLRPC_GetValueInt(i);
			break;
		case xmlrpc_double:
			ret = XMLRPC_GetValueDouble(i);
			break;
		case xmlrpc_string:
			s = XMLRPC_GetValueString(i);
			if (s == NULL)	{
				dolog(1, "No value received for topic: %s, item: %s", topic.c_str(), item);
				return false;;
			}
			ret = strtof(s, &err);
			if (*err != '\0') {
				dolog(1, "Invalid value received for topic: %s, item: %s - %s", topic.c_str(), item.c_str(), s);
				return false;
			}
			break;
		default:
			dolog(0, "Unsupported value value received for topic: %s, item: %s - %d", topic.c_str(), item.c_str(), XMLRPC_GetValueType(i));
			return false;
	}

	return true;


}

void DDEDaemon::ParseResponse(XMLRPC_REQUEST response) {
	if (response == NULL)
		return;

	if (XMLRPC_ResponseIsFault(response)) {
		dolog(1, "Failed to fetch data: %s", XMLRPC_GetResponseFaultString(response));
		return;
	}

	XMLRPC_VALUE i = XMLRPC_VectorRewind(XMLRPC_RequestGetData(response));
	std::map<std::string, std::map<int, DDEParam> >::iterator j;
	std::map<int, DDEParam>::iterator k;

	for (j = m_params.begin(); j != m_params.end() && i; j++)
		for (k = j->second.begin(); k != j->second.end() && i; ++k) {
			DDEParam& dde = k->second;

			unsigned short val = SZARP_NO_DATA;
			bool r = ConvertValue(i, j->first, k->first, val);
			i = XMLRPC_VectorNext(XMLRPC_RequestGetData(response));
			if (dde.type == DDEParam::INTEGER) {
				m_ipc->m_read[dde.address] = val;
			} else if (dde.type == DDEParam::FLOAT
					|| dde.type == DDEParam::SHORT_FLOAT) {
				if (i == NULL) {
					dolog(0, "Not enought params in repsonse from spy!", 0);
					break;
				}
				unsigned short val2;
				if (r && ConvertValue(i, j->first, k->first, val2)) {
					float f;
					memcpy(&f, &val, sizeof(val));
					memcpy((char*)(&f) + 2, &val2, sizeof(val2));
					int v = (int)mypow(f, dde.prec);
					if (dde.type == DDEParam::FLOAT)
						m_ipc->m_read[dde.address + 1] = v >> 16;
					m_ipc->m_read[dde.address] = v & 0xffff;
				}
				i = XMLRPC_VectorNext(XMLRPC_RequestGetData(response));
			} else {
				assert(false);
			}
		}

}

void DDEDaemon::Run() {

	while (true) {
		time_t st = time(NULL);

		dolog(6, "Beginning parametr fetching loop");

		for (int i = 0; i < m_ipc->m_params_count; ++i) 
			m_ipc->m_read[i] = SZARP_NO_DATA;

		XMLRPC_REQUEST request = CreateRequest();
		XMLRPC_REQUEST response = SendRequest(request);
		ParseResponse(response);
		XMLRPC_RequestFree(request, 1);
		XMLRPC_RequestFree(response, 1);

		m_ipc->GoParcook();

		int s = st + m_read_freq - time(NULL);
		while (s > 0) 
			s = sleep(s);

		dolog(6, "End of of loop");
	}
}

int main(int argc, char *argv[]) {
	DaemonConfig* cfg = new DaemonConfig("ddedmn");

	if (cfg->Load(&argc, argv))
		return 1;

	g_single = cfg->GetSingle() || cfg->GetDiagno();

	DDEDaemon dded;	
	if (dded.Configure(cfg))
		return 1;

	dded.Run();
}

#else

#include <iostream>

int main(int argc, char *argv[]) {
	std::cerr << "SZARP need to be compiled with xmlrpc-epi library for this porogram to work" << std::endl;
	return 1;
}

#endif
