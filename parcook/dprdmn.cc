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
 * Demon dla urz±dzeñ DPR100C-100D. 
 * $Id$
 */
/*
 @description_start
 @class 4
 @devices Honeywell DPR100C/DPR100D universal recorder.
 @devices.pl Rejestrator Honeywell DPR100C/DPR100D.
 @config_example
 <device
      xmlns:dprdmn="http://www.praterm.com.pl/SZARP/ipk-extra"
      daemon="/opt/szarp/bin/dprdmn" 
      path="/dev/ttyA11"
      <unit id="1" dprdmn:unit_address="0x01">
              <param
                      name="..."
                      ...
                      dprdmn:address="0x00"
                      dprdmn:param_type="alarm">
                      ...
              </param>
              <param
                      name="..."
                      ...
                      prec="0x01"
                      dprdmn:address="0x02"
                      dprdmn:param_type="input">
              </param>
              ...
      </unit>
 </device>
 @description_end
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#include <vector>
#include <string>
#include <sstream>

#include "conversion.h"
#include "szarp_config.h"
#include "dmncfg.h"
#include "liblog.h"
#include "ipchandler.h"
#include "custom_assert.h"

#define MAX_UNIT_ADDRESS 0xFFU
#define MAX_PARAM_ADDRESS 0xFFU

using std::vector;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::istream;

/**types of data sent by a device*/
enum DataType {
	BIT,	
	FLOAT
};

/** Describe paramter */
struct ParamType {
	const char* name;	/**<parameter's name in szarp's params.xml config*/
	DataType data_type;	/**<type of parameter data*/
	unsigned data_size;	/**<size of a paramter (number of filelds occupied by paramter value in a packet)*/
	unsigned param_code;	/**<code of a paramter*/
};


/**list of supported paramters*/
const ParamType Parameters[] = { 
	{"input", FLOAT, 0x4, 0x18 },	/*regular input (analog, math..)*/
	{"alarm", BIT, 0x1, 0x01 },	/*alarm*/
	{"batch", FLOAT, 0x4, 0x02 },	/*batch ??*/
	{"relay", BIT, 0x2, 0x0c }	/*relay ??*/
};

/** Main daemon class */
class Daemon {

	/**Contains data on params*/
	struct ParamsData {
		unsigned unit_no;	/**<number of unit the data are pertaining to*/
		unsigned start_index;	/**<index of first param*/
		unsigned param_code;	/**<param code of a paramters data*/
		vector<unsigned> values;/**<values of params*/
	};
	/** List of parameters sent to parcook*/
	class UnitsList {

