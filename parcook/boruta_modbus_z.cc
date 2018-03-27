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
 * 
 * Darek Marcinkiewicz <reksio@newterm.pl>
 * 
 */
/*
 Daemon description block.

 @description_start

 @class 4

 @devices This is a borutadmn subdriver for Modbus protocol, widely used by various PLC and metering
 devices or SCADA systems.
 @devices.pl Sterownik do demona borutadmn, obs³uguj±cy protokó³ Modbus, powszechnie u¿ywany przez ró¿nego
 rodzaju urz±dzenia pomiarowe, sterowniki PLC czy systemy SCADA.

 @protocol Modbus RTU or ASCII protocol over serial line (master and slave mode), Modbus TCP over network 
 (client and server mode).
 @protocol.pl Modbus RTU lub ASCII na ³±czu szeregowym (tryb master i slave), Modbus TCP przez po³±czenie
 sieciowe (tryb client i server)

 @config Driver is configured as a unit subelement of device element in params.xml. See example for allowed
 attributes.
 @config.pl Sterownik jest konfigurowany w pliku params.xml, w podelemencie unit elementu device. Opis
 dodatkowych atrybutów XML znajduje siê w przyk³adzie poni¿ej.

 @config_example
<device 
	xmlns:extra="http://www.praterm.com.pl/SZARP/ipk-extra"
	daemon="/opt/szarp/bin/borutadmn" 
	path="/dev/null"
		ignored, should be /dev/null
	speed="9600"
		ignored
	>

	<unit 
		id="1"
			Modbus unit id own (for server/slave mode) or other side; if given as single digit or letter
			it's interpreted as ASCII ('A' is 65, '1' is 49), otherwise it's interpreted as number ('01 
			is 1); you should rather use extra:id attribute which is always interpreted as number
		extra:id="1"
			Modbus unit id, overwrites 'id' attribute
		type="1"
			ignored, should be 1
	       	subtype="1" 
			ignored, should be 1
		bufsize="1"
			average buffer size, at least 1
		extra:proto="modbus" 
			protocol name, denoting boruta driver to use, should be "modbus" for this driver
		extra:mode="client" 
			unit working mode, "client" or "server"; client always denote active side (master
			for Modbus RTU, client for Modbus TCP); server always denote passive side (slave
			for Modbus RTU, server for Modbus TCP)
		extra:medium="serial"
			data transmission medium, may be "serial" (for Modbus RTU) or "tcp" (for Modbus TCP)
                extra:tcp-address="172.18.2.2"
			IP address to connect to, required for mode "client" and medium "tcp"
                extra:tcp-port="23"
                        IP port to connect to (mode "client") or listen on (mode "server"), required for 
			medium "tcp"
                extra:path="/dev/ttyS0"
                        path to serial port, required if medium is set to "serial"
                extra:speed="19200"
                        optional serial port speed in bps (for medium "serial"), default is 9600, allowed values
                        are also 300, 600, 1200, 2400, 4800, 19200, 38400; all other values result in
                        port speed 9600
                extra:parity="even"
                        optional serial port parity (for medium "serial"), default is "none", other
                        allowed values are "odd" and "even"
                extra:stopbits="1"
                        optional serial port stop bits number (for medium "serial"), default is 1, other
                        allowed value is 2
		extra:FloatOrder="msblsb"
			order of words for 2 registers encoded values, default is "msblsb" (first most, then less
			significant word), other possible value is "lsbmsb"; can be overwritten for specific
			parameter
		extra:DoubleOrder="msdlsd"
			order of words for 4 registers encoded values, default is "msdlsd" (first most, then less
			significant double), other possible value is "lsdmsd"; can be overwritten for specific
			parameter
		extra:nodata-timeout="60"
			time (in seconds) of value  expiration if no data was received, default value is
		        "0" (never expire)
		extra:nodata-value="-1"
			value send when no parameter value is not available, default is 0	
		extra:read-timeout="35"
			time (in miliseconds) gap between last and first byte of two frames in rtu data transfer
			protocol. Very important is to set this to proper value, cause daemon not always calculates
			this correctly. According to modbus rtu documentation good value is 3.5 times time between 
			two bytes in frame. Based on our experience 35ms is usually good value.
		extra:single-register-pdu="1"
			if set to 1, every modbus register will be queried separately by modbus client unit,
			may not work with float registers. Set to 0 or ommit to disable.
		extra:query-interval="0"
			interval between queries, in ms, defaults to 0.
        >
	<param
		param elements denote values that can be send or read from device to SZARP
		name="Name:Of:Parameter"
		...
		extra:address="0x12"
			required, decimal or hexadecimal raw modbus address of first register connected to parameter,
			from 0 to 65535
		extra:register_type="holding_register"
			Modbus register type, one of "holding_register" (default) or "input_register", decides
			what Modbus function can be used to read register from device (0x03 - ReadHoldingRegister or
			0x04 - ReadInputRegister); for server mode it's not significant - both 0x06 (WriteSingleRegister)
			and 0x10 (WriteMultipleRegisters) functions are accepted
		extra:val_type="integer"
			required, register value type, one of "integer", "bcd" (not supported for 'send' elements), 
			"long" or "float" or "double"; integer and bcd coded parameters occupy 1 register, long integer and float
			registers occupy 2 successive registers, double val_type occupy 4 successive registers
		extra:FloatOrder="lsbmsb"
			overwritten unit FloatOrder value for specific parameter
		extra:DoubleOrder="lsdmsd"
			overwritten unit FloatOrder value for specific parameter
		extra:val_op="MSW"
			optional operator for converting long or float modbus values to parameter values; SZARP
			holds parameters as 2 bytes integers with fixed precision; if val_op is not given,
			4 bytes float or long value is simply converted to short integer (reflecting precision);
			you can also divide one modbus values into 2 szarp parameters (called 'combined parameters')
			- you need to configure these 2 parameters with the same modbus addres and type, but one
			with val_op="MSW" (it will hold most significant word of value), second with val_op="LSW"
		...
	/>
	...
	<send
		denotes value send/read from SZARP to device
		param="Name:Of:Parameter"
			name of parameter to send
		extra:address="0x12"
			same as for param element above
		extra:register_type="holding_register"
			same as for param element above, significant only for server mode (for client mode
			function 0x10 (WriteMultipleRegisters) is always used
		extra:val_type="integer"
		extra:FloatOrder="lsbmsb"
		extra:val_op="MSW"
			see description of param element
        ...
      </unit>
 </device>

 @config_example.pl
<device 
        xmlns:extra="http://www.praterm.com.pl/SZARP/ipk-extra"
        daemon="/opt/szarp/bin/borutadmn" 
        path="/dev/null"
                ignorowany, zaleca siê ustawienie /dev/null
        speed="9600"
                ignorowany
        >

        <unit 
                id="1"
			identyfikator Modbus nasz lub strony przeciwnej (zale¿nie od trybu server/client); je¿eli
			jest liter± lub pojedyncz± cyfr± to interpretowany jest jako znak ASCII (czyli 'A' to 65,
			'1' to 49), wpp. interpretowany jest jako liczba (czyli '01' to 1); zaleca siê u¿ywanie
			zamiast niego jednoznacznego atrybutu extra:od
		extra:id="1"
			identyfikator Modbus jednostki, nadpisuje atrybut 'id', zawsze interpretowany jako liczba
                type="1"
                        ignorowany, powinno byæ "1"
                subtype="1" 
                        ignorowany, powinno byæ "1"
                bufsize="1"
                        wielko¶æ bufora u¶redniania, 1 lub wiêcej
                extra:proto="modbus" 
                        nazwa protoko³u, u¿ywana przez Borutê do ustalenia u¿ywanego sterownika, dla
                        tego sterownika musi byæ "modbus"
        	extra:mode="client" 
                        tryb pracy jednostki, "client" lub "server"; client oznacza stronê aktywn± (master
			dla Modbus RTU, klient dla Modbus TCP); server oznacza stronê pasywn± (slave dla
			Modbus RTU, serwer dla Modbus TCP)
		extra:medium="serial"
                        medium transmisyjne, "serial" dla Modbus TCP, "tcp" dla Modbus TCP
                extra:tcp-address="172.18.2.2"
			adres IP do którego siê pod³±czamy, wymagany dla trybu "client" i medium "tcp"
                extra:tcp-port="23"
			port IP na który siê ³±czymy (mode "client") lub na którym nas³uchujemy (mode "server"),
			wymagany dla medium "tcp",
                extra:path="/dev/ttyS0"
               	  	¶cie¿ka do portu szeregowego dla medium "serial"
                extra:speed="19200"
                        opcjonalna prêdko¶æ portu szeregowego w bps dla medium "serial", domy¶lna to 9600,
                        inne mo¿liwe warto¶ci to 300, 600, 1200, 2400, 4800, 19200, 38400; ustawienie innej
                        warto¶ci spowoduje przyjêcie prêdko¶ci 9600
                extra:parity="even"
                        opcjonalna parzysto¶æ portu dla medium "serial", mo¿liwe warto¶ci to "none" (domy¶lna),
                        "odd" i "even"
                extra:stopbits="1"
                        opcjonalna liczba bitów stopu dla medium "serial", 1 (domy¶lnie) lub 2
		extra:FloatOrder="msblsb"
			kolejno¶æ s³ów dla warto¶ci zajmuj±cych 2 rejestry (typu "long" lub "float"), domy¶lna
			to "msblsb" (najpierw bardziej znacz±ce s³owo), inna mo¿liwo¶æ to "lsbmsb" (najpierw
			mniej znacz±ce s³owo); mo¿e byæ nadpisana dla konkretnego parametru
		extra:nodata-timeout="60"
			czas w sekundach po którym wygasa warto¶æ parametru w przypadku braku komunikacji, domy¶lnie
			to 0 (brak wygasania)
		extra:nodata-value="-1"
			warto¶æ wysy³ana gdy warto¶æ parametru SZARP nie jest dostêpna, domy¶lnie 0
               >
	<param
		elementy param oznaczaj± warto¶æi przesy³ane/odczytywane z urz±dzenia do systemu SZARP
		name="Name:Of:Parameter"
		...
		extra:address="0x12"
			wymagany, podany dziesi±tkowo lub szesnastkowo bezpo¶redni adres Modbus pierwszego rejestru
			zawieraj±cego warto¶æ parametru, od 0 do 65535
		extra:register_type="holding_register"
			typ rejestru Modbus - "holding_register" (domy¶lne) lub "input_register", decyduje która funkcja
			Modbus jest wykorzystywana do czytania warto¶ci rejestru - (0x03 - ReadHoldingRegister or
			0x04 - ReadInputRegister); dla trybu "server" nie ma to znaczenia - akceptowane s± obie funkcje
			zapisuj±ce - zarówno x06 (WriteSingleRegister) jak i 0x10 (WriteMultipleRegisters)
		extra:val_type="integer"
			wymagany, typ kodowania warto¶æ - jeden z "integer", "bcd" (niewspierany dla elementów 'send'),
			"long" lub "float"; warto¶ci typu "integer" i "bcd" zajmuj± 1 rejestr, "long" i "float" zajmuj±
			dwa kolejne rejestry
		extra:FloatOrder="lsbmsb"
			nadpisuje warto¶æ atrybutu FloatOrder jednostki dla konkretnego parametru
		extra:val_op="MSW"
			opcjonalny operator pozwalaj±cy na konwersjê warto¶ci typu float i long na warto¶ci 
			parametrów SZARP; domy¶lnie warto¶æi te zamieniane s± na 16-bitow± reprezentacjê warto¶æi
			w systemie SZARP bezpo¶rednio, jedynie z uwzglêdnieniem precyzji parametru w SZARP; mo¿liwe
			jest jednak przepisanie tych warto¶ci do dwóch parametrów SZARP (tak zwane parametry 
			'kombinowane') co pozwala na nietracenie precyzji i/lub uwzglêdnienie wiêkszego zakresu;
			w tym celu nale¿y skonfigurowaæ 2 parametry SZARP z takimi samymi parametrami dotycz±cymi
			adresu i typu, przy czym jeden z nich powinien mieæ val_op ustawiony na "MSW", a drugi na 
			"LSW" - przyjm± warto¶æ odpowiednio bardziej i mniej znacz±cego s³owa warto¶ci parametru
			Modbus
		...
	/>
	...
	<send
		elementy send oznaczaj± warto¶ci przesy³ane/czytane z systemu SZARP do urz±dzenia
		param="Name:Of:Parameter"
			nazwa parametru, którego warto¶æ mamy przesy³aæ
		extra:address="0x12"
			adres Modbus docelowego rejestru
		extra:register_type="holding_register"
			jak dla elementu "param", ale ma znaczenie tylko dla trybu "server", dla trybu 
			"klient" u¿ywana jest zawsze funkcja 0x10 (WriteMultipleRegisters)
		extra:val_type="integer"
		extra:FloatOrder="lsbmsb"
		extra:val_op="MSW"
			analogicznie jak dla elementu param
        ...
      </unit>
 </device>

 @description_end

*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdexcept>
#include <sstream>
#include <set>
#include <deque>
#include <sys/types.h>
#include <event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "liblog.h"
#include "xmlutils.h"
#include "conversion.h"
#include "zmqhandler.h"
#include "tokens.h"
#include "borutadmn_z.h"
#include "daemonutils.h"
#include "sz4/defs.h"
#include "sz4/util.h"


const unsigned char MB_ERROR_CODE = 0x80;

/** Modbus exception codes. */
const unsigned char MB_ILLEGAL_FUNCTION  = 0x01;
const unsigned char MB_ILLEGAL_DATA_ADDRESS = 0x02;
const unsigned char MB_ILLEGAL_DATA_VALUE = 0x03;
const unsigned char MB_SLAVE_DEVICE_FAILURE = 0x04;
const unsigned char MB_ACKNOWLEDGE = 0x05;
const unsigned char MB_SLAVE_DEVICE_BUSY = 0x06;
const unsigned char MB_MEMORY_PARITY_ERROR  = 0x08;
const unsigned char MB_GATEWAY_PATH_UNAVAILABLE = 0x0A;
const unsigned char MB_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND  = 0x0B;

