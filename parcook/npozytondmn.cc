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
 * (C) Pawe³ Kolega, Pawe³ Pa³ucha
 * Daemon for POZYTON electric energy meter.
 * Pozyton Company does not allow to publish communication protocol, so 
 * this daemon needs closed-source plugin, loaded from szarp-prop-plugins.so.
 */
/*
 @description_start
 @class 4
 @devices Pozyton electric energy meters.
 @devices.pl Liczniki energii elektrycznej Pozyton.
 @comment Pozyton Company does not allow to publish communication protocol, so this 
 daemon needs closed-source plugin, loaded from szarp-prop-plugins.so.
 @comment.pl Producent liczników Pozyton nie zezwoli³ na upublicznienie protoko³u
 komunikacyjnego, wobec czego sterownik ten wymaga zamkniêtej wtyczki, dostêpnej w bibliotece
 szarp-prop-plugins.so.
 @config.pl Obs³ugiwane parametry konfiguracyjne: voltage_factor = dzielnik przek³adników napiêciowych;
 current_factor - dzielnik przek³adników pr±dowych; delay = - opó¼nienie odpytywania;
 interface = opto|rs485|currloop - typ interfejsu komunikacyjnego, codes - kody funkcji jakie maj± byæ odczytywane.
 @config_example
 <device daemon="/opt/szarp/bin/npozytondmn" xmlns:pozyton="http://www.praterm.com.pl/SZARP/ipk-extra"
 	path="/dev/ttyS0" pozyton:voltage_factor="60" pozyton:current_factor="200" pozyton:interface="opto" 
	pozyton:codes="0.8.0/1;1.8.0/1;2.8.0/1;3.8.0/1;97.5.1/1;97.5.2/1;97.5.3/1;97.4.1*10;97.4.2*10;97.4.3*10;97.6.0*100;97.1.1/1000;97.1.2/1000;97.1.3/1000;97.1.0/1000;97.2.1/1000;97.2.2/1000;97.2.3/1000;97.2.0/1000;97.3.1/1000;97.3.2/1000;97.3.3/1000;97.3.0/1000">
	<unit id="1" type="1" subtype="1" bufsize="1">
		<param name="Sieæ:LZQM-1:Energia EP plus lsw" .../>
		<param name="Sieæ:LZQM-1:Energia EP plus msw" .../>
		<param name="Sieæ:LZQM-1:Energia EP minus lsw" .../>
		<param name="Sieæ:LZQM-1:Energia EP minus msw" .../>
		<param name="Sieæ:LZQM-1:Energia EQ plus lsw" .../>
		<param name="Sieæ:LZQM-1:Energia EQ plus msw" .../>
		<param name="Sieæ:LZQM-1:Energia EQ minus lsw" .../>
		<param name="Sieæ:LZQM-1:Energia EQ minus msw" .../>
		<param name="Sieæ:LZQM-1:Napiêcie U1" .../>
		<param name="Sieæ:LZQM-1:Napiêcie U2" .../>
		<param name="Sieæ:LZQM-1:Napiêcie U3" .../>
		<param name="Sieæ:LZQM-1:Pr±d I1" .../>
		<param name="Sieæ:LZQM-1:Pr±d I2" .../>
		<param name="Sieæ:LZQM-1:Pr±d I3" .../>
		<param name="Sieæ:LZQM-1:Czêstotliwo¶æ" .../>
		<param name="Sieæ:LZQM-1:Moc czynna faza 1" .../>
		<param name="Sieæ:LZQM-1:Moc czynna faza 2" .../>
		<param name="Sieæ:LZQM-1:Moc czynna faza 3" .../>
		<param name="Sieæ:LZQM-1:Moc czynna sumaryczna rezerwa" .../>
		<param name="Sieæ:LZQM-1:Moc bierna faza 1" .../>
		<param name="Sieæ:LZQM-1:Moc bierna faza 2" .../>
		<param name="Sieæ:LZQM-1:Moc bierna faza 3" .../>
		<param name="Sieæ:LZQM-1:Moc bierna sumaryczna rezerwa" .../>
		<param name="Sieæ:LZQM-1:Moc pozorna faza 1" .../>
		<param name="Sieæ:LZQM-1:Moc pozorna faza 2" .../>
		<param name="Sieæ:LZQM-1:Moc pozorna faza 3" .../>
		<param name="Sieæ:LZQM-1:Moc pozorna sumaryczna rezerwa" .../>
	</unit>
 </device>
 @description_end
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dlfcn.h>
#include <libgen.h>
#include <libxml/tree.h>
#include "npozyton-prop-plugin.h"
#include "ipchandler.h"
#include "liblog.h"
#include "conversion.h"
#include "custom_assert.h"

/* Usage Header */
#define USAGE_HEADER \
  "POZYTON Driver (new)\n\
  Usage:\n\
        %s [-s] <n> <port>\n\
  Where:\n\
  -s - diagnostic mode\n\
  n - line number (necessary to read line<n>.cfg file)\n\
  port - full path to port\n"

