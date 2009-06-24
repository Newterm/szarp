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
 * (C) Dariusz Marcinkiewicz, Pawe³ Pa³ucha
 * Daemon for Mitsubishi PLC.
 * Mitsubishi Company does not allow to publish communication protocol, so 
 * this daemon needs closed-source plugin, loaded from szarp-prop-plugins.so.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <typeinfo>

#include <stdarg.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "liblog.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "tokens.h"
#include "xmlutils.h"
#include "conversion.h"

#include <dlfcn.h>
#include <libgen.h>
#include "mels-prop-plugin.h"

int ConfigureMELS(MELSDaemonInterface *mels, DaemonConfig *cfg, IPCHandler* ipc) {

	mels->ConfigureSerial(cfg->GetDevicePath(), cfg->GetSpeed());
	mels->SetRead(ipc->m_read);
	mels->SetSend(ipc->m_send);

	xmlNodePtr xdev = cfg->GetXMLDevice();

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(xdev->doc);
	xp_ctx->node = xdev;

	int ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);

	int i, j;
	TParam *param = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetFirstParam();
	TSendParam *send_param = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetFirstSendParam();
	for (i = 0, j = 0; j < ipc->m_params_count + ipc->m_sends_count; ++i, j++) {
		if (j == ipc->m_params_count)
			i = 0;

		char *e;
		asprintf(&e, "/ipk:params/ipk:device[position()=%d]/ipk:unit[position()=1]/ipk:%s[position()=%d]",
			cfg->GetLineNumber(), 
			i != j ? "send" : "param" ,
			i + 1);
		assert (e != NULL);

		xmlNodePtr n = uxmlXPathGetNode(BAD_CAST e, xp_ctx);
		assert(n != NULL);
		free(e);

		xmlChar* _address = xmlGetNsProp(n,
				BAD_CAST("address"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

		if (_address == 0) {
			sz_log(0, "Error, attribute mels:address missing in param definition, line(%ld)", xmlGetLineNo(n));
			return 1;
		}

		char *endptr;
		unsigned address = strtoul((char*)_address, &endptr, 0);
		if (*endptr != 0) {
			xmlFree(_address);
			sz_log(0, "Error, invalid value of parameter mels:addres in param definition, line(%ld)", xmlGetLineNo(n));
			return 1;
		}
		xmlFree(_address);

		xmlChar* _address_type = xmlGetNsProp(n, 
				BAD_CAST("address_type"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
			
		if (_address_type == 0) {
			sz_log(0, "Error, attribute mels:address_type missing in param definition, line(%ld)", xmlGetLineNo(n));
			return 1;
		}

		if (_address_type != NULL 
				&& xmlStrcmp(_address_type, BAD_CAST "standard")
				&& xmlStrcmp(_address_type, BAD_CAST "extended")) {
			xmlFree(_address_type);
			sz_log(0, "Error, invalid value of mels:address_type in param definition, line(%ld)", xmlGetLineNo(n));
			return 1;
		}

		bool extended = _address_type != NULL && xmlStrcmp(_address_type, BAD_CAST "extended") == 0;
		xmlFree(_address_type);

		xmlChar* _type = xmlGetNsProp(n,
				BAD_CAST("val_type"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_type == NULL) {
			sz_log(0, "Error, missing parameter type specification (mels:val_type) in line(%ld)", xmlGetLineNo(n));
			return 1;
		}

		MelsParam::TYPE val_type;
		if (!xmlStrcmp(_type, BAD_CAST "float"))
			val_type = MelsParam::FLOAT;
		else if (!xmlStrcmp(_type, BAD_CAST "short_float"))
			val_type = MelsParam::SHORT_FLOAT;
		else if (!xmlStrcmp(_type, BAD_CAST "integer"))
			val_type = MelsParam::INTEGER;
		else {
			sz_log(0, "Error, invalid parameter type specification (mels:val_type) in line(%ld), only 'integer' and 'float' supported", xmlGetLineNo(n));
			xmlFree(_type);
			return 1;
		}
		xmlFree(_type);


		int prec = 0;
		if (i != j) {
			if (!send_param->GetParamName().empty()) {
				param = cfg->GetIPK()->getParamByName(send_param->GetParamName());
				if (param == NULL) {
					sz_log(0, "parameter with name '%ls' not found (send parameter %d for device %d)",
						send_param->GetParamName().c_str(), i + 1, cfg->GetLineNumber());
					return 1;
				}
				prec = param->GetPrec();
			}
		} else
			prec = param->GetPrec();
			
		MELSDaemonInterface::PARAMETER_TYPE ptype;
		if (i == j) 
			if (extended)
				ptype = MELSDaemonInterface::PARAM_EXTENDED;
			else
				ptype = MELSDaemonInterface::PARAM_GENERAL;
		else
			if (extended)
				ptype = MELSDaemonInterface::SEND_EXTENDED;
			else
				ptype = MELSDaemonInterface::SEND_GENERAL;
		mels->ConfigureParam(i, address, ptype, prec, val_type);
		if (i != j)
			send_param = send_param->GetNext();
		else
			param = param->GetNext();
	}

	xmlXPathFreeContext(xp_ctx);

	return 0;
}


int main(int argc, char *argv[]) {
	DaemonConfig* cfg = new DaemonConfig("melsdmn");


	if (cfg->Load(&argc, argv))
		return 1;

	IPCHandler* ipc = new IPCHandler(cfg);
	if (ipc->Init()) {
		sz_log(0, "Intialization of IPCHandler failed");
		return 1;
	}

	char *plugin_path;
        asprintf(&plugin_path, "%s/szarp-prop-plugins.so", dirname(argv[0]));
        void *plugin = dlopen(plugin_path, RTLD_LAZY);
        if (plugin == NULL) {
		sz_log(0,
				"Cannot load %s library: %s",
				plugin_path, dlerror());
		exit(1);
	}
	dlerror();
	free(plugin_path);
	
	MELSDaemonCreate_t *createMELS = (MELSDaemonCreate_t*) dlsym(plugin, "MELSDaemonCreate");
	log_function_t **log_function = (log_function_t**) dlsym(plugin, "mels_log_function");
	if (createMELS == NULL) {
		sz_log(0, "Cannot load symbols from plugin library: %s", dlerror());
		return 1;
	}
	*log_function = &sz_log;

	MELSDaemonInterface* mels = createMELS();
	if (ConfigureMELS(mels, cfg, ipc))
		return 1;

	while (true) {
		sz_log(6, "Beginning parametr fetching loop");

		for (int i = 0; i < ipc->m_params_count; ++i) 
			ipc->m_read[i] = SZARP_NO_DATA;

		ipc->GoSender();

		try {

			mels->OpenPort();

			if (mels->Connect() == 0) {
				mels->CallAll();
			}

		} catch (SerialException &e) {
			sz_log(0, "Serial port error %s", e.what());
		}

		ipc->GoParcook();
		time_t st = time(NULL);

		int s = st + 10 - time(NULL);
		while (s > 0) 
			s = sleep(s);

		sz_log(6, "End of of loop");
	}
}