		class Unit {
			/**Holds info on various parameters of a param*/
			class ParamInfo {
				ParamInfo() {}; 
				unsigned prec; /**<precision of paramter*/
				unsigned addr; /**<parameter device's addres*/
				unsigned param_code; /**<paramter code*/
				DataType data_type; /**<type of a data stored by a paramter*/
				public:
				/**@see prec getter
				 * @return param's precision
				 */
				unsigned GetPrec() {
					return prec;
				}
				/**@see address getter
				 * @return pram's device address*/
				unsigned GetAddr() {
					return addr;
				}
				/**@see param_code getter
				 * @return param_code*/
				unsigned GetParamCode() {
					return param_code;
				}
				/**@see data_type getter
				 * @return data_type */
				unsigned GetDataType() {
					return data_type;
				}
				/*Creates ParamInfo object
				 * @param node node to a param element in params.xml
				 * @param tp pointer to TParam object
				 * @return ParamInfo object*/
				static ParamInfo* Create(xmlNodePtr node,TParam *tp) {
					ParamInfo* pi = new ParamInfo();
					char *str = NULL;
					try {
						str = (char*) xmlGetNsProp(node, BAD_CAST("address"),
							BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
						if (str == NULL)
							throw ParseEx("Attribute dprdmn:address not present in a param element",xmlGetLineNo(node));
						if (sscanf(str, "%x", &pi->addr) != 1 ||
								pi->addr >= MAX_PARAM_ADDRESS )
							throw ParseEx("Invalid address atribute in a param element",xmlGetLineNo(node));
						xmlFree(str);
						str = (char*) xmlGetNsProp(node, BAD_CAST("param_type"),
							BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
						if (str == NULL)
							throw ParseEx("Attribute dprdmn:param_type not present in a param element",xmlGetLineNo(node));
						bool found = false;
						for (unsigned i = 0; i < sizeof(Parameters)/sizeof(ParamType) ; ++i)
							if (!strcmp(str, Parameters[i].name)) {
								pi->data_type = Parameters[i].data_type;
								pi->param_code = Parameters[i].param_code;
								found = true;
								break;
							}
						if (!found)
							throw ParseEx("Unknown param_type",xmlGetLineNo(node));

						xmlFree(str);

						pi->prec = tp->GetPrec();
					}
					catch (ParseEx& e)
					{
						if (str)
							xmlFree(str);
						delete pi;
						throw;
					}
					return pi;
				}

			};
			typedef vector<ParamInfo*> VP;
			unsigned unit_no;		/**<unit numer*/
			short *param_buf;		/**<pointer to param values buffer*/
			VP params;			/**<holds "naitive" numer of each param*/
			Unit() {};			/**<default constructor*/

			/*Finds index of a param with given address and parameter code
			 * @param address param address
			 * @param param_code code of a parameter
			 * @param index output parameter, index of a param
			 * @return true if param is found false otherwise */
			bool FindParam(unsigned address, unsigned param_code,VP::size_type& index) {
				for (VP::size_type i = 0; i < params.size(); ++i) 
					if (params[i]->GetAddr() == address &&
							params[i]->GetParamCode() == param_code) {
						index = i;
						return true;
					}
				return false;
			}
			public:
			/*Creates unit object from given xml node and TUnit object
			 * @param node node to unit element in params.xml
			 * @tu pointer to a TUnit object
			 * @return ptr to unit object*/
			static Unit* Create(xmlNodePtr node,TUnit* tu) {
				ASSERT(node != NULL);
				ASSERT(tu != NULL);
				char *str = NULL;
				Unit* unit = new Unit();
				unit->param_buf = NULL;
				try {
					str = (char *) xmlGetNsProp(node,
						BAD_CAST("address"),
						BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
					if (str == NULL) 
						throw ParseEx("Attribute dprdmn:address not present in an unit element",xmlGetLineNo(node));
					if (sscanf(str, "%x", &unit->unit_no) != 1 
							|| unit->unit_no >= MAX_UNIT_ADDRESS )
						throw ParseEx(string("Invalid unit addres:") + str,xmlGetLineNo(node));
					xmlFree(str);
					/*parse params for a unit*/
					TParam *tp = tu->GetFirstParam();
					for (node = node->children; node && tp; node = node->next) {
						if (strcmp((char *) node->name, "param"))
								continue;
						unit->params.push_back(ParamInfo::Create(node,tp));
						tp = tu->GetNextParam(tp);
					}

				}
				catch (ParseEx& e) {
					if (str)
						xmlFree(str);
					delete unit;
					throw;
				}

				return unit;
				
			}
				
			/**Sets buffer used for storing paramters' values
			 * @param buffer */
			void SetBuffer(short *buffer) {
				param_buf = buffer;
			}
			/** Returns a size of a buffer which is required by a unit
			 * for storing paramters' values
			 * @return buffer size*/
			unsigned GetReqBufferSize() {
				return params.size();
			}

			/** Returns unit number 
			 * @return unit number */
			unsigned GetUnitNo() {
				return unit_no;
			}
			
			/** Converts an unsigned number which value is raw representation
			 * of float to a signed short using provided precison
			 * @param val - float value 
			 * @param prec - precision 
			 * @return val*10^prec*/
			short fbin2short(unsigned val, int prec) {
				float fval;
				memcpy(&fval, &val, sizeof(unsigned));

				if (!finitef(fval))
					return SZARP_NO_DATA;

				if (fval > (float) SHRT_MAX)
					fval = (float) SHRT_MAX;
				if (fval < (float) SHRT_MIN)
					fval = (float) SHRT_MIN;

				switch (prec) {
					case 0: return (short) ceilf(fval);
					case 1: return (short) ceilf(fval * 10);
					case 2: return (short) ceilf(fval * 100);
					case 3: return (short) ceilf(fval * 1000);
					case 4: return (short) ceilf(fval * 10000);
					default: ASSERT(false);
				}

				return SZARP_NO_DATA;
			}

			/*Set data of a parameters*/
			void SetValues(const ParamsData& d) {
				VP::size_type index; 
				int start_addr = d.start_index;
				vector<unsigned>::const_iterator i; 
				for (i = d.values.begin(); i != d.values.end(); ++i,++start_addr) 
					if (FindParam(start_addr, d.param_code, index)) {
						ParamInfo *pi = params[index];
						switch (pi->GetDataType()) {
							case BIT: 
								param_buf[index] = (short)*i;
								break;
							case FLOAT:
								param_buf[index] = fbin2short(*i,pi->GetPrec());
								break;
						}

					}
			}
			~Unit() {
				for (VP::size_type i = 0; i < params.size(); ++i)
					delete params[i];
			}

		};

		UnitsList() : param_buf(NULL), buf_size(0), ipc(NULL) {};
						/**<a default constructor*/
		short *param_buf;		/**<local(daemon's) buffer of params values*/
		unsigned buf_size;		/**<size of a buffer*/
		vector<Unit*> units;		/**<devices list*/
		IPCHandler* ipc;		/**<pointer to a IPCHandler*/

	public:
		/** Sets values of params
		 * @param d @see ParamsData structure describing params*/
		void SetValues(const ParamsData& d) {
			Unit* dev = NULL;
			for (vector<Unit*>::size_type i = 0; i < units.size();++i) {
				if (units[i]->GetUnitNo() == d.unit_no) {
					dev = units[i];
					break;
				}
			}

			if (dev == NULL) {
				//ostringstream ss;
				//ss << "No device of number:" << d.unit_no << " in configuration";
				//throw Exception(ss.str());
				sz_log(4,"No device of number %d in the configuration", d.unit_no);
				return;
			}

			dev->SetValues(d);
			
		}
		/**Sets SZARP_NO_DATA value for each param*/
		void SetNoData() {
			for (unsigned i = 0; i < buf_size;++i) 
				param_buf[i] = SZARP_NO_DATA;
		}

		/*Create UnitsList object 
		 * @param cfg pointer to DaemonConfig object
		 * @return UnitsList object*/
		static UnitsList* Create(DaemonConfig *cfg) {
			ASSERT(cfg != NULL);

			TDevice* td = cfg->GetDevice();
			ASSERT(td != NULL);

			TRadio *tr = td->GetFirstRadio();
			ASSERT(tr != NULL);

			xmlNodePtr device_node = cfg->GetXMLDevice();
			ASSERT(device_node != NULL);

			UnitsList *ul = new UnitsList();

			try {
				TUnit *tu = tr->GetFirstUnit();
				for (xmlNodePtr node = device_node->children; node && tu ; node = node->next) 
					if (!strcmp( (char*) node->name, "unit")) {
						ASSERT(tu != NULL);
						Unit* u = Unit::Create(node, tu);
						ul->buf_size += u->GetReqBufferSize();
						ul->units.push_back(u);
						tu = tr->GetNextUnit(tu);
					}

				ul->ipc = new IPCHandler(cfg);
				if (ul->ipc->Init())
					throw Exception("Error connecting to parcook");
				ul->param_buf = ul->ipc->m_read;
				
				/*Assign part of a buffer for each unit*/
				unsigned free_space_start = 0;
				for (vector<Unit*>::iterator i = ul->units.begin();i != ul->units.end();++i) {
					Unit* u	= *i;
					u->SetBuffer(&ul->param_buf[free_space_start]);
					free_space_start += u->GetReqBufferSize();
				}

			}
			catch (Exception) 
			{
				delete ul;
				throw;
			}

			return ul;
		}

		/**Sends data to a parcook*/
		void SendDataToParcook() {
			ipc->GoParcook();
		}

		~UnitsList() {
			for (vector<Unit*>::iterator i = units.begin();i != units.end();++i) 
				delete *i;
			free(param_buf);
			delete ipc;
		}
	};

