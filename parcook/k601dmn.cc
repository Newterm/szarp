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
 * (C) Pawe³ Kolega 2007
 * (C) Pawe³ Pa³ucha 2009
 * $Id$
 */
/*
 @description_start
 @class 4
 @devices Kamstrup Multical 601 heatmeter.
 @devices.pl Ciep³omierz Kamstrup Multical 601.
 @protocol Kamstrup Heat Meter Protocol.

 @comment Kamstrup Company does not allow to publish communication protocol, so this 
 daemon needs closed-source plugin, loaded from szarp-prop-plugins.so.
 @comment.pl Producent ciep³omierzy Kamstrup nie zezwoli³ na upublicznienie protoko³u
 komunikacyjnego, wobec czego sterownik ten wymaga zamkniêtej wtyczki, dostêpnej w bibliotece
 szarp-prop-plugins.so.
 
 @config Example of params.xml configuration, attributes from 'm601' namespace are mandatory.
 @config.pl Przyk³adowa konfiguracja w pliku params.xml, atrybuty z przetrzenie nazw 'm601' 
 s± obowi±zkowe.
 @config_example
 <device 
      xmlns:m601="http://www.praterm.com.pl/SZARP/ipk-extra"
      daemon="/opt/szarp/bin/k601dmn" 
      path="/dev/ttyA11"
      m601:delay_between_chars="10000"
      m601:delay_between_requests="10">
      <unit id="1">
              <param
                      name="..."
                      ...
                      k601:register="0x80"
                      k601:type="lsb"
                      k601:multiplier="100">
                      ...
              </param>
              <param
                      name="..."
                      ...
                      k601:address="0x80"
                      k601:type="msb"
			k601:multiplier="100">
                      ...
              </param>
              ...
              <param
                      name="..."
                      ...
                      k601:address="0x81"
                      k601:type="single"
			k601:multiplier="1">
                      ...
              </param>
      </unit>
 </device>
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
#include "conversion.h"

#include "ipchandler.h"
#include "liblog.h"

#include "k601-prop-plugin.h"

#define REGISTER_NOT_FOUND -1

#undef STRONG_DEBUG

class SerialPort {
 public:
	SerialPort() {
		fd = -1;
	};
	char OpenSerialPort(const char *device);
	int PutFD() {
		return fd;
	};
	void CloseSerialPort() {
		if (fd > 0)
			close(fd);
		fd = -1;
	};
	~SerialPort() {		/*if (fd > 0) close(fd); */
	};
	int fd;
	static const char SERIAL_OK;
	static const char SERIAL_ERROR;
};

char SerialPort::OpenSerialPort(const char *device)
{
	struct termio rsconf;
	sz_log(10, "SerialPort::OpenSerialPort");
	fd = open(device, O_RDWR | O_NDELAY | O_NONBLOCK);
	if (fd < 0)
		return (SERIAL_ERROR);
	ioctl(fd, TCGETA, &rsconf);
	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
	rsconf.c_cflag = B1200 | CS8 | CLOCAL | CREAD | CSTOPB;
	rsconf.c_lflag = 0;
	ioctl(fd, TCSETA, &rsconf);
	return (SERIAL_OK);
}

const char SerialPort::SERIAL_OK = 0;
const char SerialPort::SERIAL_ERROR = -1;

/* This ID-s will be shown in single (test mode) */
unsigned short TO_SHOW_CODES[] =
    { 60, 94, 63, 61, 62, 95, 96, 97, 110, 64, 65, 68, 69, 84, 85, 72, 73, 99,
86, 87, 88, 122, 89, 91, 92, 74, 75, 80 };

/** possible types of modbus communication modes */
typedef enum {
	T_LSB,
	T_MSB,
	T_SINGLE,
	T_ERROR,
} ParamMode;

typedef enum {
	RT_LSBMSB,
	RT_SINGLE,
	RT_ERROR,
} RegisterType;

/**
 * Modbus communication config.
 */
class KamstrupInfo {
 public:
	/** info about single parameter */
	class ParamInfo {
 public:
		unsigned short reg;  /**< KMP REGISTER */
		unsigned long mul;   /**< ADDITIONAL MULTIPLIER */
		ParamMode type;
	};
	unsigned long delay_between_chars;
	unsigned short delay_between_requests;

	ParamInfo *m_params;
	int m_params_count;
	unsigned short *unique_registers;
	unsigned short unique_registers_count;

