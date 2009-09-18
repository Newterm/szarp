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
 * Demon Modbus RTU do odczytu systemu monitoringu spalin.
 * 
 * $Id$
 * 
 * Konfiguracja w params.xml (przyk³ad, atrybuty z przestrzeni nazw
 * 'modbus' s± wymagane):
 * Nowy stuff, zeby bardziej zuniwersalnic demona - w trybie MASTER podaje sie numer 
 * funkcyji pytajacej
 *
 * ...
 * <device 
 *      xmlns:modbus="http://www.praterm.com.pl/SZARP/ipk-extra"
 *      daemon="/opt/szarp/bin/mbrtudmn" 
 *      path="/dev/ttyA11"
 *      modbus:id="0xA1"
 *      modbus:mode="master"
 *      <unit id="1" modbus:id="0xA11">
 *              <param
 *                      name="..."
 *                      ...
 *                      modbus:address="0x00"
 *                      modbus:type="integer">
 *                      ...
 *              </param>
 *              <param
 *                      name="..."
 *                      ...
 *                      modbus:address="0x02"
 *                      modbus:type="float">
 *                      ...
 *              </param>
 *              ...
 *              <send 
 *                      param="..." 
 *                      type="min"
 *                      modbus:address="0x1f"
 *                      modbus:type="float">
 *                      ...
 *              </send>
 *              ...
 *      </unit>
 *      <unit id="1" modbus:id="0xA12">
 *              <param
 *                      name="..."
 *                      ...
 *                      modbus:address="0x00"
 *                      modbus:type="integer">
 *                      ...
 *              </param>
 *              <param
 *                      name="..."
 *                      ...
 *                      modbus:address="0x02"
 *                      modbus:type="float">
 *                      ...
 *              </param>
 *              ...
 *              <send 
 *                      param="..." 
 *                      type="min"
 *                      modbus:address="0x1f"
 *                      modbus:type="float">
 *                      ...
 *              </send>
 *              ...
 *      </unit>
 * </device>
 *
 * Logi l±duj± domy¶lnie w /opt/szarp/log/mbrtudmn.
 * W trybie Master zostala dodana konwersja FLOAT-> parametr MSB i FLOAT-> PARAMETR LSB
 * Nowy typ danych: BCD (modbus:type="bcd")
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


#include <libxml/tree.h>
#include <vector>

#include "conversion.h"
#include "xmlutils.h"

using std::vector; 

#define _IPCTOOLS_H_
#define _HELP_H_
#define _ICON_ICON_
#include "szarp.h"
#include "msgtypes.h"

#include "mbrtu.h"
#include "ipchandler.h"
#include "liblog.h"

#define DAEMON_ERROR 1
#define NOT_FOUND -1
#define SLAVE_SEND_OK 1
#define SLAVE_RECEIVE_OK 2
#define SLAVE_OK 0;
#define SLAVE_SEND_ERROR -1
#define SLAVE_RECEIVE_ERROR -2
#define SLAVE_ERROR -3

#define PARAMS_CYCLE 0
#define SENDS_CYCLE 1

#define ENABLE 1
#define DISABLE 0

#define MSBLSB 0  /* Order in float2integer conversion */
#define LSBMSB 1

#define YES 1
#define NO 0

#define DELAY_BETWEEN_CHARS 10
#define RECEIVE_TIMEOUT 10
#define SEND_DELAY 21

/* Constans in float2msb/lsb conversion */
#define FLOAT_SINGLE -1
#define FLOAT_MSB 1
#define FLOAT_LSB 2

	/** possible types of modbus communication modes */
typedef enum {
	MB_MODE_ERR,		/**< undefined mode (config error) */
	MB_MODE_MASTER,		/**< master mode */
	MB_MODE_SLAVE,		/**< slave mode */
} ModbusMode;


typedef struct {
	unsigned short  Address;/**< Address of data */
	unsigned short  Data;/**< data */
	ModbusValType   ValueType;/**< ModbusType */
} tRegisters;


class ModbusUnit;

class ModbusLine {
	int m_fd;
	
	int m_parity;
	
	int m_stop_bits;

	bool m_debug;

	ModbusMode m_mode;

	bool m_always_init_port;

	vector<ModbusUnit*> m_units;

	IPCHandler *m_ipc;

	DaemonConfig *m_cfg;

	void PerformSlaveCycle();

	void PerformMasterCycle();

	void QuerySlaveUnit(ModbusUnit *unit);

	void PerformCycle();

	public:

	ModbusLine();

	bool ParseConfig(DaemonConfig *cfg, IPCHandler *ipc);

	void Go();
};


/**
 * Modbus communication config.
 */
class           ModbusUnit {
      public:
	/** info about single parameter */

	   class ParamInfo {
	      public:
		unsigned short  addr;	/**< parameter modbus address */
		ModbusValType   type;	/**< value type */
		int             prec;	/**< paramater precision - number by
					  which you have to divide integer
					  value to get real value; for example
					  if integer value is 131 and prec is
					  100, real value is 131/100 = 1.31 */
		int		function ; /**< function id in MODBUS standard 0..0x10 - this field is supported in MASTER mode. 
					    In slave this param is automagicaly created from MASTER frame*/
		int		extra ; /**< Extra configuration */
	};

	bool m_debug;

	int             m_id;	/**< id */
	
	short* m_reads_buffer;
				/**< array of params values*/
	ParamInfo      *m_params;
				/**< array of params to read */
	int             m_params_count;
				/**< size of params array */
	TUnit		*m_unit;
				/**< pointer to IPK unit object */

	short* m_sends_buffer;
				/**< array of sent params values*/
	ParamInfo      *m_sends;
				/**< array of params to write (send) */
	tRegisters     *MemRegisters;
				  /**< virtual memory map */
	int             m_sends_count;
				/**< size of sends array */
	char 		CheckCRC ; /** < allow to disable CRC checking */

	int 		DelayBetweenChars ; /** < Delay between received chars */
	int 		ReceiveTimeout ; /** < Select timeout error */
	
	int 		SendDelay ; /** < Sending delay */
	
	char 		zerond; /** < convert Zero to SZARP_NO_DATA? */
	
	char 		FloatOrder; /** < In float conversion MSBLSB or LSBMSB */
	char 		LongOrder; /** < In float conversion MSBLSB or LSBMSB */
	char		WriteOneTime ;	/** < Sends data only One time (required for CRP05 Heatmeter) */
	char		WriteOneTimeLatch ;
	char		WriteOneTimeLatch2 ;

	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	/* This is an orginal constructor */
	ModbusUnit(TUnit *unit): m_unit(unit)
       	{
		assert(m_unit != NULL);
		m_params_count = m_unit->GetParamsCount();
		if (m_params_count > 0) {
			m_params = new ParamInfo[m_params_count];
			assert(m_params != NULL);
		} else
			m_params = NULL;

		m_sends_count = m_unit->GetSendParamsCount();
		if (m_sends_count > 0) {
			m_sends = new ParamInfo[m_sends_count];
			assert(m_sends != NULL);
		} else
			m_sends = NULL;
		/* Ustalam cykl tej zmiennej rowny sumie parametrow wysylanych i odczytywanych */
		ReqCycle = 0 ;
		StatusCycle = PARAMS_CYCLE ;
		CheckCRC = ENABLE ;
		FloatOrder = MSBLSB ;
		LongOrder = MSBLSB ;
		WriteOneTime = DISABLE ;
		WriteOneTimeLatch = NO ;
		WriteOneTimeLatch2 = NO ;

		DelayBetweenChars = DELAY_BETWEEN_CHARS ; /* ms */
		ReceiveTimeout = RECEIVE_TIMEOUT ; /* s */
		SendDelay = SEND_DELAY ; /* ms */
		zerond = NO ;
	}