	/**Inerprets and extracts data from raw packets*/
	class PacketHandler {
		/**Contains data desribing request packet*/
		struct ReqPacket {
			unsigned unit_no;	/**<number of unit*/
			unsigned number_of_values; /**<number of values in a packet*/
			unsigned data_size;	/**<size of data single value*/
			unsigned param_code;	/**<parameter code a packet*/
			DataType data_type;	/**<data type of a packet*/
			unsigned start_index;	/**<staring index of a packet*/
			bool has_crc;		/**<indicates whether packet contains crc*/
		};

		/**packet type*/
		typedef enum {
			REQUEST_PACKET, 
			RESPONSE_PACKET,
			INVALID_PACKET
		} PacketType;

		static const unsigned NO_CRC_PROTOCOL; 	
		static const unsigned CRC_PROTOCOL;
		static const unsigned READ_CODE;
		static const string PACKET_TERMINATOR;
		static const unsigned HEX_DATA;
		static const unsigned RESP_OK_STATUS;
		static const char FIELD_SEPARATOR;

		bool has_req;			/**<indicates if PacketHandler parsed reqest packet*/
		bool new_data_available; 	/**<true if request, respone pair was successfully processed*/
		ParamsData params_data;	 	/**<data from most recent request, response packet pair*/ 
		ReqPacket req_packet;		/**<request packet*/
		string buffer;			/**<a buffer used in string operaions*/