int parseXMLDevice(xmlNodePtr node, NPozytonDataInterface * data)
{

	/* Obs³ugiwane parametry konfiguracyjne */
	/* voltage_factor = - dzielnik przek³adników napiêciowych */
	/* current_factor = - dzielnik przek³adników pr±dowych */
	/* delay = - opó¼nienie odpytywania */

	/* interface =opto|rs485|currloop - typ interfejsu komunikacyjnego */
	/* codes= - kody funkcji jakie maj± byæ odczytywane */

	ASSERT(node != NULL);
	char *str;
	char *tmp;

	short voltage_factor;
	short current_factor;
	short delay;
	short type = 0;
	char interface;

	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("delay"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		delay = 10;
		free(str);
	} else {

		delay = strtol(str, &tmp, 10);
		if ((tmp[0] != 0)) {
			sz_log(1,
			       "error parsing npozytondmn:delay attribute ('%s'), line %ld",
			       str, xmlGetLineNo(node));
			free(str);
			return 2;
		}
		free(str);
	}

	 /**/
	    str = (char *)xmlGetNsProp(node,
				       BAD_CAST("voltage_factor"),
				       BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(1,
		       "attribute npozytondmn:voltage_factor not found in device element, line %ld",
		       xmlGetLineNo(node));
		free(str);
		return 1;
	}

	voltage_factor = strtol(str, &tmp, 10);
	if ((tmp[0] != 0)) {
		sz_log(1,
		       "error parsing npozytondmn:voltage_factor attribute ('%s'), line %ld",
		       str, xmlGetLineNo(node));
		free(str);
		return 2;
	}
	free(str);
	 /**/
	    /**/
	    str = (char *)xmlGetNsProp(node,
				       BAD_CAST("current_factor"),
				       BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(1,
		       "attribute npozytondmn:current_factor not found in device element, line %ld",
		       xmlGetLineNo(node));
		free(str);
		return 1;
	}

	current_factor = strtol(str, &tmp, 10);
	if ((tmp[0] != 0)) {
		sz_log(1,
		       "error parsing npozytondmn:current_factor attribute ('%s'), line %ld",
		       str, xmlGetLineNo(node));
		free(str);
		return 2;
	}
	free(str);
	 /**/
	    /**/
	    str = (char *)xmlGetNsProp(node,
				       BAD_CAST("type"),
				       BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str != NULL) {

		if (!strcasecmp(str, "LZQM"))
			type = T_LZQM;
		else if (!strcasecmp(str, "EQABP"))
			type = T_EQABP;
		else if (!strcasecmp (str, "EQM"))
			type = T_EQM;
		else {
			sz_log(1,
			       "error parsing npozytondmn:type attribute ('%s'), line %ld",
			       str, xmlGetLineNo(node));
			free(str);
			return 2;
		}

		free(str);
	}
	/**/ /**/
	    str = (char *)xmlGetNsProp(node,
				       BAD_CAST("interface"),
				       BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(1,
		       "attribute npozytondmn:interface not found in device element, line %ld",
		       xmlGetLineNo(node));
		free(str);
		return 1;
	}

	if (!strcasecmp(str, "opto")) {
		interface = C_OPTO;
	} else if (!strcasecmp(str, "rs485")) {
		interface = C_RS485;
	} else if (!strcasecmp(str, "currloop")) {
		interface = C_CURRLOOP;
	} else {
		sz_log(1,
		       "error parsing npozytondmn:interface attribute ('%s'), line %ld it should be opto|rs485|currloop",
		       str, xmlGetLineNo(node));
		free(str);
		return 2;
	}
	free(str);
	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("codes"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(1,
		       "attribute npozytondmn:codes not found in device element, line %ld",
		       xmlGetLineNo(node));
		free(str);
		return 1;
	}
	data->Configure(voltage_factor, current_factor, delay, type, interface);
	data->FilterCodes(str);
	data->LoadCodesToList(str);
	free(str);
	return 0;
}