	~ModbusUnit() {
		delete          m_params;
		delete          m_sends;

		free(MemRegisters);
	}

	/**
	 * finding memory index
	 * @param Address finding Address 
	 * @return -1 error, other value found index
	 */
	int             FindRegIndex(unsigned short Address);

	/**
	 * Writing one variable to memory registers map
	 *@param Data - Data to write
	 *@param Address - Address to write
	 * @return -1 error, other value found index
	 */
	int             Write1Data(unsigned short Data,
				   unsigned short Address);

	/**
	 * Reading one variable to memory registers map
	 *@param &Data - Value to read
	 *@param Address - Address to write
	 * @return -1 error, other value found index
	 */
	int             Read1Data(unsigned short *Data,
				  unsigned short Address);

	/**
	 * Writing one variable to memory registers map
	 *@param Data - Value to write
	 *@param Address - Address to write
	 * @return -1 error, other value found index
	 */
	int             Write2Data(unsigned short bMSB, unsigned short bLSB,
				   unsigned short Address);

	/**
	 * Reading one variable to memory registers map
	 *@param &Data - Value to read
	 *@param Address - Address to write
	 * @return -1 error, other value found index
	 */
	int             Read2Data(unsigned short *bMSB, unsigned short *bLSB,
				  unsigned short Address);

	/**
	 * Filling m_read structure SZARP_NO_DATA value
	 */
	void            SetNoData();
	
	/**
	 * Filling current element m_read SZARP_NO_DATA value
	 */
	void            SetNoData(int param);

	/** 
	 * Parses XML 'device' element. Device element must have attributes
	 * id' (integer identifier, may be
	 * decimal or hex). Every 'param' and 'send' children of first 'unit'
	 * element must have attributes 'address' (modebus address, decimal or
	 * hex) and 'val_type' ("integer" or "float").
	 * @param node XML 'device' element
	 * @param ipk pointer to daemon config object
	 * @return - 0 on success, 1 on error 
	 */

	int parseUnitConfig(xmlNodePtr unit_node, xmlNodePtr device_node);

	bool Configure(short *read_buffer, 
			short *send_buffer, 
			xmlNodePtr unit_node, 
			xmlNodePtr device_node,
			DaemonConfig *cfg, 
			ModbusMode mode);
	/**
	 * Parses 'param' and 'send' children of 'unit' element,
	 * fills in params and sends arrays.
	 * @param unit XML 'unit' element
	 * @param ipk pointer to daemon config object
	 * @return 0 in success, 1 on error
	 */
	int             parseParams(xmlNodePtr unit, DaemonConfig * cfg, ModbusMode mode);

	/**
	 * Act as a master - send packet to slave using data from
	 * m_sends_buffer. m_reads_buffer with SZARP_NO_DATA.
	 * @param fd descriptor of port to write to
	 * @param ipc IPCHandler object (contains data to send)
	 * @return 0 on success, 1 on error; on error no response 
	 * is expected
	 */
	int             MasterAsk(int fd);

	/** Get response from slave. Uses NO_WAIT, raises error when no
	 * data is ready. Fills in m_reads_buffer.
	 * @param fd descriptor of port to read from
	 * @return 0 on success, 1 on error
	 */
	int             MasterGet(int fd);

	/** Waits for packet from master. Puts data from master into
	 * m_reads_buffer table. Returns only when packet is received or fatal
	 * error occured.
	 * @param fd descriptor of port to read from
	 */
	int             SlaveWait(int fd);

	/**
	 * Send response to master using data from m_sends_buffer array. 
	 * Fills in m_reads_buffer with SZARP_NO_DATA.
	 * @param fd descriptor of port to write to
	 * @param ipc IPCHandler object (contains data to send)
	 */
	int             SlaveReply(int fd);


	/**
	 * Update Sending CYCLE in master mode
	 *
	 *
	 */
	void		UpdateReqCycle();
      private:
	tWMasterFrame iMasterFrame; /**<Packet from Master to Slave*/
	tRMasterFrame   oMasterFrame;
				    /**<Recieved master packet from master for slave*/
		    /**<Error counter to avoid overflow or HDD space by generated logs ;) */
	unsigned short  tmpAddress;
	char            CRCStatus;
	int             MemRegistersPtr;	/* main pointer to mem
						 * registers */
	int             PSRegPtr;	/* auxilary pointer to Memregisters */
	int ReqCycle ;/**<Cykl przechodzenia po zapytaniach*/
	int StatusCycle ; /**<Aktualny cykl przechodzenia po zapytaniach*/ 
};