		/**Converts string representation of a number into unsigned integer
		 * @param s - string representing a number
		 * @param res - output paramter - converted value
		 * @return true is string was successfully converted false otherwise*/
		bool StrToU(const string& s,unsigned &res) {
			res = 0;
			for (string::const_iterator i = s.begin(); i != s.end(); ++i) 
				if (*i >= '0' && *i <= '9')
					res = res*16 + (unsigned) (*i-'0');
				else if (*i >= 'A' && *i <= 'F')
					res = res*16 + (unsigned) (*i - 'A' + 10);
				else 
					return false;
			return true;
		}

		/**Verifies CRC of a packet
		 * @param str a packet string
		 * @param end position in a @see str where packet data ends
		 * @retrun true is computed crc value is equal to that provided in crc param*/
		bool CheckCRC(const string& str,unsigned end) {
			if ((end + 2) >= str.size()) 
				return false; 

			unsigned crc;
			buffer = str.substr(end, 2);
			if (!StrToU(buffer, crc))
				return false;

			unsigned sum = 0;
			for (unsigned start = 0; start < end; ++start)
				sum += (unsigned)str[start];
			return crc == (sum & 0xffu);
		}

		/**Classifies packet
		 * @param str string representing a packet
		 * @return type of a packet*/
		PacketType ClassifyPacket(const string &str) {
			string::size_type pos;
			pos = str.find(FIELD_SEPARATOR);
			switch (pos) {
				case 2: /*possibly a request packet*/
					return REQUEST_PACKET;
				case 6: /*possibly a resposne packet*/
					return RESPONSE_PACKET;
				default:
					return INVALID_PACKET;
			}

		}

