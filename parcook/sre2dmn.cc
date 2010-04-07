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
 * Tech-Agro Company (http://www.techagro.com.pl) does not allow to publish 
 * communication protocol, so this  daemon needs closed-source plugin, loaded 
 * from szarp-prop-plugins.so.
 *
 * Daemon for electric energy sumator SRE2. Based on information from technical
 * documentation bought from producer. Version without configurable channels.
 * SRE2 has 8 channels.
 * wersja bez konfigurowalnych kana³ów SRE-2 ma 8 kana³ów bez mozliwosci uzycia retransmitera (chyba)
 */

/*
 @description_start
 @class 4
 @devices Tech-Agro SRE2 Electric Energy Sumator.
 @devices.pl Sumator liczników energii elektrycznej Tech-Agro SRE2.
 @config Version without configurable channels (8 channels possible). Available parameters:

 - Energy MSB I

 - Energy LSB I

   ...

 - Energy MSB VIII

 - Energy LSB VIII

 - Power I

   ...

 - Power VIII

 @description_end
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <termio.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <libgen.h>

#include "szarp.h"
#include "ipchandler.h"
#include "liblog.h"
#include "conversion.h"
#include "sre2-prop-plugin.h"

class Sre2dmn {
 public:
	int m_params_count;
				/**< size of params array */
	int m_serial_no;
	int energy_factor;
	Sre2dmn(int params) {
		assert(params >= 0);

		m_params_count = params;
	} ~Sre2dmn() {
	}

	/**
	 * @param node XML 'device' element
	 * @return 0 on success, 1 on error
	 */
	int parseDevice(xmlNodePtr node, GetSerialNum_t *getSerialNum_f);

	/**
	 * @param Device Path to device np "/dev/ttyS0"
	 * @param BaudRate baud rate
	 * @param StopBits stop bits 1 od 2
	 * @param Parity parity : NO_PARITY, ODD, EVEN 
	 * @param return file descriptor
	 */
	int InitComm(const char *Device, long BaudRate, unsigned char DataBits,
		     unsigned char StopBits, unsigned char Parity);
};

int Sre2dmn::parseDevice(xmlNodePtr node, GetSerialNum_t* getSerialNum_f)
{
	assert(node != NULL);
	char *str;
	char *tmp;

	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("serial_no"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		       "attribute sre2dmn:serial_no not found in device element, line %ld",
		       xmlGetLineNo(node));
		free(str);
		return 1;
	}

	m_serial_no = getSerialNum_f(strtol(str, &tmp, 0));
	if ((tmp[0] != 0) || (m_serial_no == BCD_ERROR)) {
		sz_log(0,
		       "error parsing sre2dmn:serial_no attribute ('%s'), line %ld",
		       str, xmlGetLineNo(node));
		free(str);
		return 2;
	}
	free(str);
	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("energy_factor"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		       "attribute sre2dmn:energy_factor not found in device element, line %ld",
		       xmlGetLineNo(node));
		free(str);
		return 1;
	}
	energy_factor = strtol(str, &tmp, 0);
	if (tmp[0] != 0) {
		sz_log(0,
		       "error parsing sre2dmn:energy_factor attribute ('%s'), line %ld",
		       str, xmlGetLineNo(node));
		free(str);
		return 2;
	}
	free(str);

	return 0;
}

int Sre2dmn::InitComm(const char *Device, long BaudRate, unsigned char DataBits,
		      unsigned char StopBits, unsigned char Parity)
{
	int CommId;
	long BaudRateStatus;
	long DataBitsStatus;
	long StopBitsStatus;
	long ParityStatus;
	struct termios rsconf;
	CommId = open(Device, O_RDWR | O_NDELAY | O_NONBLOCK);
	if (CommId < 0)
		return CommId;

	switch (BaudRate) {
	case 300:
		BaudRateStatus = B300;
		break;
	case 600:
		BaudRateStatus = B600;
		break;
	case 1200:
		BaudRateStatus = B1200;
		break;
	case 2400:
		BaudRateStatus = B2400;
		break;
	case 4800:
		BaudRateStatus = B4800;
		break;
	case 9600:
		BaudRateStatus = B9600;
		break;
	case 19200:
		BaudRateStatus = B19200;
		break;
	case 38400:
		BaudRateStatus = B38400;
		break;
	case 115200:
		BaudRateStatus = B115200;
		break;
	default:
		BaudRateStatus = B300;
		break;
	}

	switch (DataBits) {
	case 7:
		DataBitsStatus = CS7;
		break;
	case 8:
		DataBitsStatus = CS8;
		break;
	default:
		DataBitsStatus = CS8;
		break;
	}

	switch (StopBits) {
	case 1:
		StopBitsStatus = 0;
		break;
	case 2:
		StopBitsStatus = CSTOPB;
		break;
	default:
		StopBitsStatus = 0;
		break;
	}

	switch (Parity) {
	case NO_PARITY:
		ParityStatus = 0;
		break;
	case EVEN:
		ParityStatus = PARENB;
		break;
	case ODD:
		ParityStatus = PARENB | PARODD;
		break;
	default:
		ParityStatus = 0;
		break;
	}
	tcgetattr(CommId, &rsconf);
	rsconf.c_cflag =
	    BaudRateStatus | DataBitsStatus | StopBitsStatus |
	    ParityStatus | CLOCAL | CREAD;

	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
	rsconf.c_lflag = 0;
	rsconf.c_cc[VMIN] = 1;
	rsconf.c_cc[VTIME] = 0;
	tcsetattr(CommId, TCSANOW, &rsconf);
	return CommId;
}

int main(int argc, char *argv[])
{
	DaemonConfig *cfg;
	Sre2dmn *sre2info;
	IPCHandler *ipc;
	int fd;

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
	
	InitSRE2Data_t *initSRE2Data = (InitSRE2Data_t*) dlsym(plugin, "InitSRE2Data");
	GetSRE2Data_t *getSRE2Data = (GetSRE2Data_t*) dlsym(plugin, "GetSRE2Data");
	GetSerialNum_t *getSerialNum = (GetSerialNum_t*) dlsym(plugin, "GetSerialNum");
	if ((initSRE2Data == NULL) || (getSRE2Data == NULL) || (getSerialNum == NULL)) {
		sz_log(0, "Cannot load symbols from plugin library: %s", dlerror());
		return 1;
	}

	cfg = new DaemonConfig("sre2dmn");

	if (cfg->Load(&argc, argv))
		return 1;
	sre2info =
	    new Sre2dmn(cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetParamsCount());
	if (sre2info->parseDevice(cfg->GetXMLDevice(), getSerialNum))
		return 1;
	if (sre2info->m_params_count != 2 + 3 * MAX_CHANNELS) {
		sz_log(0, "Number of parameters must be %d, found: %d",
				2 + 3 * MAX_CHANNELS, sre2info->m_params_count);
		return 1;
	}

	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), sre2info->m_params_count);
	}

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	for (int i = 0; i < 2+3*MAX_CHANNELS; i++)
		ipc->m_read[i] = SZARP_NO_DATA;
	DataPower **dps = initSRE2Data();
	
	sz_log(2, "starting main loop");
	while (true) {
		sleep(5);
		fd = sre2info->
		    InitComm(SC::S2A(cfg->GetDevice()->GetPath()).c_str(),
			     cfg->GetDevice()->GetSpeed(), 8, 1, NO_PARITY);
		getSRE2Data(fd, ipc->m_read, sre2info->m_serial_no, dps, sre2info->energy_factor, cfg->GetSingle());

		close(fd);
		ipc->GoParcook();
	}
	return 0;
}