	/**
	 * @param params number of params to read
	 */
	KamstrupInfo(int params) {
		assert(params >= 0);

		m_params_count = params;
		if (params > 0) {
			m_params = new ParamInfo[params];
			assert(m_params != NULL);
		} else
			m_params = NULL;
	}

	~KamstrupInfo() {
		delete m_params;
		free(unique_registers);
	}

	int parseXML(xmlNodePtr node, DaemonConfig * cfg);

	int parseDevice(xmlNodePtr node);

	int parseParams(xmlNodePtr unit, DaemonConfig * cfg);

	RegisterType DetermineRegisterType(unsigned short RegisterAddress);

	int FindRegisterIndex(unsigned short RegisterAddress, ParamMode PM);

	unsigned long DetermineRegisterMultiplier(unsigned short
						  RegisterAddress);

	void SetNoData(IPCHandler * ipc);
};

int KamstrupInfo::parseDevice(xmlNodePtr node)
{
	assert(node != NULL);
	char *str;
	char *tmp;
	sz_log(10, "KamstrupInfo::parseDevice");

	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("delay_between_chars"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		       "attribute k601:delay_between_chars not found in device element, line %ld",
		       xmlGetLineNo(node));
		return 1;
	}

	delay_between_chars = strtol(str, &tmp, 0);
	if (tmp[0] != 0) {
		sz_log(0,
		       "error parsing k601:delay_between_chars attribute ('%s'), line %ld",
		       str, xmlGetLineNo(node));
		return 1;
	}
	free(str);
	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("delay_between_requests"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		       "attribute k601:delay_between_requests not found in device element, line %ld",
		       xmlGetLineNo(node));
		return 1;
	}

	delay_between_requests = strtol(str, &tmp, 0);
	if (tmp[0] != 0) {
		sz_log(0,
		       "error parsing k601:delay_between_requests attribute ('%s'), line %ld",
		       str, xmlGetLineNo(node));
		return 1;
	}
	free(str);
	return 0;
}