int ModbusUnit::parseUnitConfig(xmlNodePtr unit_node, xmlNodePtr device_node)
{
	char *str;

#define GET_ATTRIBUTE(ATTR_NAME) 							\
	str = (char*) xmlGetNsProp(unit_node,						\
				    BAD_CAST(ATTR_NAME),				\
				    BAD_CAST(IPKEXTRA_NAMESPACE_STRING));		\
											\
	if (str == NULL) {								\
		str = (char*) xmlGetNsProp(device_node,					\
					    BAD_CAST(ATTR_NAME),			\
					    BAD_CAST(IPKEXTRA_NAMESPACE_STRING));	\
	}

	GET_ATTRIBUTE("CheckCRC");
	if (str == NULL) {
		sz_log(1, "attribute modbus:CheckCRC not found in device element, line %ld",
		    xmlGetLineNo(unit_node));
		CheckCRC = ENABLE;
	} else if (strcmp(str, "enable") == 0)
		CheckCRC = ENABLE;
	else if (strcmp(str, "disable") == 0)
		CheckCRC = DISABLE;
	else {
		sz_log(0,
		    "invalid modbus:CheckCRC attribute (must be 'enable' or 'disable'), line %ld",
		    xmlGetLineNo(unit_node));
		free (str) ;
		return 3 ;
	}
	free(str);

	GET_ATTRIBUTE("zerond")
	if (str == NULL) {
		sz_log(1, "attribute modbus:zerond not found in device element, line %ld",
		    xmlGetLineNo(unit_node));
		zerond = NO;
	} else if (strcmp(str, "yes") == 0)
		zerond = YES;
	else if (strcmp(str, "no") == 0)
		zerond = NO;
	else {
		sz_log(0,
		    "invalid modbus:zerond attribute (must be 'yes' or 'no'), line %ld",
		    xmlGetLineNo(unit_node));
		free (str) ;
		return 4 ;
	}
	free(str);

	GET_ATTRIBUTE("FloatOrder")
	if (str == NULL) {
		sz_log(1, "attribute modbus:FloatOrder not found in device element, line %ld",
		    xmlGetLineNo(unit_node));
		FloatOrder = MSBLSB;
	} else if (strcmp(str, "msblsb") == 0)
		FloatOrder = MSBLSB;
	else if (strcmp(str, "lsbmsb") == 0)
		FloatOrder = LSBMSB;
	else {
		sz_log(0,
		    "invalid modbus:FloatOrder attribute (must be 'msblsb' or 'lsbmsb'), line %ld",
		    xmlGetLineNo(unit_node));
		return 5 ;
	}
	free(str);

	GET_ATTRIBUTE("LongOrder")
	if (str == NULL) {
		sz_log(1, "attribute modbus:LongOrder not found in device element, line %ld",
		    xmlGetLineNo(unit_node));
		LongOrder = MSBLSB;
	} else if (strcmp(str, "msblsb") == 0)
		LongOrder = MSBLSB;
	else if (strcmp(str, "lsbmsb") == 0)
		LongOrder = LSBMSB;
	else {
		sz_log(0,
		    "invalid modbus:LongOrder attribute (must be 'msblsb' or 'lsbmsb'), line %ld",
		    xmlGetLineNo(unit_node));
		return 6 ;
	}
	free(str);

	GET_ATTRIBUTE("id")
	char *tmp;
	m_id = strtol(str, &tmp, 0);
	if (tmp[0] != 0) {
		sz_log(0, "error parsing modbus:id attribute ('%s'), line %ld",
		    str, xmlGetLineNo(unit_node));
		free(str);
		return 9;
	}
	free(str);


	GET_ATTRIBUTE("DelayBetweenChars")
	if (str == NULL) {
		sz_log(1, "attribute modbus:DelayBetweenChars not found in device element, line %ld",
		    xmlGetLineNo(unit_node));
		DelayBetweenChars = DELAY_BETWEEN_CHARS ;
	} else{

		DelayBetweenChars = strtol(str, &tmp, 0);
		if (tmp[0] != 0) {
			sz_log(0, "error parsing modbus:DelayBetweenChars attribute ('%s'), line %ld",
		    	str, xmlGetLineNo(unit_node));
			free(str);
			return 10;
		}
		free(str);
	}

	GET_ATTRIBUTE("ReceiveTimeout")
	if (str == NULL) {
		sz_log(1, "attribute modbus:ReceiveTimeout not found in device element, line %ld",
		    xmlGetLineNo(unit_node));
		ReceiveTimeout = RECEIVE_TIMEOUT ;
	}
	else{
		ReceiveTimeout = strtol(str, &tmp, 0);
		if (tmp[0] != 0) {
			sz_log(0, "error parsing modbus:receiveTimeout attribute ('%s'), line %ld",
			    str, xmlGetLineNo(unit_node));
			free(str);
			return 11;
		}
		free(str);
	}


	GET_ATTRIBUTE("SendDelay")
	if (str == NULL) {
		sz_log(1, "attribute modbus:SendDelay not found in device element, line %ld",
			xmlGetLineNo(unit_node));
		SendDelay = SEND_DELAY ;
	} else {

		SendDelay = strtol(str, &tmp, 0);
		if (tmp[0] != 0) {
			sz_log(0, "error parsing modbus:SendDelay attribute ('%s'), line %ld",
			    str, xmlGetLineNo(unit_node));
			free(str);
			return 12;
		}
		free(str);
	}


	GET_ATTRIBUTE("WriteOneTime")
	if (str == NULL) {
		sz_log(1, "attribute modbus:WriteOneTime not found in device element, line %ld",
		    xmlGetLineNo(unit_node));
		WriteOneTime = DISABLE;
	} else if (strcmp(str, "disable") == 0)
		WriteOneTime = DISABLE;
	else if (strcmp(str, "enable") == 0)
		WriteOneTime = ENABLE;
	else {
		sz_log(1,
		    "invalid modbus:WriteOneTime attribute (must be 'disable' or 'enable'), line %ld",
		    xmlGetLineNo(unit_node));
		return 6 ;
	}
	free(str);


	return 0;
}