/** Modbus public function codes. */
const unsigned char MB_F_RHR = 0x03;		/* Read Holding registers */
const unsigned char MB_F_RIR = 0x04;		/* Read Input registers */
const unsigned char MB_F_WSR = 0x06;		/* Write single register */
const unsigned char MB_F_WMR = 0x10;		/* Write multiple registers */

enum REGISTER_TYPE {
	HOLDING_REGISTER,
	INPUT_REGISTER
};

struct PDU {
	unsigned char func_code;
	std::vector<unsigned char> data;
};

struct TCPADU {
	unsigned short trans_id;
	unsigned short proto_id;
	unsigned short length;
	unsigned char unit_id;
	PDU pdu;
};

struct SDU {
	unsigned char unit_id;
	PDU pdu;
};

std::string tcpadu2string(const struct TCPADU& adu) {
	std::stringstream ss;
	ss << "trans_id: " << (int)adu.trans_id;
	ss << "proto_id: " << (int)adu.proto_id;
	ss << "len: " << (int)adu.length;
	ss << "unit: " << (int)adu.unit_id;
	ss << "fun: " << (int)adu.pdu.func_code;
	for (size_t i = 0; i < adu.pdu.data.size(); i++) {
		ss << " " << (int)adu.pdu.data[i];
	}
	return ss.str();
}

std::string sdu2string(const struct SDU& sdu) {
	std::stringstream ss;
	ss << "unit: " << (int)sdu.unit_id;
	ss << "fun: " << (int)sdu.pdu.func_code;
	for (size_t i = 0; i < sdu.pdu.data.size(); i++) {
		ss << " " << (int)sdu.pdu.data[i];
	}
	return ss.str();
}

class modbus_register;
using RMAP = std::map<unsigned short, modbus_register*>;
using RSET = std::set<std::pair<REGISTER_TYPE, unsigned short>>;

class logging_obj {
protected:
	driver_logger* m_log;

public:
	logging_obj(driver_logger* log): m_log(log) {}
};

class register_iface {
public:
	virtual bool is_valid() const = 0;

	virtual int16_t get_val() const = 0;
	virtual void set_val(int16_t val) = 0;

	virtual sz4::nanosecond_time_t get_mod_time() const = 0;
	virtual void set_mod_time(const sz4::nanosecond_time_t&) = 0;
};

class null_register: public register_iface {
public:
	bool is_valid() const override { return false; }
	int16_t get_val() const override { return sz4::no_data<int16_t>(); }
	void set_val(int16_t val) override {}
	sz4::nanosecond_time_t get_mod_time() const override { return sz4::time_trait<sz4::nanosecond_time_t>::invalid_value; }
	void set_mod_time(const sz4::nanosecond_time_t&) override {}
} null_reg;

class parcook_val_op {
public:
	virtual void set_val(zmqhandler* handler, size_t index) = 0;
};

class sender_val_op {
public:
	virtual void update_val(zmqhandler* handler, size_t index) = 0;
};

class register_holder {
public:
	virtual boost::optional<sz4::nanosecond_time_t> get_mod_time() const = 0;
	virtual void set_mod_time(const sz4::nanosecond_time_t&) const = 0;
	virtual register_iface* operator[](size_t) const = 0;
};

template <int N_REGS>
class modbus_registers_holder: public register_holder {
	std::array<register_iface*, N_REGS> m_regs;

public:
	modbus_registers_holder(std::array<register_iface*, N_REGS> regs): m_regs(regs) {}

	boost::optional<sz4::nanosecond_time_t> get_mod_time() const override {
		sz4::nanosecond_time_t mod_time;
		for (uint32_t i = 0; i < N_REGS; ++i) {
			if (!m_regs[i]->is_valid())
				return boost::none;

			mod_time = std::min(mod_time, this->m_regs[i]->get_mod_time());
		}

		return mod_time;
	}

	void set_mod_time(const sz4::nanosecond_time_t& time) const override {
		for (auto reg: m_regs) {
			reg->set_mod_time(time);
		}
	}

	register_iface* operator[](size_t i) const override {
		if (i >= N_REGS) return &null_reg;
		return m_regs[i];
	}
};


template <class val_impl>
class read_val_op: public parcook_val_op, public logging_obj {
	register_holder* m_regs;

public:
	static constexpr uint32_t N_REGS = val_impl::N_REGS;
	using value_type = typename val_impl::value_type;

	read_val_op(register_holder* regs, value_type, int, driver_logger* log): logging_obj(log), m_regs(regs) {}

	void set_val(zmqhandler* handler, size_t index) override {
		if (auto mod_time = m_regs->get_mod_time()) {
			auto val = val_impl::get_value_from_regs(*m_regs);
			handler->set_value(index, *mod_time, val);
		}
	}
};

template <typename val_impl>
class sent_val_op: public sender_val_op, public logging_obj {
public:
	static constexpr uint32_t N_REGS = val_impl::N_REGS;
	using value_type = typename val_impl::value_type;

private:
	register_holder* m_regs;
	int m_prec;
	value_type m_nodata_value;

public:
	sent_val_op(register_holder* regs, value_type nodata_value, int prec, driver_logger* log): logging_obj(log), m_regs(regs), m_prec(prec), m_nodata_value(nodata_value)  {}

	void update_val(zmqhandler* handler, size_t index) override {
		szarp::ParamValue& value = handler->get_value(index);
		auto vt_pair = sz4::cast_param_value<value_type>(value, this->m_prec);
		if (!sz4::is_valid(vt_pair))
			return; // do not overwrite, wait for data timeout instead

		m_regs->set_mod_time(vt_pair.time);
		val_impl::set_value_to_regs(*m_regs, vt_pair.value);
	}
};

template <typename VT>
class modbus_casting_op {
public:
	static constexpr uint32_t N_REGS = sizeof(VT)/2;
	using value_type = VT;

	static VT get_value_from_regs(register_holder& regs) {
		uint16_t v[N_REGS];
		for (uint32_t i = 0; i < N_REGS; i++) {
			v[i] = regs[i]->get_val();
		}

		return *(VT*)v;
	}

	static void set_value_to_regs(register_holder& regs, VT val) {
		uint16_t iv[N_REGS];
		memcpy(iv, &val, sizeof(VT));
		for (uint32_t i = 0; i < N_REGS; i++) {
			regs[i]->set_val(iv[i]);
		}
	}
};

using short_modbus_op = modbus_casting_op<int16_t>;
using long_modbus_op = modbus_casting_op<int32_t>;
using float_modbus_op = modbus_casting_op<float>;
using double_modbus_op = modbus_casting_op<double>;

class bcd_modbus_op {
public:
	static constexpr uint32_t N_REGS = 1;
	using value_type = int16_t;

	static int16_t get_value_from_regs(register_holder& regs) {
		uint16_t val = regs[0]->get_val();
		return int16_t((val & 0xf)
                + ((val >> 4) & 0xf) * 10
                + ((val >> 8) & 0xf) * 100
                + (val >> 12) * 1000);
	}

	static void set_value_to_regs(register_holder& regs, int16_t _val) {
		uint16_t val = _val;
		regs[0]->set_val(val % 10 + (((val / 10) % 10) << 4) + (((val / 100) % 10) << 8) + (((val / 1000) % 10) << 12));
	}
};

