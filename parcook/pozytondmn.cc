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
 * $Id$
 */
/*
 @description_start
 @class 4
 @devices POZYTON EAP-MS-B13DB76 energy meter.
 @devices.pl Licznik energii POZYTON EAP-MS-B13DB76.
 @config See --help option for usage instructions, function codes must be provided on command line.
 @config.pl Uruchom z opcj± --help aby zobaczyæ instrukcjê - w szczególno¶ci demon wymaga podania
 w linii polecenia kodów funkcji pytaj±cych.
 @comment Pozyton Company does not allow to publish communication protocol, so this 
 daemon needs closed-source plugin, loaded from szarp-prop-plugins.so.
 @comment.pl Producent liczników Pozyton nie zezwoli³ na upublicznienie protoko³u
 komunikacyjnego, wobec czego sterownik ten wymaga zamkniêtej wtyczki, dostêpnej w bibliotece
 szarp-prop-plugins.so.
 @description_end
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libgen.h>
#include <dlfcn.h>

#include "pozyton-prop-plugin.h"

#include "ipchandler.h"
#include "liblog.h"
#include "conversion.h"

/* Usage Header */
#define USAGE_HEADER \
  "POZYTON EAP-IMS-B13DB76 Driver\n\
  Usage:\n\
        %s [-s] <n> <port> <--DataCodes=> <--Commas=> <--Ratio=>\n\
  Where:\n\
  -s - diagnostic mode\n\
  n - line number (necessary to read line<n>.cfg file)\n\
  port - full path to port\n\
  --DataCodes= - Number-s of function-s to read. Numbers must be separated commas(,),without space\n\
  --Commas= - Comma position (from right side) for read value\n\
  --Ratio= - Ratio of transmission\n"

void SetNoData(IPCHandler * ipc)
{
	for (int i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;
}

int main(int argc, char *argv[])
{
	DaemonConfig *cfg;
	PozytonDataInterface *dpozyton;
	IPCHandler *ipc;
	unsigned char B4Mode;	//Flaga okre¶laj±ca czy tryb odczytu danych ma byæ 2Bajtowy czy 4Bajtowy 
	int HowParams;		//Liczba parametrów odczytanych z lini komend
	tParams ParamsTable[100];
	int TRatio;
	char *header;

	cfg = new DaemonConfig("pozytondmn");

	/* Ma³e oszustwo dla parsera lini kommend */
	if ((argc - 1 < 5) || (argc - 1 > 6))
		argc = 0;
	else
		argc -= 3;

	asprintf(&header, USAGE_HEADER, argv[0]);
	cfg->SetUsageHeader(header);

	if (cfg->Load(&argc, argv, 0))
		return 1;

	char *plugin_path;
        asprintf(&plugin_path, "%s/szarp-prop-plugins.so", dirname(argv[0]));
        void *plugin = dlopen(plugin_path, RTLD_LAZY);
        if (plugin == NULL) {
		sz_log(0,
				"Cannot load %s library: %s",
				plugin_path, dlerror());
		return 1;
	}
	dlerror();
	free(plugin_path);
	
	PozytonDataCreate_t *createPozyton = (PozytonDataCreate_t*) dlsym(plugin, "PozytonDataCreate");
	log_function_t **log_function = (log_function_t**) dlsym(plugin, "pozyton_log_function");
	if ((createPozyton == NULL) || (log_function == NULL)) {
		sz_log(0, "Cannot load symbols from plugin library: %s", dlerror());
		return 1;
	}
	*log_function = &sz_log;


	dpozyton = createPozyton(cfg->GetDevice()->GetFirstRadio()->
			    GetFirstUnit()->GetParamsCount(), cfg->GetSingle());

	HowParams =
	    dpozyton->ParseCommandLine(argc, argv, ParamsTable, &TRatio);
	if (HowParams == dpozyton->GetParamsCount()) {
		B4Mode = 0;
	} else if (2 * HowParams == dpozyton->GetParamsCount()) {
		B4Mode = 1;
	} else {
		sz_log(0,
		       "pozytondmn: N. of parms from cmd line (%d) and configuration (%d) ain't equal or n. of parms from cmd line isn't 2 times smaller than n. of parms from confuguration",
		       HowParams, dpozyton->GetParamsCount());
		delete dpozyton;
		return 1;
	}

	sz_log(0, "pozytondmn: Byte mode: %d", B4Mode);

	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), dpozyton->GetParamsCount());
	}

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	sz_log(2, "starting main loop");
	SetNoData(ipc);
	while (true) {
		if (dpozyton->Query(SC::S2A(cfg->GetDevice()->GetPath()).c_str(),
				ipc->m_read,
				HowParams,
				ParamsTable,
				TRatio,
				B4Mode,
				cfg->GetSingle()) != 0) {
			SetNoData(ipc);
		}

		ipc->GoParcook();
		sleep(10);	//Wait 10 seconds
	}
	return 0;
}
