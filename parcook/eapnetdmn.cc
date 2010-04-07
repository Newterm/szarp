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
 * eapnetdmn.cc - driver  for POZYTON EAP-IMS-B13DB76 energy meter 
 * (C) Pawe³ Kolega
 * (C) Pawe³ Pa³ucha
 *
 * Pozyton Company does not allow to publish communication protocol, so this 
 * daemon needs closed-source plugin, loaded from szarp-prop-plugins.so.
 */
/*
 @description_start
 @class 4
 @devices Pozyton EAP-IMS-B13DB76 energy meter
 @devices.pl Licznik energii Pozyton EAP-IMS-B13DB76
 @config Driver needs command line parameters, run with '--help' to get usage instruction.
 @config.pl Demon wymaga dodatkowych parametrów w linii komend, zobacz opcjê '--help'.
 @description_end
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dlfcn.h>
#include <libgen.h>

#include "szarp.h"
#include "ipchandler.h"
#include "liblog.h"
#include "conversion.h"
#include "eapnet-prop-plugin.h"

/* Usage Header */
#define USAGE_HEADER \
  "POZYTON EAP-IMS-B13DB76 Driver (Network edition)\n\
  Usage:\n\
        %s [-s] <n> <port> <--EAPCode=> <--Ratio=>\n\
  Where:\n\
  -s - diagnostic mode\n\
  n - line number (necessary to read line<n>.cfg file)\n\
  port - full path to port\n\
  --EAPCodes= - ID Number-s of EAP meter. Numbers must be separated commas(,),without space\n\
  --Ratio= - Ratio of transmission. Numbers must be separated commas(,),without space\n"

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	EAPNetDataInterface     *eappozyton;
	IPCHandler     *ipc;
	int             fd;
	int 		argc_org ;
	short ParsedData[MAX_DATA];/* maxymalna ilosc danych jaka mozna uzyskac to 32 liczniczki po 2 paramsy czyli 64 paramsy  */	
	char *header;
	unsigned int c_params_count ;

	cfg = new DaemonConfig("eapnetdmn");

	argc_org = argc ; 
	/* Ma³e oszustwo dla parsera lini kommend */
	if ((argc - 1 < 4) || (argc - 1 > 6))
		argc = 0;
	else
		argc -= 3;
	asprintf(&header,USAGE_HEADER,argv[0]);
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
		exit(1);
	}
	dlerror();
	free(plugin_path);

	EAPNetDataCreate_t *createEAPNetData = (EAPNetDataCreate_t*) dlsym(plugin, "EAPNetDataCreate");
	//EAPNetDataDestroy_t *destroyEAPNetData = (EAPNetDataDestroy_t*) dlsym(plugin, "EAPNetDataDestroy");

	eappozyton = createEAPNetData(cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount(),
				cfg->GetSingle());
	c_params_count = eappozyton->ParseCommandLine(argc_org, argv);
	if ( eappozyton->m_params_count!= 2 * (int)c_params_count ){
		sz_log(0,"Error in configuration: Number of params from command line: %d / 2, Number of params from params.xml: %d / 2 \n", c_params_count, eappozyton->m_params_count );
		exit (-1);
	}

	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), eappozyton->m_params_count);
	}

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}
	
	sz_log(2, "starting main loop");
	for (int i = 0; i < ipc->m_params_count; i++) {
		ipc->m_read[i] = SZARP_NO_DATA;
	}
	while (true) {
		fd = eappozyton->OpenLine(SC::S2A(cfg->GetDevice()->GetPath()).c_str(), cfg->GetDevice()->GetSpeed());
		eappozyton->Query(fd, ParsedData);
		memcpy(ipc->m_read, ParsedData, eappozyton->m_params_count * sizeof(short));
		ipc->GoParcook();
		close(fd);
	}
	return 0;
}