class decimal3_modbus_op {
public:
	static constexpr uint32_t N_REGS = 3;
	using value_type = double;

	static value_type get_value_from_regs(register_holder& regs) {
		return (10000 * regs[0]->get_val()) + regs[1]->get_val() + (regs[2]->get_val() / 1000.0);
	}

	static void set_value_to_regs(register_holder& regs, value_type val) {
		regs[2]->set_val(uint64_t(val * 1000) % 1000);
		regs[1]->set_val(uint64_t(val) % 10000);
		regs[0]->set_val(int16_t(val / 10000.0));
	}
};

class RegsFactory: public logging_obj {
	enum FLOAT_ORDER { LSWMSW, MSWLSW } default_float_order = MSWLSW;
	enum DOUBLE_ORDER { LSDMSD, MSDLSD } default_double_order = MSDLSD;

	FLOAT_ORDER get_float_order(TAttribHolder* param, FLOAT_ORDER default_value) const;
	DOUBLE_ORDER get_double_order(TAttribHolder* param, DOUBLE_ORDER default_value) const;

public:
	using logging_obj::logging_obj;
	void configure(TUnit* unit);

	template <uint32_t N_REGS>
	std::array<uint32_t, N_REGS> get_regs_order(TAttribHolder* param, uint32_t addr) const;
};

class modbus_unit {
protected:

	unsigned char m_id;

	RMAP m_registers;

	RSET m_received;
	RSET m_sent;

	size_t m_read;
	size_t m_send;

	std::vector<parcook_val_op*> m_parcook_ops;
	std::vector<sender_val_op*> m_sender_ops;
	std::unique_ptr<RegsFactory> m_regs_factory;

	driver_logger m_log;

	sz4::nanosecond_time_t m_current_time;
	long long m_expiration_time;
	float m_nodata_value;

	template <typename val_op>
	int configure_register(TAttribHolder* param, int prec, unsigned short addr, REGISTER_TYPE rt);

	register_iface* register_for_addr(uint32_t addr, REGISTER_TYPE rt, parcook_val_op*);
	register_iface* register_for_addr(uint32_t addr, REGISTER_TYPE rt, sender_val_op*);
	register_iface* get_register(uint32_t addr);

	void pushValOp(parcook_val_op* op, TAttribHolder* param);
	void pushValOp(sender_val_op* op, TAttribHolder* param);

	template <template <typename> class op_type>
	int configure_param(TAttribHolder* param, int prec_e);
	int configure_unit(TUnit* u);

	const char* error_string(const unsigned char& error);

	bool process_request(unsigned char unit, PDU &pdu);

	void consume_read_regs_response(unsigned short start_addr, unsigned short regs_count, PDU &pdu);
	void consume_write_regs_response(unsigned short start_addr, unsigned short regs_count, PDU &pdu);

	bool perform_write_registers(PDU &pdu);
	bool perform_read_holding_regs(PDU &pdu);

	bool create_error_response(unsigned char error, PDU &pdu);

	void to_parcook();
	void from_sender();
public:
	modbus_unit(boruta_driver* driver);
	bool register_val_expired(const sz4::nanosecond_time_t& time);
	virtual int initialize();
	int configure(TUnit *unit, size_t read, size_t send);
	void finished_cycle();
	void starting_new_cycle();
	virtual zmqhandler* zmq_handler() = 0;
};

class tcp_connection_handler {
public:
	virtual void frame_parsed(TCPADU &adu, struct bufferevent* bufev) = 0;
};

class tcp_parser {
	enum { TR, TR2, PR, PR2, L, L2, U_ID, FUNC, DATA } m_state;
	size_t m_payload_size;

	tcp_connection_handler* m_tcp_handler;
	TCPADU m_adu;
	driver_logger* m_log;
public:
	tcp_parser(tcp_connection_handler *tcp_handler, driver_logger* m_log);
	void send_adu(unsigned short trans_id, unsigned char unit_id, PDU &pdu, struct bufferevent *bufev);
	void read_data(struct bufferevent *bufev);
	void reset();
};

class tcp_server : public modbus_unit, public tcp_connection_handler, public tcp_server_driver {
	std::vector<struct in_addr> m_allowed_ips;
	std::map<struct bufferevent*, tcp_parser*> m_parsers;

	bool ip_allowed(struct sockaddr_in* addr);
public:
	tcp_server();
	const char* driver_name() { return "modbus_tcp_server"; }
	virtual zmqhandler* zmq_handler() { return m_zmq; }
	virtual void frame_parsed(TCPADU &adu, struct bufferevent* bufev);
	virtual int configure(TUnit* unit, size_t read, size_t send);
	virtual int initialize();
	virtual void connection_error(struct bufferevent* bufev);
	virtual void data_ready(struct bufferevent* bufev);
	virtual int connection_accepted(struct bufferevent* bufev, int socket, struct sockaddr_in *addr);
	virtual void starting_new_cycle();
	virtual void finished_cycle();
};

class modbus_client : public modbus_unit {
protected:
	RSET::iterator m_received_iterator;
	RSET::iterator m_sent_iterator;

	PDU m_pdu;

	unsigned short m_start_addr;
	unsigned short m_regs_count;
	REGISTER_TYPE m_register_type;

	enum {
		READING_FROM_PEER,
		WRITING_TO_PEER,
		IDLE,
	} m_state;

	sz4::nanosecond_time_t m_latest_request_sent;
	int m_request_timeout;
	struct event m_next_query_timer;
	struct event m_query_deadine_timer;
	bool m_single_register_pdu;
	unsigned int m_query_interval_ms;

	void starting_new_cycle();
protected:
	modbus_client(boruta_driver *driver);
	void reset_cycle();
	void start_cycle();
	void send_next_query(bool previous_ok);
	void send_write_query();
	void send_read_query();
	void next_query(bool previous_ok);
	void send_query();
	void find_continuous_reg_block(RSET::iterator &i, RSET &regs);
	void timeout();
	bool waiting_for_data();
	void drain_buffer(struct bufferevent* event);

	void schedule_send_query();
	static void next_query_cb(int fd, short event, void* thisptr);
	static void query_deadline_cb(int fd, short event, void* thisptr);

	void connection_error();

	virtual void send_pdu(unsigned char unit, PDU &pdu) = 0;
	virtual void cycle_finished() = 0;
	virtual void terminate_connection() = 0;

public:
	int configure(TUnit *unit, size_t read, size_t send);
	void pdu_received(unsigned char u, PDU &pdu);
	virtual int initialize();
};

class modbus_tcp_client : public tcp_connection_handler, public tcp_client_driver, public modbus_client {
	tcp_parser *m_parser;
	struct bufferevent* m_bufev; 
	unsigned char m_trans_id;
protected:
	virtual void send_pdu(unsigned char unit, PDU &pdu);
	virtual void cycle_finished();
	virtual void terminate_connection();
public:
	modbus_tcp_client();
	const char* driver_name() { return "modbus_tcp_client"; }
	virtual zmqhandler* zmq_handler() { return m_zmq; }
	virtual void frame_parsed(TCPADU &adu, struct bufferevent* bufev);
	virtual void connection_error(struct bufferevent *bufev);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual void finished_cycle();
	virtual void starting_new_cycle();
	virtual int configure(TUnit* unit, size_t read, size_t send);
	virtual void timeout();
};

class serial_connection_handler {
public:
	enum ERROR_CONDITION { TIMEOUT, FRAME_ERROR, CRC_ERROR };
	virtual void frame_parsed(SDU &adu, struct bufferevent* bufev) = 0;
	virtual void error(ERROR_CONDITION) = 0;
	virtual struct event_base* get_event_base() = 0;
	virtual struct bufferevent* get_buffer_event() = 0;
};

class serial_parser {
protected:
	serial_connection_handler *m_serial_handler;
	struct event m_read_timer;
	SDU m_sdu;
	bool m_read_timer_started;

	int m_delay_between_chars;
	std::deque<unsigned char> m_output_buffer;
	struct event m_write_timer;
	bool m_write_timer_started;

	driver_logger* m_log;

	void start_read_timer(long useconds);
	void stop_read_timer();

	void start_write_timer();
	void stop_write_timer();

	virtual void read_timer_event() = 0;
	virtual void write_timer_event();
public:
	serial_parser(serial_connection_handler *serial_handler, driver_logger* log);
	virtual int configure(TUnit* unit, serial_port_configuration &spc) = 0;
	virtual void send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev) = 0;
	virtual void read_data(struct bufferevent *bufev) = 0;
	virtual void write_finished(struct bufferevent *bufev);
	virtual void reset() = 0;

	static void read_timer_callback(int fd, short event, void* serial_parser);
	static void write_timer_callback(int fd, short event, void* serial_parser);
};

class serial_rtu_parser : public serial_parser {
	enum { ADDR, FUNC_CODE, DATA } m_state;
	size_t m_data_read;

	int m_timeout_1_5_c;
	int m_timeout_3_5_c;
	bool m_is_3_5_c_timeout;

	struct bufferevent *m_bufev;

	bool check_crc();

	static void calculate_crc_table();
	static unsigned short crc16[256];
	static bool crc_table_calculated;

	unsigned short update_crc(unsigned short crc, unsigned char c);

	virtual void read_timer_event();
public:
	serial_rtu_parser(serial_connection_handler *serial_handler, driver_logger* log);
	void read_data(struct bufferevent *bufev);
	void send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev);
	void reset();
	virtual int configure(TUnit* unit, serial_port_configuration &spc);

	static const int max_frame_size = 256;
};

class serial_ascii_parser : public serial_parser {
	unsigned char m_previous_char;
	enum { COLON, ADDR_1, ADDR_2, FUNC_CODE_1, FUNC_CODE_2, DATA_1, DATA_2, LF } m_state;

	int m_timeout;

	void reset();

	bool check_crc();

	unsigned char update_crc(unsigned char c, unsigned char crc) const;
	unsigned char finish_crc(unsigned char c) const;


	virtual void read_timer_event();
public:
	serial_ascii_parser(serial_connection_handler *serial_handler, driver_logger* log);
	void read_data(struct bufferevent *bufev);
	void send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev);
	virtual int configure(TUnit* unit, serial_port_configuration &spc);
};