int KamstrupInfo::parseParams(xmlNodePtr unit, DaemonConfig * cfg)
{
	int params_found = 0;
	char *str;
	char *tmp;
	unsigned short reg;
	unsigned long mul;
	ParamMode type;
	unsigned short i, j, k;
	unsigned short *registers;
	char latch;
	sz_log(10, "KamstrupInfo::parseParams");

	for (xmlNodePtr node = unit->children; node; node = node->next) {
		if (node->ns == NULL)
			continue;
		if (strcmp
		    ((char *)node->ns->href,
		     (char *)SC::S2U(IPK_NAMESPACE_STRING).c_str()) != 0)
			continue;
		if (strcmp((char *)node->name, "param"))
			continue;

		str = (char *)xmlGetNsProp(node, BAD_CAST("register"),
					   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (str == NULL) {
			sz_log(0,
			       "attribute k601:register not found (line %ld)",
			       xmlGetLineNo(node));
			return 1;
		}
		reg = strtol(str, &tmp, 0);
		if (tmp[0] != 0) {
			sz_log(0,
			       "error parsing k601:register attribute value ('%s') - line %ld",
			       str, xmlGetLineNo(node));
			return 1;
		}
		free(str);

		str = (char *)xmlGetNsProp(node, BAD_CAST("multiplier"),
					   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (str == NULL) {
			sz_log(0,
			       "attribute k601:multiplier not found (line %ld)",
			       xmlGetLineNo(node));
			free(str);
			return 1;
		}
		mul = strtol(str, &tmp, 0);
		if (tmp[0] != 0) {
			sz_log(0,
			       "error parsing k601:multiplier attribute value ('%s') - line %ld",
			       str, xmlGetLineNo(node));
			return 1;
		}
		free(str);

		str = (char *)xmlGetNsProp(node, BAD_CAST("type"),
					   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (str == NULL) {
			sz_log(0,
			       "attribute k601:type not found (line %ld)",
			       xmlGetLineNo(node));
			type = T_SINGLE;
		}

		if (!strcasecmp(str, "lsb"))
			type = T_LSB;
		else if (!strcasecmp(str, "msb"))
			type = T_MSB;
		else if (!strcasecmp(str, "single"))
			type = T_SINGLE;
		else {
			type = T_ERROR;
			sz_log(0,
			       "attribute k601:type is illegal (%s) (line %ld)",
			       str, xmlGetLineNo(node));
			return 1;
		}

		free(str);

		if (!strcmp((char *)node->name, "param")) {
			/*
			 * for 'param' element 
			 */
			assert(params_found < m_params_count);
			m_params[params_found].reg = reg;
			m_params[params_found].mul = mul;
			m_params[params_found].type = type;
			TParam *p =
			    cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->
			    GetFirstParam()->GetNthParam(params_found);
			assert(p != NULL);
			params_found++;
		}

	}

	/**
	 * Calculating unique addresses of registers;
	 */
	unique_registers =
	    (unsigned short *)malloc(params_found * sizeof(unsigned short));
	registers =
	    (unsigned short *)malloc(params_found * sizeof(unsigned short));
	unique_registers_count = 0;
	memset(registers, 0, params_found * sizeof(unsigned short));
	latch = 0;
	k = 0;
	for (i = 0; i < params_found; i++) {
		latch = 0;
		for (j = 0; j < i; j++) {
			if (m_params[i].reg == registers[j])
				latch = 1;
		}

		if (!latch) {
			unique_registers_count++;
			registers[i] = m_params[i].reg;
			unique_registers[k] = m_params[i].reg;
			k++;
		}
	}
	free(registers);

	assert(params_found == m_params_count);

	return 0;
}

void KamstrupInfo::SetNoData(IPCHandler * ipc)
{
	int i;
	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;
}

int KamstrupInfo::parseXML(xmlNodePtr node, DaemonConfig * cfg)
{
	assert(node != NULL);
	sz_log(10, "KamstrupInfo:parseXML");

	if (parseDevice(node))
		return 1;

	for (node = node->children; node; node = node->next) {
		if ((node->ns &&
		     !strcmp((char *)node->ns->href,
			     (char *)SC::S2U(IPK_NAMESPACE_STRING).c_str())) ||
		    !strcmp((char *)node->name, "unit"))
			break;
	}
	if (node == NULL) {
		sz_log(0, "can't find element 'unit'");
		return 1;
	}

	return parseParams(node, cfg);
}

RegisterType KamstrupInfo::DetermineRegisterType(unsigned short RegisterAddress)
{
	unsigned short i;
	sz_log(10, "KamstrupInfo:DetermineRegisterType");

	for (i = 0; i < m_params_count; i++) {
		if (RegisterAddress == m_params[i].reg) {
			switch (m_params[i].type) {
			case T_LSB:
				return (RT_LSBMSB);
				break;
			case T_MSB:
				return (RT_LSBMSB);
				break;
			case T_SINGLE:
				return (RT_SINGLE);
				break;
			default:
				return (RT_ERROR);
			}

		}

	}
	return (RT_ERROR);
}

int KamstrupInfo::FindRegisterIndex(unsigned short RegisterAddress,
				    ParamMode PM)
{
	int i;
	sz_log(10, "KamstrupInfo::FindRegisterIndex");
	for (i = 0; i < m_params_count; i++) {
		if (RegisterAddress == m_params[i].reg
		    && PM == m_params[i].type)
			return i;
	}
	return REGISTER_NOT_FOUND;
}

unsigned long KamstrupInfo::
DetermineRegisterMultiplier(unsigned short RegisterAddress)
{
	int i;
	char lsb_msb_mode = 0;
	unsigned long multiplier = 1;
	sz_log(10, "KamstrupInfo::DetermineRegisterMultiplier");

	for (i = 0; i < m_params_count; i++) {
		if (RegisterAddress == m_params[i].reg
		    && m_params[i].type == T_LSB) {
			if (!lsb_msb_mode) {
				multiplier = m_params[i].mul;
			} else {
				multiplier += m_params[i].mul;
				multiplier /= 2;
			}
		}
		if (RegisterAddress == m_params[i].reg
		    && m_params[i].type == T_MSB) {
			if (!lsb_msb_mode) {
				multiplier = m_params[i].mul;
			} else {
				multiplier += m_params[i].mul;
				multiplier /= 2;
			}
		}

		if (RegisterAddress == m_params[i].reg
		    && m_params[i].type == T_SINGLE) {
			multiplier = m_params[i].mul;
		}
	}
	return multiplier;
}

#ifdef STRONG_DEBUG
#include <time.h>
void AddToLog(unsigned char *dbg, unsigned short size)
{

	time_t rawtime;
	struct tm *timeinfo;

	FILE *file;
	int i;
	char buf[100];
	sprintf(buf, "/root/k601-%d-log.log", (short)getpid());
	file = fopen(buf, "a+");

	if (file == NULL) {
		if (file == NULL) {
			printf("Nie otwarto pliku \n");
			return;
		}
	}

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	fprintf(file, "[%d] %s", (short)getpid(), asctime(timeinfo));

	for (i = 0; i < size; i++) {
		if (i + 1 < size)
			fprintf(file, "0x%x,", dbg[i]);
		else if (i + 1 == size)
			fprintf(file, "0x%x", dbg[i]);
	}
	fprintf(file, "{%d}\n", size);
	fclose(file);
}
#endif

int main(int argc, char *argv[])
{
	DaemonConfig *cfg;
	KamstrupInfo *kamsinfo;
	IPCHandler *ipc;
	SerialPort *MySerialPort;
	KMPSendInterface *MyKMPSendClass;
	KMPReceiveInterface *MyKMPReceiveClass;
	char e_c;
	unsigned short actual_register;
	RegisterType register_type;
	double tmp_float_value;
	int tmp_int_value;
	int i;

	cfg = new DaemonConfig("k601dmn");

	if (cfg->Load(&argc, argv))
		return 1;
	kamsinfo =
	    new KamstrupInfo(cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->
			     GetParamsCount());
	if (kamsinfo->parseXML(cfg->GetXMLDevice(), cfg))
		return 1;

	if (cfg->GetSingle()) {

		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n\
Delay between chars [us]: %ld\n\
Delay between requests [s]: %d\n\
Unique registers (read params): %d\n\
", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), kamsinfo->m_params_count, kamsinfo->delay_between_chars, kamsinfo->delay_between_requests, kamsinfo->unique_registers_count);
		for (i = 0; i < kamsinfo->m_params_count; i++)
			printf("  IN:  reg %04d multiplier %ld type %s\n",
			       kamsinfo->m_params[i].reg,
			       kamsinfo->m_params[i].mul,
			       kamsinfo->m_params[i].type == T_LSB ? "lsb" :
			       kamsinfo->m_params[i].type == T_MSB ? "msb" :
			       kamsinfo->m_params[i].type ==
			       T_SINGLE ? "single" : "error");

		for (int i = 0; i < kamsinfo->unique_registers_count; i++)
			printf("unique register[%d] = %d\n", i,
			       kamsinfo->unique_registers[i]);

	}

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	sz_log(2, "starting main loop");
	MySerialPort = new SerialPort();
#ifdef STRONG_DEBUG
	MySerialPort->
	    OpenSerialPort(SC::S2A(cfg->GetDevice()->GetPath().c_str()));
#endif

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
	KMPSendCreate_t *createSend =
	    (KMPSendCreate_t *) dlsym(plugin, "CreateKMPSend");
	KMPReceiveCreate_t *createReceive =
	    (KMPReceiveCreate_t *) dlsym(plugin, "CreateKMPReceive");
	KMPSendDestroy_t *destroySend =
	    (KMPSendDestroy_t *) dlsym(plugin, "DestroyKMPSend");
	KMPReceiveDestroy_t *destroyReceive =
	    (KMPReceiveDestroy_t *) dlsym(plugin, "DestroyKMPReceive");
	if ((createSend == NULL) || (createReceive == NULL) || (destroySend == NULL)
			|| (destroyReceive == NULL)) {
		sz_log(0, "Error loading symbols frop prop-plugins.so library: %s",
				dlerror());
		dlclose(plugin);
		exit(1);
	}

	while (true) {
		for (int i = 0; i < kamsinfo->unique_registers_count; i++) {
			actual_register = kamsinfo->unique_registers[i];
			register_type =
			    kamsinfo->DetermineRegisterType(actual_register);

#ifndef STRONG_DEBUG
			MySerialPort->
			    OpenSerialPort(SC::S2A(cfg->GetDevice()->GetPath()).
					   c_str());
#endif

			MyKMPSendClass = createSend();
			MyKMPSendClass->CreateQuery(0x10, 0x3F,
						    actual_register);
			if (MySerialPort->PutFD() < 0) {
				sz_log(2, "Can't open serial port");
				return 0;
			}
			MyKMPSendClass->SendQuery(MySerialPort->PutFD());

			destroySend(MyKMPSendClass);
			MyKMPReceiveClass = createReceive();
			if (MyKMPReceiveClass->
			    ReceiveResponse(MySerialPort->PutFD(), 10000,
					    10) ==
			    MyKMPReceiveClass->GetREAD_OK()) {

				MyKMPReceiveClass->UnStuffResponse();

				if (MyKMPReceiveClass->CheckResponse() ==
				    MyKMPReceiveClass->GetRESPONSE_OK()) {

					tmp_float_value =
					    MyKMPReceiveClass->
					    ReadFloatRegister(&e_c);
					tmp_int_value =
					    (int)(tmp_float_value *
						  (double)kamsinfo->
						  DetermineRegisterMultiplier
						  (actual_register));

					if (e_c ==
					    MyKMPReceiveClass->GetCONVERT_OK()) {

						switch (register_type) {
						case RT_LSBMSB:
							ipc->m_read[kamsinfo->
								    FindRegisterIndex
								    (actual_register,
								     T_LSB)] =
							    tmp_int_value &
							    0xffff;
							ipc->m_read[kamsinfo->
								    FindRegisterIndex
								    (actual_register,
								     T_MSB)] =
							    tmp_int_value >> 16;

							break;
						case RT_SINGLE:
							ipc->m_read[kamsinfo->
								    FindRegisterIndex
								    (actual_register,
								     T_SINGLE)]
							    = tmp_int_value;
							break;
						case RT_ERROR:
							break;
						}
#ifndef STRONG_DEBUG
						if (cfg->GetSingle())
							printf("Value: %d\n",
							       tmp_int_value);
#endif
					} else {
						switch (register_type) {
						case RT_LSBMSB:
							ipc->m_read[kamsinfo->
								    FindRegisterIndex
								    (actual_register,
								     T_LSB)] =
							    SZARP_NO_DATA;
							ipc->m_read[kamsinfo->
								    FindRegisterIndex
								    (actual_register,
								     T_MSB)] =
							    SZARP_NO_DATA;
							break;
						case RT_SINGLE:
							ipc->m_read[kamsinfo->
								    FindRegisterIndex
								    (actual_register,
								     T_SINGLE)]
							    = SZARP_NO_DATA;
							break;

						case RT_ERROR:
							break;
						}
#ifndef STRONG_DEBUG
						if (cfg->GetSingle())
							printf
							    ("Value: SZARP_NO_DATA (BAD CONVERT)\n");
#endif
					}

				} else {
					switch (register_type) {
					case RT_LSBMSB:
						ipc->m_read[kamsinfo->
							    FindRegisterIndex
							    (actual_register,
							     T_LSB)] =
						    SZARP_NO_DATA;
						ipc->m_read[kamsinfo->
							    FindRegisterIndex
							    (actual_register,
							     T_MSB)] =
						    SZARP_NO_DATA;
						break;
					case RT_SINGLE:
						ipc->m_read[kamsinfo->
							    FindRegisterIndex
							    (actual_register,
							     T_SINGLE)] =
						    SZARP_NO_DATA;
						break;
					case RT_ERROR:
						break;
					}
#ifndef STRONG_DEBUG
					if (cfg->GetSingle())
						printf
						    ("Value: SZARP_NO_DATA (INCORRECT RESPONSE)\n");
#endif

				}

			} else {
				switch (register_type) {
				case RT_LSBMSB:
					ipc->m_read[kamsinfo->
						    FindRegisterIndex
						    (actual_register, T_LSB)] =
					    SZARP_NO_DATA;
					ipc->m_read[kamsinfo->
						    FindRegisterIndex
						    (actual_register, T_MSB)] =
					    SZARP_NO_DATA;
					break;
				case RT_SINGLE:
					ipc->m_read[kamsinfo->
						    FindRegisterIndex
						    (actual_register,
						     T_SINGLE)] = SZARP_NO_DATA;
					break;
				case RT_ERROR:
					break;
				}
#ifndef STRONG_DEBUG
				if (cfg->GetSingle())
					printf
					    ("Value: SZARP_NO_DATA (BAD RESPONSE, SELECT)\n");
#endif
			}

			ipc->GoParcook();
			destroyReceive(MyKMPReceiveClass);

#ifndef STRONG_DEBUG
			if (cfg->GetSingle())
				printf("\n");
			sleep(1);
#endif
#ifndef STRONG_DEBUG
			MySerialPort->CloseSerialPort();
#endif

		}

#ifndef STRONG_DEBUG
		if (!cfg->GetSingle())
			sleep(kamsinfo->delay_between_requests);
#endif
	}
#ifdef STRONG_DEBUG
	MySerialPort->CloseSerialPort();
#endif
	delete MySerialPort;
	dlclose(plugin);
	return 0;
}
