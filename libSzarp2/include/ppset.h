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

#ifndef __PPSET__H_
#define __PPSET__H__

#include "config.h"

#include <libxml/tree.h>
#include <string>
#include <vector>

#ifdef MINGW32
#include <windows.h>
#endif

namespace PPSET {


enum PACKS {
	WEEK,
	DAY
};

struct regulator_id {
	std::string ne;
	std::string np;
	std::string nl;
	std::string nb;
};
class DaemonStopper { 
public:
	virtual void StartDaemon() = 0;
	virtual bool StopDaemon(xmlDocPtr *err) = 0;
	virtual ~DaemonStopper() {}
};

class DaemonStoppersFactory {
	public:
	virtual DaemonStopper* CreateDaemonStopper(const std::string &path) = 0;
};

xmlDocPtr handle_command(xmlDocPtr request, DaemonStoppersFactory &factory);

#ifndef MINGW32
typedef int PD;
#else
struct PD {
	HANDLE h;
	PD() : h(INVALID_HANDLE_VALUE) {}
};
#endif

class connection {
public:
	virtual bool open(xmlDocPtr * err_msg, DaemonStopper &stopper) = 0;
	virtual bool write(const char* buffer, int size) = 0;
	virtual bool read(char** buffer, int *size) = 0;
	virtual void close() = 0;
	virtual ~connection() {}
};

class serial_connection : public connection {
	std::string m_path;
	int m_speed;
	PD m_fd;
public:
	serial_connection(char *path, int speed);
	virtual bool open(xmlDocPtr * err_msg, DaemonStopper &stopper);
	virtual bool write(const char* buffer, int size);
	virtual bool read(char** buffer, int *size);
	virtual void close();
	virtual ~serial_connection() {}
};

class tcp_connection : public connection {
	std::string m_address;
	int m_port;
	int m_fd;
public:
	tcp_connection(char *address, int port);
	virtual bool open(xmlDocPtr * err_msg, DaemonStopper &stopper);
	virtual bool write(const char* buffer, int size);
	virtual bool read(char** buffer, int *size);
	virtual void close();
	virtual ~tcp_connection() {}
};


xmlDocPtr parse_settings_response(const char *response, int len);
xmlDocPtr create_error_message(const std::string & type, const std::string & reason);
bool wait_for_ok(connection& conn);
bool parse_regulator_id(const char *buffer, size_t size, regulator_id& rid);
void append_regualtor_id(regulator_id &rid, xmlDocPtr doc);
xmlDocPtr read_regulator_id(xmlDocPtr doc, connection& conn, char *id);
xmlDocPtr read_regulator_settings(connection& conn, char *id, bool* ok = NULL);
bool get_packs_type(xmlDocPtr request, PACKS * packs);
bool reset_packs(connection& conn, char *id, xmlDocPtr& error);
xmlDocPtr handle_set_packs_type_cmd(xmlDocPtr request, connection &conn, char *id, DaemonStopper &stopper);
bool reset_packs(connection& conn, char *id, xmlDocPtr& error);
xmlDocPtr handle_get_values_cmd(xmlDocPtr request, connection &conn, char *id, DaemonStopper &stopper);
bool send_set_command(connection& conn, char *id, std::string vals_type, std::string vals, xmlDocPtr * err);
bool send_set_command(connection& conn, char *id, std::string vals_type, std::string vals, xmlDocPtr * err);
bool get_param_data(xmlNodePtr node, std::string & number, std::string & value);
bool prepare_constants_string(xmlDocPtr request, std::string & result);
xmlDocPtr handle_set_constans_cmd(xmlDocPtr request, connection& conn, char *id, DaemonStopper &stopper);
xmlDocPtr handle_set_constans_cmd(xmlDocPtr request, connection& conn, char *id, DaemonStopper &stopper);
bool prepare_packs_string(xmlDocPtr request, std::vector<std::string> & results);
xmlDocPtr handle_set_packs_cmd(xmlDocPtr request, connection &conn, char *id, DaemonStopper &stopper);

}

#endif