class modbus_serial_client : public serial_connection_handler, public serial_client_driver, public modbus_client {
	serial_parser *m_parser;
	struct bufferevent* m_bufev;
protected:
	virtual void send_pdu(unsigned char unit, PDU &pdu);
	virtual void cycle_finished();
	virtual void terminate_connection();
public:
	modbus_serial_client();
	const char* driver_name() { return "modbus_serial_client"; }
	virtual zmqhandler* zmq_handler() { return m_zmq; }
	virtual void starting_new_cycle();
	virtual void finished_cycle();
	virtual void connection_error(struct bufferevent *bufev);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual int configure(TUnit* unit, size_t read, size_t send, serial_port_configuration &spc);
	virtual void frame_parsed(SDU &adu, struct bufferevent* bufev);
	virtual void error(ERROR_CONDITION error);
	virtual struct event_base* get_event_base();
	virtual struct bufferevent* get_buffer_event();
};

unsigned short serial_rtu_parser::crc16[256] = {};
bool serial_rtu_parser::crc_table_calculated = false;

class serial_server : public modbus_unit, public serial_connection_handler, public serial_server_driver {
	serial_parser* m_parser;
	struct bufferevent *m_bufev;

public:
	serial_server();
	const char* driver_name() { return "modbus_serial_server"; }
	virtual zmqhandler* zmq_handler() { return m_zmq; }
	virtual int configure(TUnit *unit, size_t read, size_t send, serial_port_configuration &spc);
	virtual int initialize();
	virtual void frame_parsed(SDU &sdu, struct bufferevent* bufev);
	virtual struct bufferevent* get_buffer_event();

	virtual void connection_error(struct bufferevent* bufev);
	virtual void data_ready(struct bufferevent* bufev);
	virtual struct event_base* get_event_base();
	virtual void error(serial_connection_handler::ERROR_CONDITION error);
	virtual void starting_new_cycle();
	virtual void finished_cycle();
};

class modbus_register: public register_iface, public logging_obj {
	friend class modbus_unit;
	modbus_unit *m_modbus_unit;

	// allow accessing data members
	int16_t m_val = sz4::no_data<int16_t>();
	sz4::nanosecond_time_t m_mod_time = sz4::time_trait<sz4::nanosecond_time_t>::invalid_value;

public:
	modbus_register(modbus_unit *daemon, driver_logger* log);
	bool is_valid() const override;

	void set_val(int16_t val) override { m_val = val; }
	int16_t get_val() const override { return m_val; }

	sz4::nanosecond_time_t get_mod_time() const override { return m_mod_time; }
	void set_mod_time(const sz4::nanosecond_time_t& time) override { m_mod_time = time; }
};

modbus_register::modbus_register(modbus_unit* unit, driver_logger* log) : logging_obj(log), m_modbus_unit(unit), m_val(sz4::no_data<short>()), m_mod_time(sz4::time_trait<sz4::nanosecond_time_t>::invalid_value) {}

bool modbus_register::is_valid() const {
	return sz4::time_trait<sz4::nanosecond_time_t>::is_valid(m_mod_time) && !m_modbus_unit->register_val_expired(m_mod_time);
}

bool modbus_unit::process_request(unsigned char unit, PDU &pdu) {
	if (m_id != unit && unit != 0) {
		m_log.log(1, "Received request to unit %d, not such unit in configuration, ignoring", (int) unit);
		return false;
	}

	try {

		switch (pdu.func_code) {
			case MB_F_RHR:
				return perform_read_holding_regs(pdu);
			case MB_F_WSR:
			case MB_F_WMR:
				return perform_write_registers(pdu);
			default:
				return create_error_response(MB_ILLEGAL_FUNCTION, pdu);
		}

	} catch (std::out_of_range) {	//message was distorted
		m_log.log(1, "Error while processing request - there is either a bug or sth was very wrong with request format");
		return false;
	}

}

const char* modbus_unit::error_string(const unsigned char& error) {
	switch (error) {
		case MB_ILLEGAL_FUNCTION:
			return "ILLEGAL FUNCTION";
		case MB_ILLEGAL_DATA_ADDRESS:
			return "ILLEGAL DATA ADDRESS";
		case MB_ILLEGAL_DATA_VALUE:
			return "ILLEGAL DATA VALUE";
		case MB_SLAVE_DEVICE_FAILURE:
			return "SLAVE DEVCE FAILURE";
		case MB_ACKNOWLEDGE:
			return "ACKNOWLEDGE";
		case MB_SLAVE_DEVICE_BUSY:
			return "SLAVE DEVICE FAILURE";
		case MB_MEMORY_PARITY_ERROR:
			return "MEMORY PARITY ERROR";
		case MB_GATEWAY_PATH_UNAVAILABLE:
			return "GATEWAY PATH UNAVAILABLE";
		case MB_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND:
			return "GATEWAY TARGET DEVICE FAILED TO RESPOND";
		default:
			return "UNKNOWN/UNSUPPORTED ERROR CODE";

	}
}

void modbus_unit::consume_read_regs_response(unsigned short start_addr, unsigned short regs_count, PDU &pdu) {
	m_log.log(5, "Consuming read holding register response unit_id: %d, address: %hu, registers count: %hu", (int) m_id, start_addr, regs_count);
	if (pdu.func_code & MB_ERROR_CODE) {
		m_log.log(1, "Exception received in response to read holding registers command, unit_id: %d, address: %hu, count: %hu",
		       	(int)m_id, start_addr, regs_count);
		m_log.log(1, "Error is: %s(%d)", error_string(pdu.data.at(0)), (int)pdu.data.at(0));
		return;
	} 

	if (pdu.data.size() - 1 != regs_count * 2) {
		m_log.log(1, "Unexpected data size in response to read holding registers command, requested %hu regs but got %zu data in response",
		       	regs_count, pdu.data.size() - 1);
		throw std::out_of_range("Invalid response size");
	}

	size_t data_index = 1;

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++, data_index += 2) {
		RMAP::iterator j = m_registers.find(addr);
		uint16_t v = ((uint16_t)(pdu.data.at(data_index)) << 8) | pdu.data.at(data_index + 1);
		m_log.log(9, "Setting register unit_id: %d, address: %hu, value: %hu", (int) m_id, (int) addr, v);
		j->second->m_val = v;
		j->second->m_mod_time = m_current_time;
	}
}

void modbus_unit::consume_write_regs_response(unsigned short start_addr, unsigned short regs_count, PDU &pdu) { 
	m_log.log(5, "Consuming write holding register response unit_id: %u, address: %hu, registers count: %hu", m_id, start_addr, regs_count);
	if (pdu.func_code & MB_ERROR_CODE) {
		m_log.log(1, "Exception received in response to write holding registers command, unit_id: %u, address: %hu, count: %hu",
			m_id, start_addr, regs_count);
		m_log.log(1, "Error is: %s(%d)", error_string(pdu.data.at(0)), (int)pdu.data.at(0));
	} else {
		m_log.log(5, "Write holding register command poisitively acknowledged");
	}
}

bool modbus_unit::perform_write_registers(PDU &pdu) {
	std::vector<unsigned char>& d = pdu.data;

	unsigned short start_addr;
	unsigned short regs_count;
	size_t data_index;


	start_addr = (d.at(0) << 8) | d.at(1);

	if (pdu.func_code == MB_F_WMR) {
		regs_count = (d.at(2) << 8) | d.at(3);
		data_index = 5;
	} else {
		regs_count = 1;
		data_index = 2;
	}

	m_log.log(7, "Processing write holding request registers start_addr: %hu, regs_count:%hu", start_addr, regs_count);

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++, data_index += 2) {
		RMAP::iterator j = m_registers.find(addr);
		if (j == m_registers.end())
			return create_error_response(MB_ILLEGAL_DATA_ADDRESS, pdu);

		uint16_t v = (d.at(data_index) << 8) | d.at(data_index + 1);

		m_log.log(9, "Setting register: %hu value: %hu", (int) addr, v);
		j->second->m_val = v;	
		j->second->m_mod_time = m_current_time;
	}

	if (pdu.func_code == MB_F_WMR)
		d.resize(4);

	m_log.log(7, "Request processed successfully");

	return true;
}

bool modbus_unit::perform_read_holding_regs(PDU &pdu) {
	std::vector<unsigned char>& d = pdu.data;
	uint16_t start_addr = (d.at(0) << 8) | d.at(1);
	uint16_t regs_count = (d.at(2) << 8) | d.at(3);

	m_log.log(7, "Responding to read holding registers request start_addr: %hu, regs_count:%hu", start_addr, regs_count);

	d.resize(1);
	d.at(0) = 2 * regs_count;

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++) {
		RMAP::iterator j = m_registers.find(addr);
		if (j == m_registers.end())
			return create_error_response(MB_ILLEGAL_DATA_ADDRESS, pdu);

		uint16_t v = j->second->get_val();
		d.push_back(v >> 8);
		d.push_back(v & 0xff);
		m_log.log(9, "Sending register: %hu, value: %hu", (int) addr, v);
	}

	m_log.log(7, "Request processed sucessfully.");

	return true;
}

bool modbus_unit::create_error_response(unsigned char error, PDU &pdu) {
	pdu.func_code |= MB_ERROR_CODE;
	pdu.data.resize(1);
	pdu.data[0] = error;

	m_log.log(7, "Sending error response: %s", error_string(error));

	return true;
}

void modbus_unit::to_parcook() {
	auto zmq = zmq_handler();
	size_t m = 0;
	for (auto reg: m_parcook_ops)
		reg->set_val(zmq, m_read + m++);
}

void modbus_unit::from_sender() {
	auto zmq = zmq_handler();
	size_t m = 0;
	for (auto sent: m_sender_ops)
		sent->update_val(zmq, m_send + m++);
}

bool modbus_unit::register_val_expired(const sz4::nanosecond_time_t& time) {
	if (m_expiration_time == 0)
		return false;
	else {
		auto dt = m_current_time;
		dt += m_expiration_time;
	
		return time >= dt;
	}
}

modbus_unit::modbus_unit(boruta_driver* driver) : m_log(driver) {
	// in c++14 change this to make_uniqe
	m_regs_factory = std::unique_ptr<RegsFactory>(new RegsFactory(&m_log));
}

std::string get_param_name(TAttribHolder* param) {
	using str = std::string;
	if (auto name = param->getOptAttribute<std::string>("name")) {
		return *name;
	} else if (auto name = param->getOptAttribute<std::string>("param")) {
		return *name + str(" (sent)");
	} else if (auto value = param->getOptAttribute<std::string>("value")) {
		return str("sent value ")+*value;
	} else {
		ASSERT(!"Couldn't get param name!");
		return str("");
	}
}

void RegsFactory::configure(TUnit* unit) {
	default_float_order = get_float_order(unit, MSWLSW);
	if (default_float_order == MSWLSW)
		m_log->log(5, "Setting mswlsw float order");
	else if (default_float_order == LSWMSW)
		m_log->log(5, "Setting lswmsw float order");
	
	default_double_order = get_double_order(unit, MSDLSD);
	if (default_double_order == MSDLSD)
		m_log->log(5, "Setting msdlsd double order");
	else if (default_double_order == LSDMSD)
		m_log->log(5, "Setting lsdmsd double order");
}