		/**Compute value of next filed in a packet
		 * (fields are sequences of characters separated by @see FIELD_SEPARATOR)
		 * @param is stream containg a packet
		 * @param length expected length of a field
		 * @param val output paramter, the value of a field
		 * @return true if next field was converted and was of length spefieid in length
		 * paramter*/
		bool GetFieldValue(istream& is, unsigned length, unsigned& val) {
			if (!is)
				return false;
			getline(is, buffer, FIELD_SEPARATOR);
			if (buffer.size() != length)
				return false;
			if (StrToU(buffer, val))
				return true;
			else
				return false;
		}
		/**Processes a request packet
		 * @param raw_packet a packet*/
		void HandleRequestPacket(const string &raw_packet) {
			has_req = false;
			unsigned val;
			istringstream ss(raw_packet);
			bool param_code_found = false;
			/*unit address*/
			if (!GetFieldValue(ss, 2, req_packet.unit_no)) {
				sz_log(4, "Invalid address");
				goto bad_packet;
			}
			
			/*protocol type*/
			if (!GetFieldValue(ss, 4, val)) {
				sz_log(4, "Cannot get protocol type");
				goto bad_packet;
			}

			if (val == NO_CRC_PROTOCOL)
				req_packet.has_crc = false;
			else if (val == CRC_PROTOCOL)
				req_packet.has_crc = true;
			else {
				sz_log(4,"Unknown protocol");
				goto bad_packet;
			}

			/*parameter type and function code*/
			if (!GetFieldValue(ss, 4, val)) {
				sz_log(4,"Cannot get fucntion and param codes");
				goto bad_packet;
			}
			/*check if read operation*/
			if ((val >> 8) != READ_CODE) {
				sz_log(4, "Not read packet");
				goto bad_packet;
			}
			req_packet.param_code = val & 0xffu;

			for (unsigned i = 0; i < sizeof(Parameters)/sizeof(ParamType); ++i) 
				if (Parameters[i].param_code == req_packet.param_code) {
					req_packet.data_size = Parameters[i].data_size;
					req_packet.data_type = Parameters[i].data_type;
					param_code_found = true;
					break;
				}
			if (!param_code_found) {
				sz_log(4,"Unsupported param code");
				goto bad_packet;
			}

			/*data format type*/
			if (!GetFieldValue(ss, 1, val)) {
				sz_log(4,"Cannot get data format");
				goto bad_packet;
			}
			if (val != HEX_DATA) { 
				sz_log(4,"Invalid data format");
				goto bad_packet;
			}

			/*number of values*/
			if (!GetFieldValue(ss, 2, req_packet.number_of_values)) {
				sz_log(4,"Connot get number of values");
				goto bad_packet; 
			}

			/*start index*/
			if (!GetFieldValue(ss, 2, req_packet.start_index)) {
				sz_log(4,"Connot get data starting index");
				goto bad_packet;
			}

			/*crc*/
			if (req_packet.has_crc) {

				unsigned packet_data_end = ss.tellg();
				if (!CheckCRC(raw_packet, packet_data_end)) {
					sz_log(4,"Wrong CRC");
					goto bad_packet;
				}
			}

			has_req = true;
			return;

		bad_packet:
			sz_log(4,"Unknown/illegal request packet %s",raw_packet.c_str());	
			return;

		}	

		/**Processes a response packet
		 * @param raw_packet a packet*/
		void HandleResponsePacket(const string& raw_packet) {
			istringstream ss(raw_packet);
			unsigned val;
			if (!has_req) 	/*daemon is not in sync with LPCS application*/
				return;
			if (!GetFieldValue(ss, 6, val)) {
				sz_log(4,"Cannot get status");
				goto bad_packet;
			}

			has_req = false;

			if (val != RESP_OK_STATUS) {
				sz_log(4,"Bad resp status");
				goto bad_packet;
			}

			/*read values*/
			new_data_available = false;

			switch (req_packet.data_type) {
				case BIT:
					if (!ReadBoolValues(ss))
						goto bad_packet;
					break;
				case FLOAT:
					if (!ReadFloatValues(ss))
						goto bad_packet;
					break;
			}

			if (req_packet.has_crc) { /*if request has a crc response
						  has it also*/
				unsigned packet_data_end = ss.tellg();
				if (!CheckCRC(raw_packet, packet_data_end)) {
					sz_log(4,"Wrong crc in resp packet");
					goto bad_packet;
				}

			}

			params_data.unit_no = req_packet.unit_no;
			params_data.start_index = req_packet.start_index;
			params_data.param_code = req_packet.param_code;
			new_data_available = true;
			return;
		bad_packet:
			sz_log(9, "Illegal/unknown packet:%s", raw_packet.c_str());
			new_data_available = false;

		}

		/**Retrieves boolean values from a packet
		 * @param ss a stream containing a packet
		 * @return true if operation was successful false otherwise*/
		bool ReadBoolValues(istream& ss) {
			params_data.values.clear();
			for (unsigned i = 0; i < req_packet.number_of_values; ++i) {
				unsigned value = 0;
				for (unsigned j = 0; j < req_packet.data_size; ++j) {
					unsigned val;
					if (!GetFieldValue(ss, 2,val))
						return false;	
					value = value * 16 + val;
				}
				for (unsigned j = 0; j < req_packet.data_size * 8 ; ++j) {
					params_data.values.push_back(value % 1);
					value >>= 1;
				}
			}
			return true;
		}