int ModbusUnit::parseParams(xmlNodePtr unit, DaemonConfig * cfg, ModbusMode mode)
{
	int params_found = 0;
	int sends_found = 0;
	char           *str;
	char           *tmp;

	for (xmlNodePtr node = unit->children; node; node = node->next) {
		if (node->ns == NULL)
			continue;
		if (strcmp((char *) node->ns->href, (char *) SC::S2U(IPK_NAMESPACE_STRING).c_str()) !=
		    0)
			continue;
		if (strcmp((char *) node->name, "param") &&
		    strcmp((char *) node->name, "send"))
			continue;

		int addr;
		int function = 0 ; // function
		ModbusValType type;
		int extra = FLOAT_SINGLE ;
		int prec = 1;

		/* W trybie master trzeba podac numer funkcji, ktora bedziemy wysylac zapytanie */
		if (mode == MB_MODE_MASTER){
				/* Parsing MSB/LSB Extra */
				extra = FLOAT_SINGLE ;
				str = (char *) xmlGetNsProp(node, BAD_CAST("extra"),
						    BAD_CAST
						    (IPKEXTRA_NAMESPACE_STRING));
				if (str == NULL) {
					sz_log(1, "attribute modbus:extra not found (line %ld)",
					    xmlGetLineNo(node));
			} else {
				sz_log(10, "extra value: %s", str);
				if (!strcmp(str, "msb")){
					extra = FLOAT_MSB;
				} else if (!strcmp(str, "lsb")){
					extra = FLOAT_LSB;
				} else {
					sz_log(0,
					    "unknown extra '%s' for attribute modbus:extra - should be 'msb' or 'lsb' or none (line %ld)",
					    str, xmlGetLineNo(node));
					free(str);
					return 1;
				}
			}
			free(str);
		
			str = (char *) xmlGetNsProp(node, BAD_CAST("function"),
						    BAD_CAST
						    (IPKEXTRA_NAMESPACE_STRING));
			if (str == NULL) {
				sz_log(0,
				    "attribute modebus:function not found (line %ld)",
				    xmlGetLineNo(node));
				free(str);
				return 1;
			}
			
			function = strtol(str, &tmp, 0);
			if (tmp[0] != 0) {
				sz_log(0,
				    "error parsing modebus:function attribute value ('%s') - line %ld",
				    str, xmlGetLineNo(node));
				free(str);
				return 1;
			}
			free(str);
			if ((function < 0) || (function > 0x10)) {
				sz_log(0,
				    "modebus:function is not compatible with MODBUS standard (line %ld)",
				    xmlGetLineNo(node));
				return 1;
	
			}

		}

		str = (char *) xmlGetNsProp(node, BAD_CAST("address"),
					    BAD_CAST
					    (IPKEXTRA_NAMESPACE_STRING));
		if (str == NULL) {
			sz_log(0,
			    "attribute modebus:address not found (line %ld)",
			    xmlGetLineNo(node));
			free(str);
			return 1;
		}
		addr = strtol(str, &tmp, 0);
		if (tmp[0] != 0) {
			sz_log(0,
			    "error parsing modebus:address attribute value ('%s') - line %ld",
			    str, xmlGetLineNo(node));
			free(str);
			return 1;
		}
		free(str);
		if (addr > MB_MAX_ADDRESS) {
			sz_log(0,
			    "modebus:address attribute value to big (line %ld)",
			    xmlGetLineNo(node));
			return 1;
		}
		sz_log(10, "Address: %d", addr);

		str = (char *) xmlGetNsProp(node, BAD_CAST("val_type"),
					    BAD_CAST
					    (IPKEXTRA_NAMESPACE_STRING));
		if (str == NULL) {
			sz_log(0,
			    "attribute modebus:val_type not found (line %ld)",
			    xmlGetLineNo(node));
			free(str);
			return 1;
		}
		sz_log(10, "Value type: %s", str);
		if (!strcmp(str, "integer")) {
			type = MB_TYPE_INT;
		} else if (!strcmp(str, "float")) {
			type = MB_TYPE_FLOAT;
		} else if (!strcmp(str, "long")) {
			type = MB_TYPE_LONG;
		} else if (!strcmp(str, "bcd")) {
			type = MB_TYPE_BCD;
		} else {
			sz_log(0,
			    "unknown value '%s' for attribute modbus:val_type - should be 'bcd', 'integer', 'long' or 'float' (line %ld)",
			    str, xmlGetLineNo(node));
			free(str);
			return 1;
		}
		free(str);

		if (!strcmp((char *) node->name, "param")) {
			/*
			 * for 'param' element 
			 */
			assert(params_found < m_params_count);
			m_params[params_found].addr = addr;
			m_params[params_found].type = type;
			m_params[params_found].function = function;
			m_params[params_found].extra = extra;
			
			TParam         *p =
				m_unit->GetFirstParam()->
				GetNthParam(params_found);
			assert(p != NULL);
			for (int i = 0; i < p->GetPrec(); i++)
				prec *= 10;
			m_params[params_found].prec = prec;
			params_found++;
		} else {
			/*
			 * for 'send' element 
			 */
			assert(sends_found < m_sends_count);
			m_sends[sends_found].addr = addr;
			m_sends[sends_found].type = type;
			m_sends[sends_found].function = function;
			TSendParam     *s =
				m_unit->GetFirstSendParam()->
				GetNthParam(sends_found);
			assert(s != NULL);
			std::wstring c = s->GetParamName();

			if (!c.empty()) {
				TParam         *p =
					cfg->GetIPK()->getParamByName(c);
				assert(p != NULL);
				for (int i = 0; i < p->GetPrec(); i++)
					prec *= 10;
			}
			m_sends[sends_found].prec = prec;
			sends_found++;
		}
	}

	/*
	 * buildnig virtual memory map registers 
	 */
	MemRegistersPtr = 0;
	/*
	 * Calculating memory space for memory MAP 
	 */
	for (int i = 0; i < this->m_params_count; i++) {
		switch (this->m_params[i].type) {
			case MB_TYPE_INT:
			case MB_TYPE_BCD:
				MemRegistersPtr++;
				break;
			case MB_TYPE_FLOAT:
				MemRegistersPtr += 2;
				break;
			case MB_TYPE_LONG:
				MemRegistersPtr += 2;
				break;
			case MB_TYPE_ERR:
				return DAEMON_ERROR;
				break;
			default:
				return DAEMON_ERROR;
				break;
		}
	}

	for (int i = 0; i < this->m_sends_count; i++) {
		switch (this->m_sends[i].type) {
			case MB_TYPE_INT:
			case MB_TYPE_BCD:
				MemRegistersPtr++;
				break;
			case MB_TYPE_FLOAT:
				MemRegistersPtr += 2;
				break;
			case MB_TYPE_LONG:
				MemRegistersPtr += 2;
				break;
			case MB_TYPE_ERR:
				return DAEMON_ERROR;

				break;
			default:
				return DAEMON_ERROR;
				break;
		}
	}

	MemRegisters =
		(tRegisters *) calloc(MemRegistersPtr, sizeof(tRegisters));
	/*
	 * Rewritting memory adresses and cleaning structures
	 */

	PSRegPtr = 0;
	for (int i = 0; i < this->m_sends_count; i++) {
		MemRegisters[PSRegPtr].ValueType = m_sends[i].type;

		switch (this->m_sends[i].type) {
			case MB_TYPE_INT:
			case MB_TYPE_BCD:
				MemRegisters[PSRegPtr].Address =
					this->m_sends[i].addr;

				PSRegPtr++;
				break;
			case MB_TYPE_FLOAT:
				MemRegisters[PSRegPtr].Address =
					this->m_sends[i].addr;
				PSRegPtr++;
				MemRegisters[PSRegPtr].Address =
					this->m_sends[i].addr + 1;
				MemRegisters[PSRegPtr].ValueType =
					m_sends[i].type;

				PSRegPtr++;
				break;
			case MB_TYPE_LONG:
				MemRegisters[PSRegPtr].Address =
					this->m_sends[i].addr;
				PSRegPtr++;
				MemRegisters[PSRegPtr].Address =
					this->m_sends[i].addr + 1;
				MemRegisters[PSRegPtr].ValueType =
					m_sends[i].type;
				PSRegPtr++;
				break;
			case MB_TYPE_ERR:
				return DAEMON_ERROR;
				break;
			default:
				return DAEMON_ERROR;
				break;
		}
	}


	for (int i = 0; i < this->m_params_count; i++) {
		MemRegisters[PSRegPtr].ValueType = m_params[i].type;
		switch (this->m_params[i].type) {
			case MB_TYPE_INT:
			case MB_TYPE_BCD:
				MemRegisters[PSRegPtr].Address =
					this->m_params[i].addr;
				PSRegPtr++;
				break;
			case MB_TYPE_FLOAT:
				MemRegisters[PSRegPtr].Address =
					this->m_params[i].addr;
				PSRegPtr++;
				MemRegisters[PSRegPtr].Address =
					this->m_params[i].addr + 1;
				MemRegisters[PSRegPtr].ValueType =
					m_params[i].type;
				PSRegPtr++;
				break;
			case MB_TYPE_LONG:
				MemRegisters[PSRegPtr].Address =
					this->m_params[i].addr;
				PSRegPtr++;
				MemRegisters[PSRegPtr].Address =
					this->m_params[i].addr + 1;
				MemRegisters[PSRegPtr].ValueType =
					m_params[i].type;
				PSRegPtr++;
				break;
			case MB_TYPE_ERR:
				return DAEMON_ERROR;
				break;
			default:
				return DAEMON_ERROR;
				break;
		}
	}

	for (int i = 0; i < MemRegistersPtr; i++) {
		MemRegisters[i].Data = 0;
	}

	/*
	 * end of building memory registers 
	 */

	assert(params_found == m_params_count);
	assert(sends_found == m_sends_count);

	return 0;
}

int ModbusUnit::FindRegIndex(unsigned short Address)
{
	int i;

	for (i = 0; i < MemRegistersPtr; i++) {
		if (MemRegisters[i].Address == Address)
			return i;
	}
	return NOT_FOUND;
}

int ModbusUnit::Write1Data(unsigned short Data, unsigned short Address)
{
	int result;

	result = FindRegIndex(Address);
	if (result == NOT_FOUND)
		return NOT_FOUND;
	MemRegisters[result].Data = Data;
	return result;
}

int ModbusUnit::Read1Data(unsigned short *Data, unsigned short Address)
{
	int result;

	result = FindRegIndex(Address);
	if (result == NOT_FOUND)
		return NOT_FOUND;
	*Data = MemRegisters[result].Data;
	return result;
}

int ModbusUnit::Write2Data(unsigned short bMSB, unsigned short bLSB,
			   unsigned short Address)
{
	int result;

	result = FindRegIndex(Address);
	if (result == NOT_FOUND)
		return NOT_FOUND;
	MemRegisters[result].Data = bMSB;
	MemRegisters[result + 1].Data = bLSB;
	return result;
}

int ModbusUnit::Read2Data(unsigned short *bMSB, unsigned short *bLSB,
			  unsigned short Address)
{
	int result;

	result = FindRegIndex(Address);
	if (result == NOT_FOUND)
		return NOT_FOUND;
	*bMSB = MemRegisters[result].Data;
	*bLSB = MemRegisters[result + 1].Data;
	return result;
}

void ModbusUnit::SetNoData()
{
	for (int i = 0; i < m_params_count; i++)
		m_reads_buffer[i] = SZARP_NO_DATA;
}

void ModbusUnit::SetNoData(int param)
{
	m_reads_buffer[param] = SZARP_NO_DATA;
}

bool ModbusUnit::Configure(short *read_buffer, 
		short *send_buffer, 
		xmlNodePtr unit_node, 
		xmlNodePtr device_node,
		DaemonConfig *cfg, 
		ModbusMode mode)
{
	m_reads_buffer = read_buffer;
	m_sends_buffer = send_buffer;

	/*
	 * parse unit configuration
	 */
	if (parseUnitConfig(unit_node, device_node))
		return false;

	/*
	 * parse 'param' and 'send' elements 
	 */
	if (parseParams(unit_node, cfg, mode))
		return false;

	m_debug = cfg->GetSingle() || cfg->GetDiagno();

	return true;
}