RegsFactory::FLOAT_ORDER RegsFactory::get_float_order(TAttribHolder* param, FLOAT_ORDER default_value) const {
	std::string float_order = param->getAttribute<std::string>("extra:FloatOrder", "");
	if (float_order.empty()) {
		return default_value;
	} else if (float_order == "msblsb") {
		return MSWLSW;
	} else if (float_order == "lsbmsb") {
		return LSWMSW;
	} else {
		throw std::runtime_error(std::string("Invalid float order specification: ") + float_order);
	}
}

RegsFactory::DOUBLE_ORDER RegsFactory::get_double_order(TAttribHolder* param, DOUBLE_ORDER default_value) const {
	std::string double_order = param->getAttribute<std::string>("extra:DoubleOrder", "");;
	if (double_order.empty()) {
		return default_value;
	} else if (double_order == "msdlsd") {
		return MSDLSD;
	} else if (double_order == "lsdmsd") {
		return LSDMSD;
	} else {
		throw std::runtime_error(std::string("Invalid double order specification: ") + double_order);
	}
}

template <uint32_t N_REGS>
std::array<uint32_t, N_REGS> RegsFactory::get_regs_order(TAttribHolder* param, uint32_t addr) const {
	std::array<uint32_t, N_REGS> regs;
	for (uint16_t i = 0; i < N_REGS; ++i) {
		regs[i] = addr + i;
	}
	return regs;
}

template <>
std::array<uint32_t, 2> RegsFactory::get_regs_order<2>(TAttribHolder* param, uint32_t addr) const {
	FLOAT_ORDER float_order = get_float_order(param, default_float_order);

	if (float_order == MSWLSW) {
		return {addr + 1u, addr};
	} else {
		return {addr, addr + 1u};
	}
}

template <>
std::array<uint32_t, 4> RegsFactory::get_regs_order<4>(TAttribHolder* param, uint32_t addr) const {
	DOUBLE_ORDER double_order = get_double_order(param, default_double_order);
	auto lsa = get_regs_order<2>(param, addr);
	auto msa = get_regs_order<2>(param, addr+2);

	if (double_order == MSDLSD) {
		return {msa[0], msa[1], lsa[0], lsa[1]};
	} else {
		return {lsa[0], lsa[1], msa[0], msa[1]};
	}
}

register_iface* modbus_unit::register_for_addr(uint32_t addr, REGISTER_TYPE rt, parcook_val_op*) {
	m_received.insert(std::make_pair(rt, addr));
	return get_register(addr);
}

register_iface* modbus_unit::register_for_addr(uint32_t addr, REGISTER_TYPE rt, sender_val_op*) {
	m_sent.insert(std::make_pair(rt, addr));
	return get_register(addr);
}

register_iface* modbus_unit::get_register(uint32_t addr) {
	if (m_registers.find(addr) == m_registers.end()) {
		m_registers[addr] = new modbus_register(this, &m_log);
	}
	return m_registers[addr];
}

void modbus_unit::pushValOp(parcook_val_op* op, TAttribHolder* param) {
	m_parcook_ops.push_back(op);
}

void modbus_unit::pushValOp(sender_val_op* op, TAttribHolder* param) {
	m_sender_ops.push_back(op);
}

template <typename val_op>
int modbus_unit::configure_register(TAttribHolder* param, int prec, unsigned short addr, REGISTER_TYPE rt) {
	std::array<uint32_t, val_op::N_REGS> r_addrs = m_regs_factory->get_regs_order<val_op::N_REGS>(param, addr);
	std::array<register_iface*, val_op::N_REGS> registers;

	int32_t i = 0;
	for (auto r_addr: r_addrs) {
		registers[i++] = register_for_addr(r_addr, rt, (val_op*) nullptr); // tag dispatch
	}
	
	pushValOp(new val_op(new modbus_registers_holder<val_op::N_REGS>(registers), m_nodata_value, prec, &m_log), param);
	m_log.log(8, "Param %s mapped to unit: %u, register %hu", get_param_name(param).c_str(), m_id, addr);
	return 0;
}

template <template <typename> class op_type>
int modbus_unit::configure_param(TAttribHolder* param, int prec_e) { 
	unsigned short addr;
	if (auto l = param->getOptAttribute<long>("extra:address")) {
		if (*l < 0 || *l > 65535) {
			m_log.log(1, "Invalid address attribute value: %ld, should be between 0 and 65535", *l);
			return 1;
		}
		addr = *l;
	} else {
		m_log.log(1, "No address attribute value!");
		return 1;
	}

	REGISTER_TYPE rt;
	auto reg_type_attr = param->getAttribute<std::string>("extra:register_type", "holding_register");
	if (reg_type_attr == "holding_register") {
		rt = HOLDING_REGISTER;
	} else if (reg_type_attr == "input_register") {
		rt = INPUT_REGISTER;
	} else {
		m_log.log(1, "Unsupported register type, should be either input_register or holding_register");
		return 1;
	}

	std::string val_type = param->getAttribute<std::string>("extra:val_type", "");
	if (val_type.empty()) {
		m_log.log(1, "No val_type provided!");
		return 1;
	}

	int prec = exp10(prec_e);

	int ret;
	if (val_type == "integer") {
		ret = configure_register<op_type<short_modbus_op>>(param, prec, addr, rt);
	} else if (val_type == "bcd") {
		ret = configure_register<op_type<bcd_modbus_op>>(param, prec, addr, rt);
	} else if (val_type == "long") {
		ret = configure_register<op_type<long_modbus_op>>(param, prec, addr, rt);
	} else if (val_type == "float") {
		ret = configure_register<op_type<float_modbus_op>>(param, prec, addr, rt);
	} else if (val_type == "double") {
		ret = configure_register<op_type<double_modbus_op>>(param, prec, addr, rt);
	} else if (val_type == "decimal3") {
		ret = configure_register<op_type<decimal3_modbus_op>>(param, prec, addr, rt);
	} else {
		m_log.log(1, "Unsupported value type: %s", val_type.c_str());
		ret = 1;
	}

	return ret;
}

int modbus_unit::configure_unit(TUnit* u) {
	std::string _id = u->getAttribute<std::string>("extra:id", "");;
	if (_id.empty()) {
		switch (u->GetId()) {
			case L'0'...L'9':
				m_id = u->GetId() - L'0';
				break;
			case L'a'...L'z':
				m_id = (u->GetId() - L'a') + 10;
				break;
			case L'A'...L'Z':
				m_id = (u->GetId() - L'A') + 10;
				break;
			default:
				m_id = u->GetId();
				break;
		}
	} else {
		m_id = strtol(_id.c_str(), NULL, 0);
	}

	for (auto p: u->GetParams()) {
		if (configure_param<read_val_op>(p, p->GetPrec())) {
			m_log.log(0, "Error in param %ls", p->GetName().c_str());
			return 1;
		}
	}

	for (auto p: u->GetSendParams()) {
		if (configure_param<sent_val_op>(p, p->GetPrec())) {
			m_log.log(0, "Error in send %ls", p->GetParamName().c_str());
			return 1;
		}
	}

	return 0;
}

int modbus_unit::configure(TUnit *unit, size_t read, size_t send) {
	m_regs_factory->configure(unit);

	m_expiration_time = unit->getAttribute("extra:nodata-timeout", 60);
	m_expiration_time *= 1000000000;

	m_nodata_value = unit->getAttribute("extra:nodata-value", sz4::no_data<float>());
	m_log.log(9, "No data value set to: %f", m_nodata_value);

	if (configure_unit(unit))
		return 1;

	m_read = read;
	m_send = send;

	return 0;
}

void modbus_unit::finished_cycle() {
	to_parcook();
}

void modbus_unit::starting_new_cycle() {
	m_log.log(5, "Timer click, doing ipc data exchange");
	zmq_handler()->receive();
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	m_current_time.second = time.tv_sec;
	m_current_time.nanosecond = time.tv_nsec;
	from_sender();
}

int modbus_unit::initialize() {
	return 0;
}

tcp_parser::tcp_parser(tcp_connection_handler *tcp_handler, driver_logger *log) : m_state(TR), m_payload_size(0), m_tcp_handler(tcp_handler), m_log(log)
{ }

void tcp_parser::reset() {
	m_state = TR;
	m_adu.pdu.data.resize(0);
};

void tcp_parser::read_data(struct bufferevent *bufev) {
	unsigned char c;

	while (bufferevent_read(bufev, &c, 1) == 1) switch (m_state) {
		case TR:
			m_log->log(8, "Started to parse new tcp frame");
			m_adu.pdu.data.resize(0);
			m_adu.trans_id = c;
			m_state = TR2;
			break;
		case TR2:
			m_adu.trans_id |= (c << 8);
			m_state = PR;
			m_adu.trans_id = ntohs(m_adu.trans_id);
			m_log->log(8, "Transaction id: %hu", m_adu.trans_id);
			break;
		case PR:
			m_adu.proto_id = c;
			m_state = PR2;
			break;
		case PR2:
			m_state = L;
			m_adu.proto_id |= (c << 8);
			m_adu.proto_id = ntohs(m_adu.proto_id);
			m_log->log(8, "Protocol id: %hu", m_adu.proto_id);
			break;
		case L:
			m_state = L2;
			m_adu.length = c;
			break;
		case L2:
			m_state = U_ID;
			m_adu.length |= (c << 8);
			m_adu.length = ntohs(m_adu.length);
			m_payload_size = m_adu.length - 2;
			m_log->log(8, "Data size: %zu", m_payload_size);
			break;
		case U_ID:
			m_state = FUNC;
			m_adu.unit_id = c;
			m_log->log(8, "Unit id: %u", m_adu.unit_id);
			break;
		case FUNC:
			m_state = DATA;
			m_adu.pdu.func_code = c;
			m_log->log(8, "Func code: %d", (int)m_adu.pdu.func_code);
			break;
		case DATA:
			m_adu.pdu.data.push_back(c);
			m_payload_size -= 1;
			if (m_payload_size == 0) {
				m_tcp_handler->frame_parsed(m_adu, bufev);
				m_state = TR;
				return;
			}
			break;
		}

}

