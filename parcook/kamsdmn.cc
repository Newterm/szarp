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
 * Daemon for Kamstrup heatmeters Multical IV, 66CDE 
 *
 * Kamstrup Company does not allow to publish communication protocol, so this 
 * daemon needs closed-source plugin, loaded from szarp-prop-plugins.so.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <time.h>
#include <termio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <math.h>
#include <ctype.h>
#include <dlfcn.h>

#include "liblog.h"
#include "ipchandler.h"
#include "kams-prop-plugin.h"

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	IPCHandler     *ipc;

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);
	
	cfg = new DaemonConfig("kamsdmn");
	assert(cfg != NULL);
	if (cfg->Load(&argc, argv))
		return 1;
	if (cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetParamsCount() 
			!= NUMBER_OF_VALS) {
		sz_log(0, "Incorrect number of parameters: %d, must be %d",
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount(),
				NUMBER_OF_VALS);
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
	KamstrupReadData_t *ReadData =
	    (KamstrupReadData_t *) dlsym(plugin, "KamstrupReadData");

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	while (1) {
		for (int i = 0; i < ipc->m_params_count; i++) {
			ipc->m_read[i] = SZARP_NO_DATA;
		}
		ReadData(cfg->GetDevicePath(), ipc->m_read, cfg->GetSingle());
		ipc->GoParcook();

		sleep(280);	/* for saving heatmeter battery */
	}
}