int ModbusUnit::MasterAsk(int fd)
{
//	int FrameSize = 0;	// Frame data size
	unsigned short bMSB, bLSB;
	int SendsPtr = 0;
	sz_log(10, "calling MasterAsk()");
	switch (StatusCycle){
		case PARAMS_CYCLE:
			/* W tym trybie trzeba jedynie wyslac zapytanie do slave */
			iMasterFrame.DeviceId = m_id;
			iMasterFrame.FunctionId = m_params[ReqCycle].function ;
			iMasterFrame.Address = m_params[ReqCycle].addr ;
			switch (m_params[ReqCycle].type) {
				case MB_TYPE_INT:
				case MB_TYPE_BCD:
					iMasterFrame.DataSize = 1 ; /* Tylko 1 */
				break;
				case MB_TYPE_FLOAT:
					iMasterFrame.DataSize = 2 ; /* Tylko 2 */
				break;
				case MB_TYPE_LONG:
					iMasterFrame.DataSize = 2 ; /* Tylko 2 */
				break;

				case MB_TYPE_ERR:
					iMasterFrame.DataSize = 0 ;
				break;

			}
		break;

		case SENDS_CYCLE:
			SendsPtr = ReqCycle - m_params_count;
			iMasterFrame.DeviceId = m_id;
			iMasterFrame.FunctionId = m_sends[SendsPtr].function ; /*  was ReqCycle */
			iMasterFrame.Address = m_sends[SendsPtr].addr ; /* was ReqCycle  */
			switch (m_sends[SendsPtr].type) {
				case MB_TYPE_INT:
					iMasterFrame.DataSize = 1 ; /* Tylko 1 */
					iMasterFrame.Body[0] = (signed short) m_sends_buffer[SendsPtr];
					break;
				case MB_TYPE_BCD:
					iMasterFrame.DataSize = 1 ; /* Tylko 1 */
					int err;
					iMasterFrame.Body[0] = int2bcd(m_sends_buffer[SendsPtr], &err);
					if (err) {
						sz_log(1, "Error converting value %d to BCD",
								m_sends_buffer[SendsPtr]);
						return 1;
					}
				case MB_TYPE_FLOAT:
					iMasterFrame.DataSize = 2 ; /* Tylko 2 */
					float2bin(int2float (m_sends_buffer[SendsPtr], m_sends[SendsPtr].prec), &bMSB, &bLSB);
					iMasterFrame.Body[0] = bMSB;
					/* Tu chyba powinno byc 1 */
					iMasterFrame.Body[1] = bLSB;
				break;
				case MB_TYPE_LONG:
					iMasterFrame.DataSize = 2 ; /* Tylko 2 */
					iMasterFrame.Body[0] = (signed short) m_sends_buffer[SendsPtr];
					/* Tu chyba powinno byc 1 */
					iMasterFrame.Body[1] = 0; /* Na razie nie mozna wysylac wiecej niz pojedynczy int */
				break;

				case MB_TYPE_ERR:
					iMasterFrame.DataSize = 0 ;
				break;

			}
		break;
	}
	
	if (m_debug) {
		if (StatusCycle==PARAMS_CYCLE){
			fprintf(stderr,"DEBUG[]: PARAMS CYCLE: %d\n",ReqCycle);
		}
		if (StatusCycle==SENDS_CYCLE){
			fprintf(stderr,"DEBUG[]: SENDS CYCLE: %d\n",SendsPtr);
		}
	}

	usleep(SendDelay * 1000) ;
	SendMasterPacket(fd, iMasterFrame);
	return 0 ;
}

int ModbusUnit::MasterGet(int fd)
{
	tRSlaveFrame SlaveFrame;
	signed short fi = 0;
	float f = 0 ;
	int fii = 0 ;
	unsigned short bMSB, bLSB;
	short lMSB,lLSB ; /* Zmienne ze znakiem */
	int status ;
	sz_log(10, "calling MasterGet()");

	status = ReceiveSlavePacket(fd, &SlaveFrame, NULL, ReceiveTimeout,DelayBetweenChars);  
	if ((status == GENERAL_ERROR) || (status == TIMEOUT_ERROR)){
		if (status==GENERAL_ERROR){
			if (m_debug) {
				fprintf(stderr,"DEBUG[]: select zwróci³ GENERAL_ERROR\n");
			}
			if (StatusCycle == PARAMS_CYCLE)
				SetNoData(ReqCycle);
		}

		if (status==TIMEOUT_ERROR){
			if (m_debug) 
				fprintf(stderr,"DEBUG[]: select zwróci³ TIMEOUT_ERROR\n");
			if (StatusCycle==PARAMS_CYCLE)
					SetNoData(ReqCycle);
		}

		
		/* Konczymy i wychodzimy */
		UpdateReqCycle() ;
		return status;
	}
	switch (StatusCycle){
		case PARAMS_CYCLE:
			m_reads_buffer[ReqCycle] = SZARP_NO_DATA ;
			/* Trzeba sparsowac co SLAVE przyslal */			
			/* Zalozenie: przy zamianie float 2 MSB,LSB 2 parametry maja ten sam adres i sa obok siebie */
			
			switch (this->m_params[ReqCycle].type) {
				case MB_TYPE_INT:
					bin2int(SlaveFrame.Body[0], &fi);
					m_reads_buffer[ReqCycle] = fi ;
					break;
				case MB_TYPE_BCD:
					int err;
					m_reads_buffer[ReqCycle] = bcd2int(SlaveFrame.Body[0], &err);
					if (err) {
						sz_log(1, "Error parsing BCD value %x", 
								SlaveFrame.Body[0]);
						m_reads_buffer[ReqCycle] = SZARP_NO_DATA;
					}
					break;
				case MB_TYPE_FLOAT:
					switch (FloatOrder){
						case MSBLSB:
						bMSB =  SlaveFrame.Body[0] ; 
						bLSB =  SlaveFrame.Body[1] ; 
						break;
						case LSBMSB:
						bLSB =  SlaveFrame.Body[0] ; 
						bMSB =  SlaveFrame.Body[1] ; 
						break;
						default:
						bMSB =  SlaveFrame.Body[0] ; 
						bLSB =  SlaveFrame.Body[1] ; 
						break;
					}
					if (m_params[ReqCycle].extra==FLOAT_SINGLE){
						bin2float(bMSB, bLSB,&f);
						m_reads_buffer[ReqCycle] = float2int(f,m_params[ReqCycle].prec);
					}
				
					if ((m_params[ReqCycle].extra==FLOAT_MSB) || (m_params[ReqCycle].extra==FLOAT_LSB)){
						bin2float(bMSB, bLSB,&f);
						/*fprintf(stderr,"Wartosc orginalna: %f\n",f);*/
						fii = float2int4(f,m_params[ReqCycle].prec);
						/*fprintf(stderr,"Wartosc zlozona: %d\n",fii);*/
						lMSB = (int)(fii >> 16) ;
						lLSB = (int)(fii & 0xffff) ;
						if (m_params[ReqCycle].extra == FLOAT_MSB){
							m_reads_buffer[ReqCycle] = lMSB ;
							ReqCycle++;
							m_reads_buffer[ReqCycle] = lLSB ;
						} else if (m_params[ReqCycle].extra == FLOAT_LSB){
							m_reads_buffer[ReqCycle] = lLSB ;
							ReqCycle++;
							m_reads_buffer[ReqCycle] = lMSB ;
						}
					
					}

				break;

				case MB_TYPE_LONG:
					switch (LongOrder){
						case MSBLSB:
						bMSB =  SlaveFrame.Body[0] ; 
						bLSB =  SlaveFrame.Body[1] ; 
						break;
						case LSBMSB:
						bLSB =  SlaveFrame.Body[0] ; 
						bMSB =  SlaveFrame.Body[1] ; 
						break;
						default:
						bMSB =  SlaveFrame.Body[0] ; 
						bLSB =  SlaveFrame.Body[1] ; 
						break;
					}
					if (m_params[ReqCycle].extra==FLOAT_SINGLE){
						m_reads_buffer[ReqCycle] = bMSB;
					}
				
					if ((m_params[ReqCycle].extra==FLOAT_MSB) || (m_params[ReqCycle].extra==FLOAT_LSB)){
						/*fprintf(stderr,"Wartosc zlozona: %d\n",fii);*/
						if (m_params[ReqCycle].extra == FLOAT_MSB){
							m_reads_buffer[ReqCycle] = bMSB ;
							ReqCycle++;
							m_reads_buffer[ReqCycle] = bLSB ;
						}else
						if (m_params[ReqCycle].extra == FLOAT_LSB){
							m_reads_buffer[ReqCycle] = bLSB ;
							ReqCycle++;
							m_reads_buffer[ReqCycle] = bMSB ;
						}
					
					}
						
				break;
					case MB_TYPE_ERR:
				break;
			}

		break;

		case SENDS_CYCLE:
			/* Olewam nie przejmuje sie co sie dzieje tutej - po co mi to wiedziec */

		break;
	}

	/* To oczywiscie na samym koncu */
	UpdateReqCycle() ;
	return 0 ;
}