void tcp_parser::send_adu(unsigned short trans_id, unsigned char unit_id, PDU &pdu, struct bufferevent *bufev) {
	unsigned short proto_id = 0;
	unsigned short length;

	trans_id = htons(trans_id);
	length = htons(2 + pdu.data.size());
	
	bufferevent_write(bufev, &trans_id, sizeof(trans_id));
	bufferevent_write(bufev, &proto_id, sizeof(proto_id));
	bufferevent_write(bufev, &length, sizeof(length));
	bufferevent_write(bufev, &unit_id, sizeof(unit_id));
	bufferevent_write(bufev, &pdu.func_code, sizeof(pdu.func_code));
	bufferevent_write(bufev, &pdu.data[0], pdu.data.size());
}

bool tcp_server::ip_allowed(struct sockaddr_in* addr) {
	if (m_allowed_ips.size() == 0)
		return true;
	for (size_t i = 0; i < m_allowed_ips.size(); i++)
		if (m_allowed_ips[i].s_addr == addr->sin_addr.s_addr)
			return true;

	return false;
}

tcp_server::tcp_server() : modbus_unit(this) {}

void tcp_server::frame_parsed(TCPADU &adu, struct bufferevent* bufev) {
	if (process_request(adu.unit_id, adu.pdu) == true)
		m_parsers[bufev]->send_adu(adu.trans_id, adu.unit_id, adu.pdu, bufev);
}

int tcp_server::configure(TUnit* unit, size_t read, size_t send) {
	if (modbus_unit::configure(unit, read, send))
		return 1;
	std::string ips_allowed = unit->getAttribute<std::string>("extra:tcp-allowed", "");;
	std::stringstream ss(ips_allowed);
	std::string ip_allowed;
	while (ss >> ip_allowed) {
		struct in_addr ip;
		int ret = inet_aton(ip_allowed.c_str(), &ip);
		if (ret == 0) {
			m_log.log(1, "incorrect IP address '%s'", ip_allowed.c_str());
			return 1;
		} else {
			m_log.log(5, "IP address '%s' allowed", ip_allowed.c_str());
			m_allowed_ips.push_back(ip);
		}
	}
	if (m_allowed_ips.size() == 0)
		m_log.log(1, "warning: all IP allowed");	
	return 0;
}

int tcp_server::initialize() {
	return 0;
}
	
void tcp_server::data_ready(struct bufferevent* bufev) {
	m_parsers[bufev]->read_data(bufev);
}

int tcp_server::connection_accepted(struct bufferevent* bufev, int socket, struct sockaddr_in *addr) {
	if (!ip_allowed(addr))
		return 1;
	m_parsers[bufev] = new tcp_parser(this, &m_log);
	return 0;
}

void tcp_server::starting_new_cycle() {
	modbus_unit::starting_new_cycle();
}

void tcp_server::finished_cycle() {
	modbus_unit::finished_cycle();
}

void tcp_server::connection_error(struct bufferevent *bufev) {
	tcp_parser* parser = m_parsers[bufev];
	m_log.log(5, "Terminating connection");
	m_parsers.erase(bufev);
	delete parser;
}

void modbus_client::starting_new_cycle() {
	modbus_unit::starting_new_cycle();
	switch (m_state)  {
		case IDLE:
			return;
		case READING_FROM_PEER:
			m_log.log(2, "New cycle started, we are in read cycle");
			break;
		case WRITING_TO_PEER:
			m_log.log(2, "New cycle started, we are in write cycle");
			break;
	}
}


modbus_client::modbus_client(boruta_driver* driver) : modbus_unit(driver), m_state(IDLE), m_request_timeout(0)
{
}

void modbus_client::reset_cycle() {
	m_received_iterator = m_received.begin();
	m_sent_iterator = m_sent.begin();
}

void modbus_client::start_cycle() {
	m_log.log(7, "Starting cycle");
	send_next_query(true);
}

void modbus_client::send_next_query(bool previous_ok) {

	switch (m_state) {
		case IDLE:
		case READING_FROM_PEER:
		case WRITING_TO_PEER:
			next_query(previous_ok);
			schedule_send_query();
			break;
		default:
			ASSERT(false);
			break;
	}
}

void modbus_client::schedule_send_query() {
	unsigned int wait_ms = m_query_interval_ms;
	const struct timeval tv = ms2timeval(wait_ms);
	switch (m_state) {
		case READING_FROM_PEER:
		case WRITING_TO_PEER:
			evtimer_add(&m_next_query_timer, &tv);
			m_log.log(10, "schedule next query in %dms", wait_ms);
			break;
		default:
			break;
	}
}

void modbus_client::next_query_cb(int fd, short event, void* thisptr) {
	reinterpret_cast<modbus_client*>(thisptr)->send_query();
}

void modbus_client::query_deadline_cb(int fd, short event, void* thisptr) {
	modbus_client* _this = reinterpret_cast<modbus_client*>(thisptr);

	switch (_this->m_state) {
		case READING_FROM_PEER:
		case WRITING_TO_PEER:
			_this->m_log.log(8, "Query timed out, progressing with queries");
			_this->send_next_query(false);
			break;
		default:
			return;
	}
}

void modbus_client::connection_error() {
	event_del(&m_query_deadine_timer);
}

void modbus_client::send_write_query() {
	m_pdu.func_code = MB_F_WMR;	
	m_pdu.data.resize(0);
	m_pdu.data.push_back(m_start_addr >> 8);
	m_pdu.data.push_back(m_start_addr & 0xff);
	m_pdu.data.push_back(m_regs_count >> 8);
	m_pdu.data.push_back(m_regs_count & 0xff);
	m_pdu.data.push_back(m_regs_count * 2);

	m_log.log(7, "Sending write multiple registers command, start register: %hu, registers count: %hu", m_start_addr, m_regs_count);
	for (size_t i = 0; i < m_regs_count; i++) {
		uint16_t v = m_registers[m_start_addr + i]->get_val();
		m_pdu.data.push_back(v >> 8);
		m_pdu.data.push_back(v & 0xff);
		m_log.log(7, "Sending register: %zu, value: %hu:", m_start_addr + i, v);
	}

	send_pdu(m_id, m_pdu);
}

void modbus_client::send_read_query() {
	m_log.log(7, "Sending read holding registers command, unit_id: %u,  address: %hu, count: %hu", m_id, m_start_addr, m_regs_count);
	switch (m_register_type) {
		case INPUT_REGISTER:
			m_pdu.func_code = MB_F_RIR;
			break;
		case HOLDING_REGISTER:
			m_pdu.func_code = MB_F_RHR;
			break;
	}
	m_pdu.data.resize(0);
	m_pdu.data.push_back(m_start_addr >> 8);
	m_pdu.data.push_back(m_start_addr & 0xff);
	m_pdu.data.push_back(m_regs_count >> 8);
	m_pdu.data.push_back(m_regs_count & 0xff);

	send_pdu(m_id, m_pdu);
}

void modbus_client::next_query(bool previous_ok) {
	switch (m_state) {
		case IDLE:
			m_state = READING_FROM_PEER;
		case READING_FROM_PEER:
			if (m_received_iterator != m_received.end()) {
				find_continuous_reg_block(m_received_iterator, m_received);
				break;
			} 
			m_state = WRITING_TO_PEER;
		case WRITING_TO_PEER:
			if (m_sent_iterator != m_sent.end()) {
				find_continuous_reg_block(m_sent_iterator, m_sent);
				break;
			}
			m_state = IDLE;
			reset_cycle();
			if (previous_ok)
				cycle_finished();
			else {
				m_log.log(1, "Previous query failed and we have just finished our cycle - teminating connection");
				terminate_connection();
			}
			break;
	}
}

void modbus_client::send_query() {
	m_log.log(10, "send_query");
	switch (m_state) {
		default:
			return;
		case READING_FROM_PEER:
			send_read_query();
			break;
		case WRITING_TO_PEER:
			send_write_query();
			break;
	}

	const struct timeval tv = ms2timeval(m_request_timeout * 1000);
	event_add(&m_query_deadine_timer, &tv);
}

void modbus_client::timeout() {
	event_del(&m_query_deadine_timer);

	switch (m_state) {
		case READING_FROM_PEER:
			m_log.log(1, "Timeout while reading data, unit_id: %d, address: %hu, registers count: %hu, progressing with queries",
					(int)m_id, m_start_addr, m_regs_count);

			send_next_query(false);
			break;
		case WRITING_TO_PEER:
			m_log.log(1, "Timeout while writing data, unit_id: %d, address: %hu, registers count: %hu, progressing with queries",
					(int)m_id, m_start_addr, m_regs_count);
			send_next_query(false);
			break;
		default:
			m_log.log(1, "Received unexpected timeout from connection, ignoring.");
			break;
	}
}

bool modbus_client::waiting_for_data() {
	return !evtimer_pending(&m_next_query_timer, NULL);
}

void modbus_client::drain_buffer(struct bufferevent* bufev) {
	char c;
	while (bufferevent_read(bufev, &c, 1));
}


void modbus_client::find_continuous_reg_block(RSET::iterator &i, RSET &regs) {
	unsigned short current;
	ASSERT(i != regs.end());

	m_start_addr = current = i->second;
	m_regs_count = 1;
	m_register_type = i->first;

	if (m_single_register_pdu) {
		++i;
		return;
	}
	while (++i != regs.end() && m_regs_count < 125) {
		if (m_register_type != i->first)
			break;
		unsigned short addr = i->second;
		if (++current != addr)
			break;
		m_regs_count += 1;
	}
}

int modbus_client::configure(TUnit *unit, size_t read, size_t send) {
	m_single_register_pdu = unit->getAttribute("extra:single-register-pdu", false);
	m_query_interval_ms = unit->getAttribute("extra:query-interval", 0);
	m_request_timeout = unit->getAttribute("extra:request-timeout", 10);

	evtimer_set(&m_next_query_timer, next_query_cb, this);
	evtimer_set(&m_query_deadine_timer, query_deadline_cb, this);

	return modbus_unit::configure(unit, read, send);
}

void modbus_client::pdu_received(unsigned char u, PDU &pdu) {
	if (u != m_id && u != 0) {
		m_log.log(1, "Received PDU from unit %d while we wait for response from unit %d, ignoring it!", (int)u, (int)m_id);
		return;
	}

	switch (m_state) {
		case READING_FROM_PEER:
			try {
				consume_read_regs_response(m_start_addr, m_regs_count, pdu);

				send_next_query(true);
			} catch (std::out_of_range&) {	//message was distorted
				send_next_query(false);
			}
			break;
		case WRITING_TO_PEER:
			try {
				consume_write_regs_response(m_start_addr, m_regs_count, pdu);

				send_next_query(true);
			} catch (std::out_of_range&) {	//message was distorted
				send_next_query(false);
			}
			break;
		default:
			m_log.log(10, "Received unexpected response from slave - ignoring it.");
			break;
	}

	switch (m_state) {
		case IDLE:
			event_del(&m_query_deadine_timer);
			break;
		default:
			break;
	}
}