		/**Retrieves float values from a packet
		 * @param ss a stream containing a packet
		 * @return true if operation was successful false otherwise*/
		bool ReadFloatValues(istream& ss) {
			params_data.values.clear();
			for (unsigned i = 0; i < req_packet.number_of_values; ++i) {
				unsigned value = 0;
				for (unsigned j = 0; j < 4; ++j) {
					unsigned val;
					if (!GetFieldValue(ss, 2, val))
						return false;	
					value = value * 256 + val;
				}
				params_data.values.push_back(value);

			}
			return true;
		}
		public:
		PacketHandler() : has_req(false), new_data_available(false) {};

		/**Indicates whether new params' data are available*/
		bool HasNewData() {
			return new_data_available;
		}

		/**Retrieves params data
		 @return a reference to ParamsData object*/	
		const ParamsData& GetParamsData() {
			new_data_available = false;
			return params_data;
		}

		/**Extracts data from a packet
		 * @param - string representing a packet*/
		void HandlePacket(const string& packet) {

			switch(ClassifyPacket(packet)) {
				case REQUEST_PACKET:
					HandleRequestPacket(packet);
					break;
				case RESPONSE_PACKET:
					HandleResponsePacket(packet);
					break;
				default:
					sz_log(9,"Unknown packet %s",packet.c_str());
			}
				
		}
			
	};

	/**Provides access to a serial port device*/
	class SerialPort {
#define SP_BUF_SIZE 255
		bool is_open;	/**<true if port is opened*/
		int fd;		/**<descriptor of an opened port*/
		char buf[255];	/**<data buffer*/
		int buf_pos;	/**<current write position in buf*/
		long timeout;
		public:
		SerialPort() : is_open(false),buf_pos(0) {};

		/**Opens serial port, sets communiation parameters
		 * @param device a serial port device
		 * @param speed serial port speed
		 * @param c_stopbits number of stop bits
		 * @return true if open was successful*/
		int Open(const char* device, int speed, int c_stopbits) { 
			if (is_open)
				close(fd);

			if ((fd = open(device, O_RDWR | O_NOCTTY, 0)) == -1) 
				return false;
	
			struct termios ti;
			if (tcgetattr(fd, &ti) < 0) {
				close(fd);
				return false;
			}
	
			ti.c_cflag = 0;
			switch (speed) {
				case 300:
					ti.c_cflag = B300;
					break;
				case 600:
					ti.c_cflag = B600;
					break;
				case 1200:
					ti.c_cflag = B1200;
					break;
				case 2400:
					ti.c_cflag = B2400;
					break;
				case 4800:
					ti.c_cflag = B4800;
					break;
				case 9600:
					ti.c_cflag = B9600;
					break;
				case 19200:
					ti.c_cflag = B19200;
					break;
				case 38400:
					ti.c_cflag = B38400;
					break;
				default:
					ti.c_cflag = B9600;
			}
	
			if (c_stopbits == 2)
				ti.c_cflag |= CSTOPB;
				
			ti.c_oflag = 
				ti.c_iflag =
				ti.c_lflag = 0;
			ti.c_cflag |= CS7 | CREAD | CLOCAL | PARENB;

			if (tcsetattr(fd, TCSANOW, &ti) < 0) {
				close(fd);
				return false;
			}

			return true;
		}
		/*Sets @see Readline operation timeout
		 * @param _timeout read timeout in miliseconds*/
		void SetTimeout(long _timeout) {
			timeout = _timeout;
		}

		/**Reads a line (any sequence of charaters terminated with \n) from a port
		 * @return string containing read line or NULL in case of timeout*/
		const char* Readline() {
			if (is_open)
				return NULL;
			struct timeval tv;
			fd_set rdset;
			int sel;

			FD_ZERO(&rdset);
			FD_SET(fd,&rdset);
			
			tv.tv_sec = 0;
			tv.tv_usec = timeout;

			sel = select(fd + 1, &rdset, NULL, NULL, &tv);

			if (sel < 0) 
				throw Exception(string("Select error") + strerror(errno));
			if (sel > 0)
				while (read(fd, &buf[buf_pos], 1) > 0) {
					if (buf_pos + 2 >= SP_BUF_SIZE) {
						buf_pos = 0;
						throw Exception("Buffer overflow");
					}
					if (buf[buf_pos] == '\n') {
						buf[buf_pos + 1] = 0;
						buf_pos = 0;
						//log(1,"Got something which resembles packet");
						//log(1,"Buf:%s",buf);
						return buf;
					}
					buf_pos++;
				}

			return NULL;
		}
#undef SP_BUF_SIZE
	};