int ModbusUnit::SlaveWait(int fd)
{
	sz_log(10, "calling SlaveWait()");
	return ReceiveMasterPacket(fd, 
			&oMasterFrame, 
			&CRCStatus,
			ReceiveTimeout,
			DelayBetweenChars);
}

/*
 * Debug 
 */
/*FILE           *debag; */

/*
 * //Debug 
 */
int ModbusUnit::SlaveReply(int fd)
{
	sz_log(10, "calling SlaveReply");
	int i, j, IndexData;
	unsigned short AddressPtr;
	unsigned short SB;
	unsigned short bMSB, bLSB;
	char found;
	char NotFound;
	float f = 0;
	signed short fi = 0;
	tWErrorFrame iErrorFrame;
	tWSlaveFrame iSlaveFrame;

	if (oMasterFrame.DeviceId != m_id)
		return SLAVE_ERROR;	// Is packet to us? 

	if ((CRCStatus != CRC_OK) && (CheckCRC == ENABLE)) {	// Chcecking CRC
		iErrorFrame.DeviceId = m_id;
		iErrorFrame.FunctionId = oMasterFrame.FunctionId;
		iErrorFrame.Exception = MB_MEMORY_PARITY_ERROR;
		usleep(SendDelay * 1000);
		if (m_debug)
			fprintf(stderr,"DEBUG[]: MB_MEMORY_PARITY_ERROR\n");
		SendErrorPacket(fd, iErrorFrame);
		return SLAVE_ERROR;
	}
	
	if ((CRCStatus != CRC_OK) && (CheckCRC == DISABLE)) {	// Chcecking CRC
		return SLAVE_ERROR;
	}

	
	if ((oMasterFrame.FunctionId != MB_F_RHR)
	    && (oMasterFrame.FunctionId != MB_F_WMR)) {
		// Only functions Read Holding Register (0x03) and Write
		// Multiple Register (0x10) are supported
		iErrorFrame.DeviceId = m_id;
		iErrorFrame.FunctionId = oMasterFrame.FunctionId;
		iErrorFrame.Exception = MB_ILLEGAL_FUNCTION;
		usleep(SendDelay * 1000);
		if (m_debug)
			fprintf(stderr,"DEBUG[]: MB_ILLEGAL_FUNCTION\n");
		SendErrorPacket(fd, iErrorFrame);
		return SLAVE_ERROR;
	}

	/*
	 * Searching Addresses to send 
	 */
	if (oMasterFrame.FunctionId == MB_F_RHR) {	/* Read Holding Register */
		if (m_debug)
			fprintf(stderr,"DEBUG[]: read holding register\n");
		found = FALSE;
		j = 0;
		IndexData = 0;

		/*
		 * Tutaj umieszczam kawalek do kopiowania parametrow z
		 * sendera do pamieci zywcem
		 */
		/*
		 * Inicjalizacja 
		 */
		/*
		 * Przepisywanie czesci SEND 
		 */
		for (i = 0; i < m_sends_count; i++) for (j = 0; j < MemRegistersPtr; j++) {
			if (m_sends[i].addr != MemRegisters[j].Address) 
				continue;	
			// MemRegisters[j].ValueType =
			// m_sends[i].type; //To jest chyba
			// zbedne po 1349 modyfikacji
			switch (m_sends[i].type) {
				case MB_TYPE_INT:
					if (m_sends_buffer[i] != SZARP_NO_DATA) {
						int2bin(m_sends_buffer[i], &SB);
					} else {
						SB = 0;
					}
					Write1Data(SB, MemRegisters[j].Address);
					break;
				case MB_TYPE_BCD:
					if (m_sends_buffer[i] != SZARP_NO_DATA) {
						int err;
						SB = int2bcd(m_sends_buffer[i], &err);
						if (err) {
							sz_log(1, "Error converting value %d to BCD",
									m_sends_buffer[i]);
							SB = 0;
						}
					} else {
						SB = 0;
					}
					Write1Data(SB, MemRegisters[j].Address);
					break;
				case MB_TYPE_FLOAT:
/*					fprintf(stderr,"m_send[%d] %d\n",i,ipc->m_send[i]);*/
					if (m_sends_buffer[i] != SZARP_NO_DATA) {
						switch (FloatOrder){
							case MSBLSB:
								float2bin(
									int2float(m_sends_buffer[i], m_sends[i].prec),
									&bMSB,
								 	&bLSB);
								break;
							case LSBMSB:
								float2bin(
									int2float(m_sends_buffer[i], m_sends[i].prec),
								 	&bLSB,
								 	&bMSB);
								break;
							default:
								float2bin(
									int2float(m_sends_buffer[i], m_sends[i].prec),
								 	&bMSB,
								 	&bLSB);
								break;
						}
					} else {
						switch (FloatOrder) {
							case MSBLSB:
								float2bin(
									int2float(0,0),
									&bMSB,
									&bLSB);
								break;
							case LSBMSB:
								float2bin(
									int2float(0,0),
									 &bLSB,
									 &bMSB);
								break;
							default:
								float2bin(int2float(0,0),
									 &bMSB,
									 &bLSB);
							break;
		
						}
					}
					Write1Data(bMSB, MemRegisters[j].Address);
					Write1Data(bLSB, MemRegisters[j]. Address + 1);
					break;
				case MB_TYPE_ERR:
					break;
				default:
					break;
			}

		}

		i = 0;
		AddressPtr = oMasterFrame.Address;
		IndexData = 0;
		NotFound = FALSE;
		while (i < oMasterFrame.DataSize) {
			j = 0;
			found = FALSE;
			while (j < MemRegistersPtr) {
				if ((AddressPtr != MemRegisters[j].Address) || (found == TRUE)) {
					j++;
					continue;
				}
				
				found = TRUE;
				switch (this->MemRegisters[j].ValueType) {
					case MB_TYPE_INT:
					case MB_TYPE_BCD:
						Read1Data(&SB, AddressPtr);
						iSlaveFrame.Body[IndexData] = SB;
						AddressPtr++;
						IndexData++;
						break;
					case MB_TYPE_FLOAT:
						switch (FloatOrder){
							case MSBLSB:
								Read2Data(&bMSB, &bLSB, AddressPtr);
								break;
							case LSBMSB:
								Read2Data(&bLSB, &bMSB, AddressPtr);
								break;
							default:
								Read2Data(&bMSB, &bLSB, AddressPtr);
							break;
						}
						iSlaveFrame.Body[IndexData] = bMSB;
						IndexData++;
						iSlaveFrame.Body[IndexData] = bLSB;
						IndexData++;
						AddressPtr += 2;
						i++;
						break;
					case MB_TYPE_ERR:
						break;
					default:
						break;
				} 
				j++;
			}
			if (found == FALSE)
				NotFound = TRUE;
			i++;
		}

		// ////////////////////////////////////////////////////


		if (NotFound == TRUE) {
			iErrorFrame.DeviceId = oMasterFrame.DeviceId;
			iErrorFrame.FunctionId = oMasterFrame.FunctionId;
			iErrorFrame.Exception = MB_ILLEGAL_DATA_ADDRESS;
			usleep(SendDelay * 1000);
			if (m_debug)
				fprintf(stderr,"DEBUG[]: Ivalid data adress - element is not accessible\n");

			SendErrorPacket(fd, iErrorFrame);
/*
			sz_log(0,
			    "Error in MB_F_RHR function - Invalid Data Address");
*/
			return SLAVE_SEND_ERROR;
		}

		if (NotFound == FALSE) {
			iSlaveFrame.DeviceId = m_id;
			iSlaveFrame.FunctionId = MB_F_RHR;
			iSlaveFrame.Address = oMasterFrame.Address;
			iSlaveFrame.DataSize = oMasterFrame.DataSize;
			usleep(SendDelay * 1000);
			SendSlavePacket(fd, iSlaveFrame);
			if (m_debug)
				fprintf(stderr,"DEBUG[]: RHR Succesfully sent\n");

			return SLAVE_SEND_OK;
		}
		return SLAVE_SEND_OK;
	} 
	/* Read holding Register */
	if (oMasterFrame.FunctionId == MB_F_WMR) {	/* Read Holding Register */
		/*
		 * To ma byc przed engine do parsowania 
		 */
		if (m_debug)
			fprintf(stderr,"DEBUG[]: WMR recived\n");
		i = 0;
		AddressPtr = oMasterFrame.Address;
		IndexData = 0;
		NotFound = FALSE;
		/*
		 * Parsowanie pakietu do pamieci rejestrow 
		 */
		while (i < oMasterFrame.DataSize) {
			j = 0;
			found = FALSE;
			while (j < MemRegistersPtr) {
				if ((AddressPtr != MemRegisters[j].Address) || (found == TRUE)) {
					j++;
					continue;
				}

				found = TRUE;
				switch (MemRegisters[j].ValueType) {
					case MB_TYPE_INT:
					case MB_TYPE_BCD:
						SB = oMasterFrame.
							Body
							[IndexData];
						Write1Data(SB,
							   AddressPtr);
						AddressPtr++;
						IndexData++;
						// i++;
						break;
					case MB_TYPE_FLOAT:
						bMSB = oMasterFrame.
							Body
							[IndexData];
						IndexData++;
						bLSB = oMasterFrame.
							Body
							[IndexData];
						IndexData++;
						switch (FloatOrder) {
							case MSBLSB:
								Write2Data(bMSB, bLSB,
								    AddressPtr);
							break;
							case LSBMSB:
								Write2Data(bLSB, bMSB,
								   AddressPtr);
							break;
							default:
								Write2Data(bMSB, bLSB,
								   AddressPtr);
							break;
						}
						AddressPtr += 2;
						i++;
						break;
					case MB_TYPE_ERR:
						break;
					default:
							break;
				}
				j++;

			}

			if (found == FALSE)
				NotFound = TRUE;
			i++;
		}

		if (NotFound == TRUE) {
			iErrorFrame.DeviceId = oMasterFrame.DeviceId;
			iErrorFrame.FunctionId = oMasterFrame.FunctionId;
			iErrorFrame.Exception = MB_ILLEGAL_DATA_ADDRESS;
			usleep(SendDelay * 1000);
			if (m_debug)
				fprintf(stderr,"DEBUG[]: Ivalid data adress - element is not accessible\n");
	
			SendErrorPacket(fd, iErrorFrame);
			return SLAVE_RECEIVE_ERROR;
		}
	
		if (NotFound == FALSE) {
			iSlaveFrame.DeviceId = m_id;
			iSlaveFrame.FunctionId = MB_F_WMR;
			iSlaveFrame.Address = oMasterFrame.Address;
			iSlaveFrame.DataSize = oMasterFrame.DataSize;
			usleep(SendDelay * 1000);
			SendSlavePacket(fd, iSlaveFrame);
			if (m_debug)
				fprintf(stderr,"DEBUG[]: WMR Succesfully parsed\n");
		}
	
		/*
		 * Przepisywanie Grab z pamiêci do stuktury parcookowej
		 * (m_params)
		 */
		for (i = 0; i < m_params_count; i++) for (j = 0; j < MemRegistersPtr; j++) {
			if (m_params[i].addr != MemRegisters[j].Address) 
				continue;
			// MemRegisters[j].ValueType =
			// m_params[i].type; Nie ma potrzeby
			// tego po modyfikacji 1349
			switch (m_params[i].type) {
				case MB_TYPE_INT:
					Read1Data(&SB,
						  MemRegisters
						  [j].
						  Address);
					bin2int(SB, &fi);
					m_reads_buffer[i] = fi;
					break;
				case MB_TYPE_BCD:
					Read1Data(&SB,
						  MemRegisters
						  [j].
						  Address);
					int err;
					m_reads_buffer[i] = bcd2int(SB, &err);
					if (err) {
						sz_log(1, "Error parsing BCD value %x", SB);
						m_reads_buffer[i] = SZARP_NO_DATA;
					}
					break;
				case MB_TYPE_FLOAT:
					/* Tu juz nie trzeba swapa */
					Read2Data(&bMSB,
						  &bLSB,
						  MemRegisters
						  [j].
						  Address);
					bin2float(bMSB, bLSB,
						  &f);
					m_reads_buffer[i] =
						float2int(f,
							  m_params
							  [i].
							  prec);
					/* Experymentalne */
					break;
				case MB_TYPE_ERR:
					break;
				default:
					break;
			}
	
			/* Experymentalne */
			if (zerond == YES) {
				if (m_reads_buffer[i] <= 0) { 
					m_reads_buffer[i] = SZARP_NO_DATA ;
				}
			}
	
		}

		/* end of WRITE_MULTIPLE REGISTER */
		return SLAVE_RECEIVE_OK;

	}

	return SLAVE_ERROR;	/* Tu jest stan nieustalony */
}

