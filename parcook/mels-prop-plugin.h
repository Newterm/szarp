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

/** Daemon for Mitsubishi regulators */

class SerialException : public std::runtime_error {
	public:		
	SerialException() : std::runtime_error("Error details not speficied") {};
	SerialException(int err) : std::runtime_error(strerror(err)) {};
	SerialException(const std::string &err) : std::runtime_error(err) {};
};

struct MelsParam {
	enum TYPE {
		FLOAT, 
		SHORT_FLOAT,
		INTEGER,
		NONE
	} type;
	int prec;
	int address;
	MelsParam() {
		type = NONE;
		prec = 0;
		address = -1;
	}
};

class MELSDaemonInterface { 
public:
	enum PARAMETER_TYPE {
		PARAM_GENERAL = 0,
		PARAM_EXTENDED,
		SEND_GENERAL,
		SEND_EXTENDED,
		LAST_PARAMETER_TYPE };
	virtual void OpenPort() = 0;
	virtual int Connect() = 0;
	virtual void CallAll() = 0;
	virtual void ConfigureSerial(const char *path, int speed) = 0;
	std::map<unsigned, MelsParam> m_params[LAST_PARAMETER_TYPE];
	virtual void SetRead(short *data) = 0;
	virtual void SetSend(short *data) = 0;
	MELSDaemonInterface() { }
	virtual ~MELSDaemonInterface() { }
	virtual void ConfigureParam(
			int index, 
			int address, 
			MELSDaemonInterface::PARAMETER_TYPE type,
			int prec, 
			MelsParam::TYPE val_type) = 0;
};

typedef void log_function_t(int, const char *fmt, ...);
/** Initialize it in program !!! */
extern "C" log_function_t* mels_log_function;

typedef MELSDaemonInterface* MELSDaemonCreate_t();
typedef void MELSDaemonDestroy_t(MELSDaemonInterface*);