int modbus_client::initialize() {
	return 0;
}

void modbus_tcp_client::send_pdu(unsigned char unit, PDU &pdu) {
	m_trans_id++;
	m_parser->send_adu(m_trans_id, unit, pdu, m_bufev);
}

void modbus_tcp_client::cycle_finished() {
	m_manager->driver_finished_job(this);
}

void modbus_tcp_client::terminate_connection() {
	m_manager->terminate_connection(this);
}

modbus_tcp_client::modbus_tcp_client() : modbus_client(this) {}

void modbus_tcp_client::frame_parsed(TCPADU &adu, struct bufferevent* bufev) {
	
	if (m_trans_id != adu.trans_id) {
		m_log.log(1, "Received unexpected tranasction id in response: %u, expected: %u, ignoring the response", unsigned(adu.trans_id), unsigned(m_trans_id));
		return;
	}

	pdu_received(adu.unit_id, adu.pdu);
}

void modbus_tcp_client::finished_cycle() {
	modbus_unit::finished_cycle();
}

void modbus_tcp_client::starting_new_cycle() {
	modbus_client::starting_new_cycle();
}

void modbus_tcp_client::connection_error(struct bufferevent *bufev) {
	modbus_client::connection_error();

	m_bufev = NULL;
	m_state = IDLE;
	m_parser->reset();
}

void modbus_tcp_client::scheduled(struct bufferevent* bufev, int fd) {
	m_bufev = bufev;
	start_cycle();
}

void modbus_tcp_client::data_ready(struct bufferevent* bufev, int fd) {
	if (waiting_for_data())
		m_parser->read_data(bufev);
	else
		drain_buffer(bufev);
}

int modbus_tcp_client::configure(TUnit* unit, size_t read, size_t send) {
	if (modbus_client::configure(unit, read, send))
		return 1;

	event_base_set(m_event_base, &m_next_query_timer);
	event_base_set(m_event_base, &m_query_deadine_timer);

	m_trans_id = 0;
	m_parser = new tcp_parser(this, &m_log);
	modbus_client::reset_cycle();
	return 0;
}

void modbus_tcp_client::timeout() {
	modbus_client::timeout();
}

void modbus_serial_client::send_pdu(unsigned char unit, PDU &pdu) {
	m_parser->send_sdu(unit, pdu, m_bufev);
}

void modbus_serial_client::cycle_finished() {
	m_manager->driver_finished_job(this);
}

void modbus_serial_client::terminate_connection() {
	m_manager->terminate_connection(this);
}

void modbus_serial_client::finished_cycle() {
	modbus_unit::finished_cycle();
}

modbus_serial_client::modbus_serial_client() : modbus_client(this) {}

void modbus_serial_client::starting_new_cycle() {
	modbus_client::starting_new_cycle();
}

void modbus_serial_client::connection_error(struct bufferevent *bufev) {
	modbus_client::connection_error();

	m_bufev = NULL;
	m_state = IDLE;
	m_parser->reset();
}

void modbus_serial_client::scheduled(struct bufferevent* bufev, int fd) {
	m_bufev = bufev;
	start_cycle();
}

void modbus_serial_client::data_ready(struct bufferevent* bufev, int fd) {
	if (waiting_for_data())
		m_parser->read_data(bufev);
	else
		drain_buffer(bufev);
}

int modbus_serial_client::configure(TUnit* unit, size_t read, size_t send, serial_port_configuration &spc) {
	if (modbus_client::configure(unit, read, send))
		return 1;

	event_base_set(m_event_base, &m_next_query_timer);
	event_base_set(m_event_base, &m_query_deadine_timer);

	std::string protocol = unit->getAttribute<std::string>("extra:serial_protocol_variant", "");;
	if (protocol.empty() || protocol == "rtu")
		m_parser = new serial_rtu_parser(this, &m_log);
	else if (protocol == "ascii")
		m_parser = new serial_ascii_parser(this, &m_log);
	else {
		m_log.log(1, "Unsupported protocol variant: %s, unit not configured", protocol.c_str());
		return 1;
	}

	if (m_parser->configure(unit, spc))
		return 1;
	modbus_client::reset_cycle();
	return 0;
}

void modbus_serial_client::frame_parsed(SDU &sdu, struct bufferevent* bufev) {
	pdu_received(sdu.unit_id, sdu.pdu);
}

void modbus_serial_client::error(ERROR_CONDITION error) {
	//for now treat everything as timeout
	modbus_client::timeout();
}

struct event_base* modbus_serial_client::get_event_base() {
	return m_event_base;
}

struct bufferevent* modbus_serial_client::get_buffer_event() {
	return m_bufev;
}


void serial_rtu_parser::calculate_crc_table() {
	unsigned short crc, c;

	for (size_t i = 0; i < 256; i++) {
		crc = 0;
		c = (unsigned short) i;
		for (size_t j = 0; j < 8; j++) {
			if ((crc ^ c) & 0x0001)
				crc = (crc >> 1) ^ 0xa001;
			else
				crc = crc >> 1;
			c = c >> 1;
		}
		crc16[i] = crc;
	}

	crc_table_calculated = true;
}

unsigned short serial_rtu_parser::update_crc(unsigned short crc, unsigned char c) {
	if (crc_table_calculated == false)
		calculate_crc_table();
	unsigned short tmp;
	tmp = crc ^ c;
	crc = (crc >> 8) ^ crc16[tmp & 0xff];
	return crc;
}

bool serial_rtu_parser::check_crc() {
	if (m_data_read <= 2)
		return false;
	std::vector<unsigned char>& d = m_sdu.pdu.data;
	unsigned short crc = 0xffff;
	crc = update_crc(crc, m_sdu.unit_id);
	crc = update_crc(crc, m_sdu.pdu.func_code);

	for (size_t i = 0; i < m_data_read - 2; i++) 
		crc = update_crc(crc, m_sdu.pdu.data[i]);

	unsigned short frame_crc = d[m_data_read - 2] | (d[m_data_read - 1] << 8);

	m_log->log(8,"Unit ID = %hx",m_sdu.unit_id);
	m_log->log(8,"Func code = %hx",m_sdu.pdu.func_code);
	for (size_t i = 0; i < m_data_read ; i++) m_log->log(9, "Data[%zu] = %hx", i, d[i]);
	m_log->log(8, "Checking crc, result: %s, unit_id: %d, calculated crc: %hx, frame crc: %hx",
	       	(crc == frame_crc ? "OK" : "ERROR"), m_sdu.unit_id, crc, frame_crc);
	return crc == frame_crc;
}

serial_parser::serial_parser(serial_connection_handler *serial_handler, driver_logger *log) : m_serial_handler(serial_handler), m_read_timer_started(false), m_write_timer_started(false), m_log(log) {
	evtimer_set(&m_read_timer, read_timer_callback, this);
	event_base_set(m_serial_handler->get_event_base(), &m_read_timer);
}

void serial_parser::write_timer_event() {
	if (m_delay_between_chars && m_output_buffer.size()) {
		unsigned char c = m_output_buffer.front();
		m_output_buffer.pop_front();
		bufferevent_write(m_serial_handler->get_buffer_event(), &c, 1);
		if (m_output_buffer.size())
			start_write_timer();
		else
			stop_write_timer();
	}
	
}

void serial_parser::write_finished(struct bufferevent *bufev) {
	if (m_delay_between_chars && m_output_buffer.size())
		start_write_timer();
}

void serial_parser::stop_write_timer() {
	if (m_write_timer_started) {
		event_del(&m_write_timer);
		m_write_timer_started = false;
	}
}

void serial_parser::start_write_timer() {
	stop_write_timer();

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = m_delay_between_chars;
	evtimer_add(&m_write_timer, &tv);

	m_write_timer_started = true;
}

void serial_parser::stop_read_timer() {
	if (m_read_timer_started) {
		event_del(&m_read_timer);
		m_read_timer_started = false;
	}
}

void serial_parser::start_read_timer(long useconds) {
	stop_read_timer();

	struct timeval tv;
	tv.tv_sec = useconds / 1000000;
	tv.tv_usec = useconds % 1000000;
	evtimer_add(&m_read_timer, &tv); 
	m_read_timer_started = true;
}

void serial_parser::read_timer_callback(int fd, short event, void* parser) {
	((serial_parser*) parser)->read_timer_event();
}

void serial_parser::write_timer_callback(int fd, short event, void* parser) {
	((serial_parser*) parser)->write_timer_event();
}

serial_rtu_parser::serial_rtu_parser(serial_connection_handler *serial_handler, driver_logger* log) : serial_parser(serial_handler, log), m_state(FUNC_CODE) {
	reset();
}

int serial_rtu_parser::configure(TUnit* unit, serial_port_configuration &spc) {
	m_delay_between_chars = unit->getAttribute("extra:out-intra-character-delay", 0);
	if (m_delay_between_chars == 0)
		m_log->log(10, "Serial port configuration, delay between chars not given (or 0) assuming no delay");
	else
		m_log->log(10, "Serial port configuration, delay between chars set to %d miliseconds", m_delay_between_chars);

	int bits_per_char = spc.stop_bits
				+ spc.parity == serial_port_configuration::NONE ? 0 : 1
				+ int(spc.char_size);

	int chars_per_sec = spc.speed / bits_per_char;
	/*according to protocol specification, intra-character
	 * delay cannot exceed 1.5 * (time of transmittion of one character) */
	if (chars_per_sec)
		m_timeout_1_5_c = 1000000 / chars_per_sec * (15 / 10);
	else
		m_timeout_1_5_c = 100000;

	m_log->log(8, "Setting 1.5Tc timer to %d us",  m_timeout_1_5_c);

	m_timeout_3_5_c = unit->getAttribute("extra:read-timeout", 0);
	if (m_timeout_3_5_c == 0) {
		m_log->log(10, "Serial port configuration, read timeout not given (or 0), will use one based on speed");
		if (chars_per_sec)
			m_timeout_3_5_c = 1000000 / chars_per_sec * (35 / 10);
		else
			m_timeout_3_5_c = 100000;
	} else {
		m_log->log(10, "Serial port configuration, read timeout set to %d miliseconds", m_timeout_3_5_c);
		m_timeout_3_5_c *= 1000;
	}

	m_log->log(8, "Setting 3.5Tc timer to %d us",  m_timeout_3_5_c);

	if (m_delay_between_chars)
		evtimer_set(&m_write_timer, write_timer_callback, this);
	return 0;
}


void serial_rtu_parser::reset() {
	m_state = ADDR;	
	m_bufev = NULL;
	m_is_3_5_c_timeout = false;
	stop_write_timer();
	stop_read_timer();
	m_output_buffer.resize(0);
}