void ModbusUnit::UpdateReqCycle() {
	int TotalCycle;
	if (WriteOneTime==ENABLE){
		if (WriteOneTimeLatch2 == YES)  {
			TotalCycle = m_params_count ; /* Total count of params */
		} else {
			TotalCycle = m_params_count + m_sends_count ; /* Total count of params */
		}
	} else {
			TotalCycle = m_params_count + m_sends_count ; /* Total count of params */
	}
	ReqCycle = (ReqCycle + 1) % TotalCycle ;

	if (ReqCycle >= m_params_count)	
		StatusCycle = SENDS_CYCLE ;
	else
		StatusCycle = PARAMS_CYCLE ;
	/* Hack dla CRP05 */
	if (WriteOneTime == ENABLE){
		if (StatusCycle == SENDS_CYCLE) {
			WriteOneTimeLatch = YES;
		}
		
		if ((StatusCycle == PARAMS_CYCLE) && (WriteOneTimeLatch == YES)) {	
			WriteOneTimeLatch2 = YES;
		}

		if (WriteOneTimeLatch2 == YES) {
			StatusCycle = PARAMS_CYCLE ;	
		}
	}
}

ModbusLine::ModbusLine() : m_mode(MB_MODE_ERR) {}

bool ModbusLine::ParseConfig(DaemonConfig *cfg, IPCHandler *ipc) {
	xmlDocPtr doc = cfg->GetXMLDoc();
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);
	xp_ctx->node = cfg->GetXMLDevice();

	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
		SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "modbus",
		BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);

	xmlChar *c = uxmlXPathGetProp(BAD_CAST strdup("./@modbus:mode"), xp_ctx);
	if (c == NULL) {
		sz_log(0, "Missing modbus mode attribute in configuration");
		return false;
	}
	if (!xmlStrcmp(c, BAD_CAST "master")) 
		m_mode = MB_MODE_MASTER;
	else if (!xmlStrcmp(c, BAD_CAST "slave"))
		m_mode = MB_MODE_SLAVE;
	else {
		sz_log(0, "Invalid modbus mode");
		return false;
	}
	xmlFree(c);

	c = uxmlXPathGetProp(BAD_CAST "./@modbus:parity", xp_ctx);
	if (!c)
		m_parity = NO_PARITY;
	else{
		if (!xmlStrcmp(c, BAD_CAST "NONE")){
			m_parity = NO_PARITY;
		}else if (!xmlStrcmp(c, BAD_CAST "ODD")){
			m_parity = ODD;
		
		}else if (!xmlStrcmp(c, BAD_CAST "EVEN")){ 
			m_parity = EVEN;
		}else{
			sz_log(0, "Unknown modbus:parity attribute %s",  (char *)c);
			return false;
		}
	}
	xmlFree(c);

	c = uxmlXPathGetProp(BAD_CAST "./@modbus:StopBits", xp_ctx);
	if (!c)
		m_stop_bits = 1;
	else{
		if (!xmlStrcmp(c, BAD_CAST "1")){
			m_stop_bits = 1;
		}else if (!xmlStrcmp(c, BAD_CAST "2")){
			m_stop_bits = 2;
		}else{
			sz_log(0, "Unknown modbus:StopBits attribute %s",  (char *)c);
			return false;
		}
	}
	xmlFree(c);

	c = uxmlXPathGetProp(BAD_CAST "./@modbus:OpenPortEveryTime", xp_ctx);
	if (!c || xmlStrcmp(c, BAD_CAST "yes"))
		m_always_init_port = false;
	else
		m_always_init_port = true;
	xmlFree(c);

	TUnit *unit = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit();
	size_t unit_num = 0;

	short* read = ipc->m_read;
	short* send = ipc->m_send;

	for (; unit; unit = unit->GetNext()) {

	        char *expr;
	        asprintf(&expr, ".//ipk:unit[position()=%zu]",
			unit_num + 1);
		xmlNodePtr node = uxmlXPathGetNode(BAD_CAST expr, xp_ctx);
		free(expr);

		assert(node);

		ModbusUnit *mbu = new ModbusUnit(unit);

		bool configured = mbu->Configure(read, 
					send, 
					node, 
					cfg->GetXMLDevice(), 
					cfg, 
					m_mode);

		if (configured == false)
			return false;

		m_units.push_back(mbu);
		read += unit->GetParamsCount();
		send += unit->GetSendParamsCount();
		
		unit_num++;

	}

	if (m_mode == MB_MODE_SLAVE && 
			unit_num > 1) {

		sz_log(0, "Configuration error - daemon is in slave mode and more than one unit is present in configuration, exiting");
		return false;
	}

	if (unit_num == 0) {
		sz_log(0, "No units defined");
		return false;
	}

	m_debug = cfg->GetSingle() || cfg->GetDiagno();

	m_fd = -1;
	m_cfg = cfg;
	m_ipc = ipc;

	return true;

}

