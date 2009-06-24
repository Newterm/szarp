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
 * (C) Pawel Palucha
 */

#ifndef __NPOZYTON_PROP_PLUGIN_H__
#define __NPOZYTON_PROP_PLUGIN_H__

/* Communication types */
#define C_OPTO 1		/* Optical head */
#define C_RS485 2		/* RS-485 */
#define C_CURRLOOP 3		/* electric current loop */
#define C_UNKNOWN -1		/* unknown */

/* type of meter */
#define T_LZQM	1		/* LZQM (default) */
#define T_EQABP	2		/* EQABP */


/**
 * PozytonData communication config.
 */
class NPozytonDataInterface {
 public:
	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	NPozytonDataInterface(int params, int _Single) { }
	NPozytonDataInterface() { }
	virtual ~ NPozytonDataInterface() { }

	virtual void Configure(short voltage_factor, short current_factor,
			       short delay, short type, char interface) = 0;
      	virtual void FilterCodes(char *Codes) = 0;
	virtual short LoadCodesToList(char *Codes) = 0;
	virtual short GoCodes() = 0;
	/** Print parameters to output */
	virtual void PrintInfo(FILE * stream) = 0;
	/** Perform query of device.
	 * @param device path to serial device
	 * @param databuff buffer for return data
	 * @return 0 on success, otherwise error
	 */
	virtual int Query(const char *device, short *databuff) = 0;
	virtual int GetDelay() = 0;
};

typedef void log_function_t (int, const char *fmt, ...);
/** Initialize it in program !!! */
extern "C" log_function_t* npozyton_log_function;

typedef NPozytonDataInterface* NPozytonDataCreate_t(int, int);
typedef void NPozytonDataDestroy_t(NPozytonDataInterface *);

#endif