void SetNoData(short *data, int count)
{
	for (int i = 0; i < count; i++)
		data[i] = SZARP_NO_DATA;

}

int main(int argc, char *argv[])
{
	DaemonConfig *cfg;
	NPozytonDataInterface *dpozyton;
	IPCHandler *ipc;
	char *header;
	short Size;

	cfg = new DaemonConfig("npozytondmn");

	asprintf(&header, USAGE_HEADER, argv[0]);
	cfg->SetUsageHeader(header);

	if (cfg->Load(&argc, argv, 0))
		return 1;
	int params_count = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->
		                            GetParamsCount();

	char *plugin_path;
        asprintf(&plugin_path, "%s/szarp-prop-plugins.so", dirname(argv[0]));
        void *plugin = dlopen(plugin_path, RTLD_LAZY);
        if (plugin == NULL) {
		sz_log(1,
				"Cannot load %s library: %s",
				plugin_path, dlerror());
		return 1;
	}
	dlerror();
	free(plugin_path);
	
	NPozytonDataCreate_t *createPozyton = (NPozytonDataCreate_t*) dlsym(plugin, "NPozytonDataCreate");
	log_function_t **log_function = (log_function_t**) dlsym(plugin, "npozyton_log_function");
	if ((createPozyton == NULL) || (log_function == NULL)) {
		sz_log(0, "Cannot load symbols from plugin library: %s", dlerror());
		return 1;
	}
	*log_function = &sz_log;
	dpozyton = createPozyton(params_count, cfg->GetSingle());
	if (parseXMLDevice(cfg->GetXMLDevice(), dpozyton) != 0) {
		sz_log(0, "Error parsing xml, exiting");
		return 1;
	}

	Size = dpozyton->GoCodes();
	if (Size < 0) {
		free(dpozyton);
		sz_log(0, "Error parsing xml, exiting");
		return 1;
	}

	short *ParsedData = (short *)malloc(sizeof(short) * Size);
	if (cfg->GetSingle()) {
		fprintf(stderr, "line number: %d\ndevice: %s\nparams in: %d\n", 
				cfg->GetLineNumber(), 
				SC::S2A(cfg->GetDevice()->GetPath()).c_str(),
				params_count);
		dpozyton->PrintInfo(stderr);
	}

	try {
		ipc = new IPCHandler(m_cfg);
	} catch(const std::exception& e) {
		sz_log(0, "Error while initializing IPC: %s, exiting", e.what());
		return 1;
	}

	sz_log(2, "starting main loop");

	SetNoData(ipc->m_read, ipc->m_params_count);

	while (true) {
		if (dpozyton->Query(SC::S2A(cfg->GetDevice()->GetPath()).c_str(), ParsedData) != 0) {
			SetNoData(ipc->m_read, ipc->m_params_count);
		} else {
			memcpy(ipc->m_read, ParsedData, params_count * sizeof(short));
		}
		ipc->GoParcook();
		if (cfg->GetSingle())
			sleep(10);
		else
			sleep(dpozyton->GetDelay());
	}
	free(header);
	free(ParsedData);
	return 0;
}