void ModbusLine::PerformSlaveCycle() {

	ModbusUnit *u = m_units[0];

	int status = u->SlaveWait(m_fd);

	if (status > 0) {
		if (m_debug) 
			fprintf(stderr, "We have a packet\n");

		status = u->SlaveReply(m_fd);

		if (status != SLAVE_RECEIVE_OK) {
			if (m_debug)
				fprintf(stderr, "Error while receiving slave frame %d", status);
		}
	} else {
		if (m_debug) 
			fprintf(stderr, "Error while reading data from master %d\n", status);

	}

}

void ModbusLine::QuerySlaveUnit(ModbusUnit *unit) {
	unit->MasterAsk(m_fd);
	unit->MasterGet(m_fd);
}

void ModbusLine::PerformMasterCycle() {
	vector<ModbusUnit*>::iterator i;

	for (i = m_units.begin(); i != m_units.end(); ++i)
		QuerySlaveUnit(*i);
}

void ModbusLine::PerformCycle() {
	m_ipc->GoSender();

	switch (m_mode) {
		case MB_MODE_MASTER:
			PerformMasterCycle();
			break;
		case MB_MODE_SLAVE:
			PerformSlaveCycle();
			break;
		default:
			assert(false);
			break;
	}

	m_ipc->GoParcook();
}

void ModbusLine::Go() {

	m_fd = InitComm(SC::S2A(m_cfg->GetDevice()->GetPath()).c_str(),
			     m_cfg->GetDevice()->GetSpeed(), 8, m_stop_bits, m_parity);

	if (m_fd == -1) {
		sz_log(0, "Failed to open port, exiting");
		return;
	}

	do {
		time_t start = time(NULL);
		if (m_fd == -1) 
			m_fd = InitComm(SC::S2A(m_cfg->GetDevice()->GetPath()).c_str(),
				     m_cfg->GetDevice()->GetSpeed(), 8, m_stop_bits, m_parity);

		if (m_fd == -1) {
			sz_log(0, "Failed to init port, doing nothing");
			goto finish_cycle;
		}

		PerformCycle();

finish_cycle:
		if (m_always_init_port && (m_fd != -1)) {
			close(m_fd);
			m_fd = -1;
		}

		time_t now = time(NULL);
		int s = 10 - (now - start);
		if (s < 1) 
			sz_log(3, "Cycle lasts longer than 10sec (%ld)!!!", (now - start));

		if (s > 0)
			while ((s = sleep(s)) > 0);
	} while (true);

}

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	IPCHandler     *ipc;

	cfg = new DaemonConfig("mbrtudmn");

	if (cfg->Load(&argc, argv))
		return 1;

	ModbusLine modbus;

	ipc = new IPCHandler(cfg);
	if (ipc->Init()) {
		sz_log(0, "Initialization of IPC failed");
		return 1;
	}

	if (modbus.ParseConfig(cfg, ipc) == false)
		return 1;

	if ((cfg->GetSingle() == 1) || (cfg->GetDiagno() == 1))
		fprintf(stderr,"DEBUG[]: DAEMON STARTED\n");

	// wlaczenie trybu debug do biblioteki mbrtu
	mbrtu_single = FALSE;
	if ((cfg->GetSingle() == 1) || (cfg->GetDiagno()==1))
		mbrtu_single = TRUE;

	modbus.Go();

	return 1;
}