	/**Exception class*/
	struct Exception {
		string what; /**<cause of an exception*/
		Exception(const string& _what = "") : what(_what) {};
	};

	/**Exception generated by error while extracting data from params.xml*/
	struct ParseEx : public Exception {
		int line_no;	/**<line in which error ocurred*/
		ParseEx(const string& _what = "",int _line_no = -1) : Exception(_what) , line_no(_line_no) {};
	};
	UnitsList* ul; 	/**<@see UnitsList*/
	SerialPort* sp; /**<@see SerialPort*/
	PacketHandler* ph;	/**<@see PacketHandler*/
	DaemonConfig* dc; 	/**daemon configuration object*/

	public:
	/**<Initializes daemon
	 * @param cfg pointer to DaemonsConfig configuration*/
	Daemon(DaemonConfig *cfg) : ul(NULL), sp(NULL), ph(NULL), dc(cfg) {
		ASSERT(cfg != NULL);
		try {
			ul = UnitsList::Create(dc);
			sp = new SerialPort();
			ph = new PacketHandler();
		}
		catch (ParseEx& pe) {
			sz_log(0,"Error while parsing params.xml: %s, line:%d.", pe.what.c_str(), pe.line_no);
			exit(1);
		}
		catch (Exception &e) {
			sz_log(0,"Initialization error:%s.", e.what.c_str());
			exit(1);
		}

		TDevice* dev = dc->GetDevice();
		if (!sp->Open(SC::S2A(dev->GetPath()).c_str(),
				dev->GetSpeed(),
				dev->GetStopBits())) {
			sz_log(0,"Unable to open serial port %ls.",dev->GetPath().c_str());
			exit(1);
		}

		sp->SetTimeout(1000000); /*1 sec*/

		ul->SetNoData();

	}
	/**Daemon's main loop*/ 
	void Go() {
		sz_log(9,"Start");
		int err_count = 0;
		time_t last_sent_time = time(NULL);
		while (true) {
			time_t cur_time = time(NULL);
			if (cur_time - last_sent_time >= 10) {
				ul->SendDataToParcook();
				last_sent_time = cur_time;
				sz_log(9,"Sending data to parcook");
				ul->SetNoData();
			}

			const char *packet = NULL;

			try {
				packet = sp->Readline();
			}
			catch (Exception e) {
				sz_log(1,"Error while reading data from port:%s", e.what.c_str());
				if (err_count++ > 100) {
					sz_log(0, "Too many port reading erros, exiting");
					exit(1);
				}
			}

			if (!packet)
				continue;

			ph->HandlePacket(packet);

			if (ph->HasNewData())
				ul->SetValues(ph->GetParamsData());
		}
	}

	~Daemon() {
		delete ul;
		delete sp;
		delete ph;
	}


};

const unsigned Daemon::PacketHandler::NO_CRC_PROTOCOL = 0x0204;
const unsigned Daemon::PacketHandler::CRC_PROTOCOL = 0x4204;
const unsigned Daemon::PacketHandler::READ_CODE = 0x01;
const string Daemon::PacketHandler::PACKET_TERMINATOR = "\r\n";
const unsigned Daemon::PacketHandler::HEX_DATA = 0x0;
const unsigned Daemon::PacketHandler::RESP_OK_STATUS = 0x1;
const char Daemon::PacketHandler::FIELD_SEPARATOR = ',';

int main(int argc,char *argv[]) {
	DaemonConfig *cfg;
	Daemon *daemon;

	cfg = new DaemonConfig("dprdmn");
	if (cfg->Load(&argc, argv)) {
		sz_log(0, "Error loading config, exiting");
		return 1;
	}

	daemon = new Daemon(cfg);
	daemon->Go();
	return 1;

}

