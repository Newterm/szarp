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

#ifndef __POZYTON_PROP_PLUGIN_H__
#define __POZYTON_PROP_PLUGIN_H__

typedef struct {
	char ParamCode[100];	/* Kod identyfikacyjny symbolu */
	int Comma;		/* pozycja przecinka */
} tParams;

/**
 * PozytonData communication config.
 */
class PozytonDataInterface {
 public:
	virtual int GetParamsCount() = 0;

	PozytonDataInterface(int params, int _Single) {}

	/**
	 * Perform query.
	 * @param device path to serial device
	 * @param data buffer for output data
	 * @param single 1 for verbose debugging 
	 * @return 0 on success, 1 otherwise */
	virtual int Query(const char *device, short *data, int HowParams, tParams *ParamsTable, 
			int TRatio, unsigned char B4Mode, int single) = 0;

	virtual int ParseCommandLine(int argc, char *argv[], tParams * ParamsTable,
			     int *TRatio) = 0;
	virtual ~PozytonDataInterface() {}
};

typedef void log_function_t(int, const char *fmt, ...);
/** Initialize it in program !!! */
extern "C" log_function_t* pozyton_log_function;

typedef PozytonDataInterface* PozytonDataCreate_t(int, int);
typedef void PozytonDataDestroy_t(PozytonDataInterface *);

#endif