void serial_rtu_parser::read_data(struct bufferevent *bufev) {
	size_t r;

	m_log->log(8, "serial_rtu_parser::read_data: got data, resetting timers");
	stop_read_timer();
	m_is_3_5_c_timeout = false;

	m_bufev = bufev;

	switch (m_state) {
		case ADDR:
			if (bufferevent_read(bufev, &m_sdu.unit_id, sizeof(m_sdu.unit_id)) == 0) 
				break;
			m_log->log(8, "Parsing new frame, unit_id: %d", (int) m_sdu.unit_id);
			m_state = FUNC_CODE;

			m_data_read = 0;
			m_sdu.pdu.data.resize(max_frame_size - 2);
		case FUNC_CODE:
			if (bufferevent_read(bufev, &m_sdu.pdu.func_code, sizeof(m_sdu.pdu.func_code)) == 0) {
				start_read_timer(m_timeout_1_5_c);
				break;
			}
			m_log->log(8, "\tfunc code: %d", (int) m_sdu.pdu.func_code);
			m_state = DATA;
		case DATA:
			r = bufferevent_read(bufev, &m_sdu.pdu.data[m_data_read], m_sdu.pdu.data.size() - m_data_read);
			m_data_read += r;
			start_read_timer(m_timeout_1_5_c);
			break;
	}
}

void serial_rtu_parser::read_timer_event() {
	if (m_bufev == NULL)
		return;

	m_log->log(8, "serial_rtu_parser - read timer expired, didn't get any data for %fTc , checking crc",
		m_is_3_5_c_timeout ? 3.5 : 1.5);

	if (check_crc()) {
		m_is_3_5_c_timeout = false;

		m_state = ADDR;
		m_sdu.pdu.data.resize(m_data_read - 2);
		m_serial_handler->frame_parsed(m_sdu, m_bufev);
	} else {
		if (m_is_3_5_c_timeout) {
			m_log->log(8, "crc check failed, erroring out");
			m_serial_handler->error(serial_connection_handler::TIMEOUT);
			reset();
		} else  {
			m_log->log(8, "crc check failed, scheduling 3.5Tc timer");
			m_is_3_5_c_timeout = true;
			start_read_timer(m_timeout_3_5_c - m_timeout_1_5_c);
		}
	}
}

void serial_rtu_parser::send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev) {
	unsigned short crc = 0xffff;
	crc = update_crc(crc, unit_id);
	crc = update_crc(crc, pdu.func_code);
	for (size_t i = 0; i < pdu.data.size(); i++)
		crc = update_crc(crc, pdu.data[i]);

	unsigned char cl, cm;
	cl = crc & 0xff;
	cm = crc >> 8;

	if (m_delay_between_chars) {
		m_output_buffer.resize(0);
		m_output_buffer.push_back(pdu.func_code);
		m_output_buffer.insert(m_output_buffer.end(), pdu.data.begin(), pdu.data.end());
		m_output_buffer.push_back(cl);
		m_output_buffer.push_back(cm);

		bufferevent_write(bufev, &unit_id, sizeof(unit_id));
	} else {
		bufferevent_write(bufev, &unit_id, sizeof(unit_id));
		bufferevent_write(bufev, &pdu.func_code, sizeof(pdu.func_code));
		bufferevent_write(bufev, &pdu.data[0], pdu.data.size());
		bufferevent_write(bufev, &cl, 1);
		bufferevent_write(bufev, &cm, 1);
	}
}

unsigned char serial_ascii_parser::update_crc(unsigned char c, unsigned char crc) const {
	return crc + c;
}

unsigned char serial_ascii_parser::finish_crc(unsigned char crc) const {
	return 0x100 - crc; 
}

serial_ascii_parser::serial_ascii_parser(serial_connection_handler* serial_handler, driver_logger *log) : serial_parser(serial_handler, log), m_state(COLON) { }

bool serial_ascii_parser::check_crc() {
	unsigned char crc = 0;
	std::vector<unsigned char>& d = m_sdu.pdu.data;
	crc = update_crc(crc, m_sdu.unit_id);
	crc = update_crc(crc, m_sdu.pdu.func_code);
	if (d.size() == 0)
		return false;
	for (size_t i = 0; i < d.size() - 1; i++)
		crc = update_crc(crc, d[i]);
	crc = finish_crc(crc);
	unsigned char frame_crc = d[d.size() - 1];
	d.resize(d.size() - 1);
	return crc == frame_crc;
}

void serial_ascii_parser::read_data(struct bufferevent* bufev) {
	unsigned char c;
	stop_read_timer();
	while (bufferevent_read(bufev, &c, 1)) switch (m_state) {
		case COLON:
			if (c != ':') {
				m_log->log(1, "Modbus ascii frame did not start with the colon");
				m_serial_handler->error(serial_connection_handler::FRAME_ERROR);
				return;
			}
			m_state = ADDR_1; 
			break;
		case ADDR_1:
			m_previous_char = c;
			m_state = ADDR_2;
			break;
		case ADDR_2:
			if (ascii::from_ascii(m_previous_char, c, m_sdu.unit_id)) {
				m_log->log(1, "Invalid value received in request");
				m_serial_handler->error(serial_connection_handler::FRAME_ERROR);
				m_state = COLON;
				return;
			}
			m_log->log(9, "Parsing new frame, unit_id: %d", (int) m_sdu.unit_id);
			m_state = FUNC_CODE_1;
			break;
		case FUNC_CODE_1:
			m_previous_char = c;
			m_state = FUNC_CODE_2; 
			break;
		case FUNC_CODE_2:
			if (ascii::from_ascii(m_previous_char, c, m_sdu.pdu.func_code)) {
				m_log->log(1, "Invalid value received in request");
				m_serial_handler->error(serial_connection_handler::FRAME_ERROR);
				m_state = COLON;
				return;
			}
			m_sdu.pdu.data.resize(0);
			m_state = DATA_1;
			break;
		case DATA_1:
			if (c == '\r') {
				m_state = LF;	
			} else {
				m_previous_char = c;
				m_state = DATA_2;
			}
			break;
		case DATA_2:
			if (ascii::from_ascii(m_previous_char, c, c)) {
				m_log->log(1, "Invalid value received in request");
				m_serial_handler->error(serial_connection_handler::FRAME_ERROR);
				m_state = COLON;
				return;
			}
			m_sdu.pdu.data.push_back(c);
			m_state = DATA_1;
			break;
		case LF:
			if (c != '\n') {
				m_log->log(1, "Modbus ascii frame did not end with crlf");
				m_state = COLON;
				m_serial_handler->error(serial_connection_handler::FRAME_ERROR);
				return;
			}
			if (check_crc()) 
				m_serial_handler->frame_parsed(m_sdu, bufev);
			else 
				m_serial_handler->error(serial_connection_handler::CRC_ERROR);
			m_state = COLON;
			return;
	}
	start_read_timer(m_timeout);
}

void serial_ascii_parser::read_timer_event() {
	m_serial_handler->error(serial_connection_handler::TIMEOUT);
	reset();
}

void serial_ascii_parser::send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev) {
	unsigned char crc = 0;
	unsigned char c1, c2;

	bufferevent_write(bufev, const_cast<char*>(":"), 1);

#define SEND_VAL(v) { \
	ascii::to_ascii(v, c1, c2); \
	bufferevent_write(bufev, &c1, sizeof(c1)); \
	bufferevent_write(bufev, &c2, sizeof(c2)); }

	crc = update_crc(crc, unit_id);
	SEND_VAL(unit_id);

	crc = update_crc(crc, pdu.func_code);
	SEND_VAL(pdu.func_code);

	for (size_t i = 0; i < pdu.data.size(); i++) {
		crc = update_crc(crc, pdu.data[i]);
		SEND_VAL(pdu.data[i]);
	}

	crc = finish_crc(crc);
	SEND_VAL(crc);

	bufferevent_write(bufev, const_cast<char*>("\r"), 1);
	bufferevent_write(bufev, const_cast<char*>("\n"), 1);
}

int serial_ascii_parser::configure(TUnit* unit, serial_port_configuration &spc) {
	m_delay_between_chars = 0;
	m_timeout = unit->getAttribute("extra:read-timeout", 0);
	if (m_timeout == 0) {
		m_timeout = 1000000;
	} else {
		m_timeout *= 1000;
		m_log->log(10, "Serial port configuration, read timeout set to %d microseconds", m_timeout);
	}
	m_log->log(9, "serial_parser m_timeout: %d", m_timeout);
	return 0;
}

void serial_ascii_parser::reset() {
	m_state = COLON;
}

serial_server::serial_server() : modbus_unit(this) {}

int serial_server::configure(TUnit *unit, size_t read, size_t send, serial_port_configuration &spc) {
	if (modbus_unit::configure(unit, read, send))
		return 1;
	std::string protocol = unit->getAttribute<std::string>("extra:serial_protocol_variant", "");
	if (protocol.empty() || protocol == "rtu")
		m_parser = new serial_rtu_parser(this, &m_log);
	else if (protocol == "ascii")
		m_parser = new serial_ascii_parser(this, &m_log);
	else {
		m_log.log(1, "Unsupported protocol variant: %s, unit not configured", protocol.c_str());
		return 1;
	}
	if (m_parser->configure(unit, spc))
		return 1;
	return 0;
}

int serial_server::initialize() {
	return 0;
}

void serial_server::frame_parsed(SDU &sdu, struct bufferevent* bufev) {
	if (process_request(sdu.unit_id, sdu.pdu) == true)
		m_parser->send_sdu(sdu.unit_id, sdu.pdu, bufev);
}

void serial_server::connection_error(struct bufferevent* bufev) {
	m_bufev = NULL;
	m_parser->reset();
}

void serial_server::data_ready(struct bufferevent* bufev) {
	m_bufev = bufev;
	m_parser->read_data(bufev);	
}

void serial_server::error(serial_connection_handler::ERROR_CONDITION error) {
	m_parser->reset();
}

void serial_server::starting_new_cycle() {
	modbus_unit::starting_new_cycle();
}

void serial_server::finished_cycle() {
	modbus_unit::finished_cycle();
}

struct event_base* serial_server::get_event_base() {
	return m_event_base;
}

struct bufferevent* serial_server::get_buffer_event() {
	return m_bufev;
}

serial_client_driver* create_modbus_serial_client() {
	return new modbus_serial_client();
}

tcp_client_driver* create_modbus_tcp_client() {
	return new modbus_tcp_client();
}

serial_server_driver* create_modbus_serial_server() {
	return new serial_server();
}

tcp_server_driver* create_modbus_tcp_server() {
	return new tcp_server();
}
