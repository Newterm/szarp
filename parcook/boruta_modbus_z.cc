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

using szarp::ms;
using szarp::us;

template <typename T>
using bopt = boost::optional<T>;

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

struct ADU {
	bopt<unsigned> trans_id;
	unsigned char unit_id;
	unsigned char func_code;
	std::vector<unsigned char> data;
};

std::string adu2string(const struct ADU& adu) {
	std::stringstream ss;
	ss << "unit: " << (int)adu.unit_id;
	if (adu.trans_id)
		ss << " trans id: " << (int) (*adu.trans_id);

	ss << " fun:" << (int) adu.func_code;
	for (auto d: adu.data) {
		ss << " " << std::to_string(d);
	}

	return ss.str();
}

using register_iface = register_iface_t<int16_t>;
class null_register: public register_iface {
public:
	bool is_valid() const override { return false; }
	int16_t get_val() const override { return sz4::no_data<int16_t>(); }
	void set_val(int16_t val) override {}
	sz4::nanosecond_time_t get_mod_time() const override { return sz4::time_trait<sz4::nanosecond_time_t>::invalid_value; }
	void set_mod_time(const sz4::nanosecond_time_t&) override {}
};

std::shared_ptr<null_register> null_reg = std::make_shared<null_register>();

class register_holder {
public:
	virtual boost::optional<sz4::nanosecond_time_t> get_mod_time() const = 0;
	virtual void set_mod_time(const sz4::nanosecond_time_t&) const = 0;
	virtual std::shared_ptr<register_iface> operator[](size_t) const = 0;
};

template <int N_REGS>
class modbus_registers_holder: public register_holder {
	std::array<std::shared_ptr<register_iface>, N_REGS> m_regs;

public:
	modbus_registers_holder(std::array<std::shared_ptr<register_iface>, N_REGS> regs): m_regs(regs) {}

	boost::optional<sz4::nanosecond_time_t> get_mod_time() const override;
	void set_mod_time(const sz4::nanosecond_time_t& time) const override;
	std::shared_ptr<register_iface> operator[](size_t i) const override;
};

template <class _val_impl>
class read_val_op: public parcook_val_op {
protected:
	slog m_log;
	std::unique_ptr<register_holder> m_regs;

public:
	using val_impl = _val_impl;
	static constexpr uint32_t N_REGS = val_impl::N_REGS;
	using value_type = typename val_impl::value_type;

	read_val_op(register_holder* regs, value_type, int, slog log): m_log(log), m_regs(regs) {}

	void publish_val(zmqhandler& handler, size_t index) override {
		if (auto mod_time = m_regs->get_mod_time()) {
			auto val = val_impl::get_value_from_regs(*m_regs);
			szlog::log() << szlog::debug << m_log->header << ": setting read value " << val << szlog::endl;
			handler.set_value(index, *mod_time, val);
		}
	}
};

template <typename _val_impl>
class sent_val_op: public sender_val_op {
public:
	using val_impl = _val_impl;
	static constexpr uint32_t N_REGS = val_impl::N_REGS;
	using value_type = typename val_impl::value_type;

protected:
	slog m_log;
	std::unique_ptr<register_holder> m_regs;
	int m_prec;
	value_type m_nodata_value;

public:
	sent_val_op(register_holder* regs, value_type nodata_value, int prec, slog log): m_log(log), m_regs(regs), m_prec(prec), m_nodata_value(nodata_value) {}

	void update_val(zmqhandler& handler, size_t index) override {
		auto vt_pair = handler.get_send<value_type>(index, this->m_prec);
		if (!sz4::is_valid(vt_pair)) {
			m_log->log(10, "Sent value for index %zu was invalid", index);
			return; // do not overwrite, wait for data timeout instead
		}

		szlog::log() << szlog::debug << m_log->header << ": setting sent value " << vt_pair.value << " for index " << index << szlog::endl;
		m_regs->set_mod_time(vt_pair.time);
		val_impl::set_value_to_regs(*m_regs, vt_pair.value);
	}
};

template <typename _val_impl>
class sent_const_val_op: public sent_val_op<_val_impl> {
public:
	using val_impl = _val_impl;
	static constexpr uint32_t N_REGS = val_impl::N_REGS;
	using value_type = typename val_impl::value_type;

	value_type value;

public:
	sent_const_val_op(register_holder* regs, value_type _value, slog log): sent_val_op<val_impl>(regs, 0, sz4::no_data<value_type>(), log), value(_value) {}

	void update_val(zmqhandler& handler, size_t index) override {
		sz4::nanosecond_time_t t_now((const sz4::second_time_t) time(NULL));
		this->m_regs->set_mod_time(t_now);
		val_impl::set_value_to_regs(*(this->m_regs), value);
	}
};

template <>
class read_val_op<void>: public parcook_val_op {
public:
	using base_type = parcook_val_op;
};

template <>
class sent_val_op<void>: public sender_val_op {
public:
	using base_type = sender_val_op;
};

template <>
class sent_const_val_op<void>: public sent_val_op<void> {
public:
	using sent_val_op::base_type;
};

template <template <typename> class val_op>
struct base_type {
	using type = typename val_op<void>::base_type;
};


class RegsFactory {
	slog m_log;

	enum FLOAT_ORDER { LSWMSW, MSWLSW } default_float_order = MSWLSW;
	enum DOUBLE_ORDER { LSDMSD, MSDLSD } default_double_order = MSDLSD;

	FLOAT_ORDER get_float_order(TAttribHolder* param, FLOAT_ORDER default_value) const;
	DOUBLE_ORDER get_double_order(TAttribHolder* param, DOUBLE_ORDER default_value) const;

public:
	RegsFactory(slog log): m_log(log) {}
	void configure(UnitInfo* unit);

	template <uint32_t N_REGS>
	std::array<uint32_t, N_REGS> get_addresses(TAttribHolder* param, uint32_t addr) const;

};

class modbus_unit: public cycle_callback_handler {
protected:
	non_owning_ptr<zmqhandler> zmq;

	unsigned char m_id;

	using RMAP = std::map<unsigned short, std::shared_ptr<register_iface>>;
	RMAP m_registers;

	using RSET = std::set<std::pair<REGISTER_TYPE, unsigned short>>;
	RSET m_received;
	RSET m_sent;

	size_t m_read;
	size_t m_send;

	std::vector<std::unique_ptr<parcook_val_op>> m_parcook_ops;
	std::vector<std::pair<std::unique_ptr<sender_val_op>, size_t>> m_sender_ops;
	std::unique_ptr<RegsFactory> m_regs_factory;

	slog m_log;

	sz4::nanosecond_time_t m_current_time = szarp::time_now<sz4::nanosecond_time_t>();
	long long m_expiration_time;
	float m_nodata_value;

	std::shared_ptr<register_iface> register_for_addr(uint32_t addr, REGISTER_TYPE rt, parcook_val_op*);
	std::shared_ptr<register_iface> register_for_addr(uint32_t addr, REGISTER_TYPE rt, sender_val_op*);
	std::shared_ptr<register_iface> get_register(uint32_t addr);

	template <template <typename> class val_op>
	typename base_type<val_op>::type* configure_register(TAttribHolder* param, int prec_e);

	template <typename val_op>
	val_op* create_register(TAttribHolder* param, int prec, unsigned short addr, REGISTER_TYPE rt) {
		return create_register<val_op>(param, prec, addr, rt, (val_op*) nullptr);
	}

	template <typename val_op>
	val_op* create_register(TAttribHolder* param, int prec, unsigned short addr, REGISTER_TYPE rt, void*);

	template <typename val_op>
	val_op* create_register(TAttribHolder* param, int prec, unsigned short addr, REGISTER_TYPE rt, sent_const_val_op<typename val_op::val_impl>*);

	void configure_param(IPCParamInfo* param);
	void configure_send(SendParamInfo* param);
	void configure_unit(UnitInfo* u);

	const char* error_string(const unsigned char& error);

public:
	bool process_request(ADU &adu);

protected:
	void consume_read_regs_response(unsigned short start_addr, unsigned short regs_count, ADU &adu);
	void consume_write_regs_response(unsigned short start_addr, unsigned short regs_count, ADU &adu);

	bool perform_write_registers(ADU &adu);
	bool perform_read_holding_regs(ADU &adu);

	bool create_error_response(unsigned char error, ADU &adu);

public:
	modbus_unit(zmqhandler* _zmq, slog logger);

	bool register_val_expired(const sz4::nanosecond_time_t& time) const;

	virtual int configure(UnitInfo *unit, size_t read, size_t send);

	void do_ipc() {
		to_parcook();
		from_sender();
	}

	void start_cycle() override {}

	void to_parcook();
	void from_sender();

	virtual void adu_received(BaseConnection* conn, ADU &adu) = 0;
	virtual void reset_cycle() = 0;
	virtual void timeout() = 0;
};

enum ERROR_CONDITION { TIMEOUT, FRAME_ERROR, CRC_ERROR };
std::string error_condition_str(const ERROR_CONDITION e) {
	const std::map<ERROR_CONDITION, std::string> error_str_map = { {TIMEOUT, "timeout"}, {FRAME_ERROR, "frame error"}, {CRC_ERROR,"crc error"} };
	return error_str_map.at(e);
}

class bc_handler {
public:
	virtual void frame_parsed(ADU& adu, BaseConnection*) = 0;
	virtual void send_frame(BaseConnection* conn, ADU& adu) = 0;

	virtual void error(ERROR_CONDITION) = 0;
};

class modbus_server: public modbus_unit {
	non_owning_ptr<bc_handler> handler;

public:
	modbus_server(bc_handler* _handler, boruta_daemon* boruta, slog log): modbus_unit(boruta->get_zmq(), log), handler(_handler) {}

	void adu_received(BaseConnection* conn, ADU &adu) override {
		if (modbus_unit::process_request(adu)) {
			handler->send_frame(conn, adu);
		}
	}

	void reset_cycle() override {};
	void timeout() override {}
};

class modbus_client: public modbus_unit {
	non_owning_ptr<bc_handler> handler;
	non_owning_ptr<BaseConnection> m_connection;
	
protected:
	RSET::iterator m_received_iterator;
	RSET::iterator m_sent_iterator;

	ADU m_du;

	unsigned short m_start_addr;
	unsigned short m_regs_count;
	REGISTER_TYPE m_register_type;

	enum {
		READING_FROM_PEER,
		WRITING_TO_PEER,
		IDLE,
	} m_state;

	sz4::nanosecond_time_t m_latest_request_sent;
	Scheduler next_query_timer;
	Scheduler query_deadline_timer;
	struct event m_query_deadine_timer;
	bool m_single_register_pdu;

public:
	modbus_client(BaseConnection* conn, bc_handler* handler, boruta_daemon* boruta, slog log);
	int configure(UnitInfo *unit, size_t read, size_t send) override;

	void reset_cycle() override;
	void send_next_query();
	void send_write_query();
	void send_read_query();
	void next_query();
	void send_query();
	void find_continuous_reg_block(RSET::iterator &i, RSET &regs);

	void timeout() override;

	void adu_received(BaseConnection* conn, ADU &adu);

	// cycle_callback_handler impl
	void start_cycle() override;
};

class bc_parser {
public:
	virtual int configure(UnitInfo* unit, const SerialPortConfiguration &) = 0;
	virtual void send_adu(BaseConnection* conn, ADU& adu) = 0;
	virtual void read_data(const BaseConnection* conn, const std::vector<unsigned char>& data) = 0;
	virtual void write_finished() = 0;
	virtual void reset() = 0;
};

class bc_modbus_driver: public bc_driver, public bc_handler, public cycle_callback_handler {
protected:
	CycleScheduler cycle_scheduler;
	Scheduler reconnection_timer;
	std::unique_ptr<modbus_unit> m_unit;
	std::unique_ptr<bc_parser> m_parser;
	slog m_log;

private:
	std::map<std::string, std::function<modbus_unit*()>> mode_map;
	std::map<std::string, std::function<bc_parser*()>> proto_map;

public:
	bc_modbus_driver(BaseConnection* conn, boruta_daemon* boruta, slog log);

	void start_cycle() override;

	int configure(UnitInfo* unit, size_t read, size_t send, const SerialPortConfiguration &conf) override;
	int configure_unit(UnitInfo* unit, size_t read, size_t send);
	int configure_parser(UnitInfo* unit, const SerialPortConfiguration &conf);

	// ConnectionListener impl
	void ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) override;
	void ReadError(const BaseConnection *conn, short int event) override;
	void OpenFinished(const BaseConnection *conn) override;
	void SetConfigurationFinished(const BaseConnection *conn) override;

	void frame_parsed(ADU &adu, BaseConnection* conn) override;
	void send_frame(BaseConnection* conn, ADU &adu) override;
	void error(const ERROR_CONDITION e) override;
};

class bc_modbus_tcp_client: public bc_modbus_driver {
	unsigned char m_trans_id;
	std::set<unsigned char> valid_ids;

public:
	using bc_modbus_driver::bc_modbus_driver;

	// bc_handler impl
	void send_frame(BaseConnection* conn, ADU &adu) override;

	// bc_onnection_handler impl
	void frame_parsed(ADU &adu, BaseConnection* conn) override;

	void start_cycle() override;
};

class bc_serial_parser: public bc_parser {
protected:
	slog m_log;
	non_owning_ptr<BaseConnection> m_connection;
	std::unique_ptr<bc_handler> m_serial_handler;

	ADU m_adu;

	std::deque<unsigned char> m_input_buffer;
	std::deque<unsigned char> m_output_buffer;

	int m_delay_between_chars;

	Scheduler read_timer;
	Scheduler write_timer;

	virtual void read_timer_event() = 0;
	virtual void write_timer_event();

public:
	bc_serial_parser(BaseConnection* conn, bc_handler *serial_handler, slog log);
	void write_finished() override;

};

class bc_tcp_parser: public bc_parser {
	non_owning_ptr<bc_handler> handler;
	slog m_log;

public:
	bc_tcp_parser(bc_handler* _handler, slog log): handler(_handler), m_log(log) {}
	int configure(UnitInfo* unit, const SerialPortConfiguration &) override;

	void send_adu(BaseConnection* conn, ADU &adu) override;
	void read_data(const BaseConnection* conn, const std::vector<unsigned char>& data) override;

	void write_finished() override {}
	void reset() override {}
};

class bc_serial_rtu_parser : public bc_serial_parser {
	enum { ADDR, FUNC_CODE, DATA } m_state;
	size_t m_data_read;

	us m_timeout_1_5_c = 100000;
	us m_timeout_3_5_c = 100000;
	bool m_is_3_5_c_timeout;

	std::vector<unsigned char> m_data;

	bool check_crc();

	static void calculate_crc_table();
	static unsigned short crc16[256];
	static bool crc_table_calculated;

	unsigned short update_crc(unsigned short crc, unsigned char c);

	void read_timer_event() override;

public:
	bc_serial_rtu_parser(BaseConnection* conn, bc_handler *serial_handler, slog log): bc_serial_parser(conn, serial_handler, log), m_state(FUNC_CODE) {
		reset();
	}

	void read_data(const BaseConnection* conn, const std::vector<unsigned char>& data) override;
	void send_adu(BaseConnection* conn, ADU &sdu) override;
	void reset() override;
	int configure(UnitInfo* unit, const SerialPortConfiguration &) override;

	static const int max_frame_size = 256;
};

class bc_serial_ascii_parser : public bc_serial_parser {
	unsigned char m_previous_char;
	enum { COLON, ADDR_1, ADDR_2, FUNC_CODE_1, FUNC_CODE_2, DATA_1, DATA_2, LF } m_state;

	void reset();

	bool check_crc();

	unsigned char update_crc(unsigned char c, unsigned char crc) const;
	unsigned char finish_crc(unsigned char c) const;

	void read_timer_event() override;
public:
	bc_serial_ascii_parser(BaseConnection* conn, bc_handler *serial_handler, slog log);
	void read_data(const BaseConnection* conn, const std::vector<unsigned char>& data) override;
	void send_adu(BaseConnection* conn, ADU &adu) override;
	int configure(UnitInfo* unit, const SerialPortConfiguration&) override;
};

unsigned short bc_serial_rtu_parser::crc16[256] = {};
bool bc_serial_rtu_parser::crc_table_calculated = false;

class modbus_register: public register_iface {
	non_owning_ptr<modbus_unit> m_modbus_unit;
	slog m_log;

	// allow accessing data members
	int16_t m_val = sz4::no_data<int16_t>();
	sz4::nanosecond_time_t m_mod_time = sz4::time_trait<sz4::nanosecond_time_t>::invalid_value;

public:
	modbus_register(modbus_unit* unit, slog log): m_modbus_unit(unit), m_log(log), m_val(sz4::no_data<short>()), m_mod_time(sz4::time_trait<sz4::nanosecond_time_t>::invalid_value) {}

	bool is_valid() const override {
		return sz4::time_trait<sz4::nanosecond_time_t>::is_valid(m_mod_time) && !m_modbus_unit->register_val_expired(m_mod_time);
	}

	void set_val(int16_t val) override { m_val = val; }
	int16_t get_val() const override { return m_val; }

	sz4::nanosecond_time_t get_mod_time() const override { return m_mod_time; }
	void set_mod_time(const sz4::nanosecond_time_t& time) override { m_mod_time = time; }
};

class const_val_register: public register_iface {
	int16_t m_val = sz4::no_data<int16_t>();

public:
	bool is_valid() const override { return true; }

	void set_val(int16_t val) override { m_val = val; }
	int16_t get_val() const override { return m_val; }

	sz4::nanosecond_time_t get_mod_time() const override { return sz4::nanosecond_time_t((sz4::second_time_t)time(NULL)); }

	void set_mod_time(const sz4::nanosecond_time_t& time) override {}
};

bool modbus_unit::process_request(ADU &adu) {
	if (m_id != adu.unit_id && adu.unit_id != 0) {
		m_log->log(1, "Received request to unit %d, not such unit in configuration, ignoring", (int) adu.unit_id);
		return false;
	}

	try {

		switch (adu.func_code) {
			case MB_F_RHR:
				return perform_read_holding_regs(adu);
			case MB_F_WSR:
			case MB_F_WMR:
				return perform_write_registers(adu);
			default:
				return create_error_response(MB_ILLEGAL_FUNCTION, adu);
		}

	} catch (std::out_of_range) {	//message was distorted
		m_log->log(1, "Error while processing request - there is either a bug or sth was very wrong with request format");
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

void modbus_unit::consume_read_regs_response(unsigned short start_addr, unsigned short regs_count, ADU &adu) {
	m_log->log(5, "Consuming read holding register response unit_id: %d, address: %hu, registers count: %hu", (int) m_id, start_addr, regs_count);
	if (adu.func_code & MB_ERROR_CODE) {
		m_log->log(1, "Exception received in response to read holding registers command, unit_id: %d, address: %hu, count: %hu",
		       	(int)m_id, start_addr, regs_count);
		m_log->log(1, "Error is: %s(%d)", error_string(adu.data.at(0)), (int)adu.data.at(0));
		return;
	} 

	if (adu.data.size() - 1 != regs_count * 2) {
		m_log->log(1, "Unexpected data size in response to read holding registers command, requested %hu regs but got %zu data in response",
		       	regs_count, adu.data.size() - 1);
		throw std::out_of_range("Invalid response size");
	}

	size_t data_index = 1;

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++, data_index += 2) {
		auto j = m_registers.find(addr);
		uint16_t v = ((uint16_t)(adu.data.at(data_index)) << 8) | adu.data.at(data_index + 1);
		m_log->log(9, "Setting register unit_id: %d, address: %hu, value: %hu", (int) m_id, (int) addr, v);
		j->second->set_val(v);
		j->second->set_mod_time(m_current_time);
	}
}

void modbus_unit::consume_write_regs_response(unsigned short start_addr, unsigned short regs_count, ADU &adu) { 
	m_log->log(5, "Consuming write holding register response unit_id: %u, address: %hu, registers count: %hu", m_id, start_addr, regs_count);
	if (adu.func_code & MB_ERROR_CODE) {
		m_log->log(1, "Exception received in response to write holding registers command, unit_id: %u, address: %hu, count: %hu",
			m_id, start_addr, regs_count);
		m_log->log(1, "Error is: %s(%d)", error_string(adu.data.at(0)), (int)adu.data.at(0));
	} else {
		m_log->log(5, "Write holding register command poisitively acknowledged");
	}
}

bool modbus_unit::perform_write_registers(ADU &adu) {
	std::vector<unsigned char>& d = adu.data;

	unsigned short start_addr;
	unsigned short regs_count;
	size_t data_index;


	start_addr = (d.at(0) << 8) | d.at(1);

	if (adu.func_code == MB_F_WMR) {
		regs_count = (d.at(2) << 8) | d.at(3);
		data_index = 5;
	} else {
		regs_count = 1;
		data_index = 2;
	}

	m_log->log(7, "Processing write holding request registers start_addr: %hu, regs_count:%hu", start_addr, regs_count);

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++, data_index += 2) {
		auto j = m_registers.find(addr);
		if (j == m_registers.end())
			return create_error_response(MB_ILLEGAL_DATA_ADDRESS, adu);

		uint16_t v = (d.at(data_index) << 8) | d.at(data_index + 1);

		m_log->log(9, "Setting register: %hu value: %hu", (int) addr, v);
		j->second->set_val(v);	
		j->second->set_mod_time(m_current_time);
	}

	if (adu.func_code == MB_F_WMR)
		d.resize(4);

	m_log->log(7, "Request processed successfully");

	return true;
}

bool modbus_unit::perform_read_holding_regs(ADU &adu) {
	std::vector<unsigned char>& d = adu.data;
	uint16_t start_addr = (d.at(0) << 8) | d.at(1);
	uint16_t regs_count = (d.at(2) << 8) | d.at(3);

	m_log->log(7, "Responding to read holding registers request start_addr: %hu, regs_count:%hu", start_addr, regs_count);

	d.resize(1);
	d.at(0) = 2 * regs_count;

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++) {
		auto j = m_registers.find(addr);
		if (j == m_registers.end())
			return create_error_response(MB_ILLEGAL_DATA_ADDRESS, adu);

		uint16_t v = j->second->get_val();
		d.push_back(v >> 8);
		d.push_back(v & 0xff);
		m_log->log(9, "Sending register: %hu, value: %hu", (int) addr, v);
	}

	m_log->log(7, "Request processed sucessfully.");

	return true;
}

bool modbus_unit::create_error_response(unsigned char error, ADU &adu) {
	adu.func_code |= MB_ERROR_CODE;
	adu.data.resize(1);
	adu.data[0] = error;

	m_log->log(7, "Sending error response: %s", error_string(error));

	return true;
}

void modbus_unit::to_parcook() {
	size_t m = 0;
	for (auto& reg: m_parcook_ops)
		reg->publish_val(*zmq, m_read + m++);
}

void modbus_unit::from_sender() {
	m_current_time = szarp::time_now<sz4::nanosecond_time_t>();

	for (auto& sent: m_sender_ops)
		sent.first->update_val(*zmq, sent.second);
}

bool modbus_unit::register_val_expired(const sz4::nanosecond_time_t& time) const {
	if (m_expiration_time == 0)
		return false;
	else {
		auto dt = time;
		dt += m_expiration_time;
	
		return m_current_time >= dt;
	}
}

modbus_unit::modbus_unit(zmqhandler* _zmq, slog log) : zmq(_zmq), m_log(log) {
	// in c++14 change this to make_uniqe
	m_regs_factory = std::unique_ptr<RegsFactory>(new RegsFactory(log));
}

std::string get_param_name(const TAttribHolder* param) {
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

void RegsFactory::configure(UnitInfo* unit) {
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
std::array<uint32_t, N_REGS> RegsFactory::get_addresses(TAttribHolder* param, uint32_t addr) const {
	std::array<uint32_t, N_REGS> regs;
	for (uint16_t i = 0; i < N_REGS; ++i) {
		regs[i] = addr + i;
	}
	return regs;
}

template <>
std::array<uint32_t, 2> RegsFactory::get_addresses<2>(TAttribHolder* param, uint32_t addr) const {
	FLOAT_ORDER float_order = get_float_order(param, default_float_order);

	if (float_order == MSWLSW) {
		return {addr + 1u, addr};
	} else {
		return {addr, addr + 1u};
	}
}

template <>
std::array<uint32_t, 4> RegsFactory::get_addresses<4>(TAttribHolder* param, uint32_t addr) const {
	DOUBLE_ORDER double_order = get_double_order(param, default_double_order);
	auto lsa = get_addresses<2>(param, addr);
	auto msa = get_addresses<2>(param, addr+2);

	if (double_order == MSDLSD) {
		return {msa[0], msa[1], lsa[0], lsa[1]};
	} else {
		return {lsa[0], lsa[1], msa[0], msa[1]};
	}
}

std::shared_ptr<register_iface> modbus_unit::register_for_addr(uint32_t addr, REGISTER_TYPE rt, parcook_val_op*) {
	m_received.insert(std::make_pair(rt, addr));
	return get_register(addr);
}

std::shared_ptr<register_iface> modbus_unit::register_for_addr(uint32_t addr, REGISTER_TYPE rt, sender_val_op*) {
	m_sent.insert(std::make_pair(rt, addr));
	return get_register(addr);
}

std::shared_ptr<register_iface> modbus_unit::get_register(uint32_t addr) {
	if (m_registers.find(addr) == m_registers.end()) {
		m_registers[addr] = std::make_shared<modbus_register>(this, m_log);
	}
	return m_registers[addr];
}

template <int N_REGS>
boost::optional<sz4::nanosecond_time_t> modbus_registers_holder<N_REGS>::get_mod_time() const {
	sz4::nanosecond_time_t mod_time;
	for (uint32_t i = 0; i < N_REGS; ++i) {
		if (!m_regs[i]->is_valid())
			return boost::none;

		mod_time = std::max(mod_time, this->m_regs[i]->get_mod_time());
	}

	return mod_time;
}

template <int N_REGS>
void modbus_registers_holder<N_REGS>::set_mod_time(const sz4::nanosecond_time_t& time) const {
	for (auto reg: m_regs) {
		reg->set_mod_time(time);
	}
}

template <int N_REGS>
std::shared_ptr<register_iface> modbus_registers_holder<N_REGS>::operator[](size_t i) const {
	if (i >= N_REGS) return null_reg;
	return m_regs[i];
}

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

uint16_t get_address(TAttribHolder* param) {
	if (bopt<long> addr_opt = param->getOptAttribute<long>("extra:address")) {
		if (*addr_opt < 0 || *addr_opt > 65535) {
			throw std::runtime_error("Invalid address attribute value, should be between 0 and 65535");
		}
		return *addr_opt;
	} else {
		throw std::runtime_error("No address attribute value!");
	}
}

REGISTER_TYPE get_rt(TAttribHolder* param) {
	auto reg_type_attr = param->getAttribute<std::string>("extra:register_type", "holding_register");
	if (reg_type_attr == "holding_register") {
		return HOLDING_REGISTER;
	} else if (reg_type_attr == "input_register") {
		return INPUT_REGISTER;
	} else {
		throw std::runtime_error("Unsupported register type, should be either input_register or holding_register");
	}
}

void modbus_unit::configure_param(IPCParamInfo* param) { 
	if (auto nreg = configure_register<read_val_op>(param, param->GetPrec())) {
		m_parcook_ops.push_back(std::unique_ptr<parcook_val_op>(nreg));
	} else {
		throw std::runtime_error("Couldn't configure param!");
	}
}

void modbus_unit::configure_send(SendParamInfo* param) {
	if (auto val = param->getOptAttribute<std::string>("value")) {
		configure_register<sent_const_val_op>(param, param->GetPrec());
	} else if (auto nreg = configure_register<sent_val_op>(param, param->GetPrec())) {
		m_sender_ops.push_back({std::unique_ptr<sender_val_op>(nreg), m_send++});
	} else {
		throw std::runtime_error("Couldn't configure send param!");
	}
}

template <template <typename> class val_op>
typename base_type<val_op>::type* modbus_unit::configure_register(TAttribHolder* param, int prec_e) {
	uint16_t addr = get_address(param);
	REGISTER_TYPE rt = get_rt(param);

	bopt<std::string> val_type = param->getAttribute<std::string>("extra:val_type");
	if (!val_type) {
		throw std::runtime_error(std::string("Couldn't configure register for ") + get_param_name(param));
	}
	
	int prec = exp10(prec_e);
	const std::map<std::string, std::function<typename val_op<void>::base_type*()>> type_fun_map = {
		{ "integer", [=]() { return create_register<val_op<short_modbus_op>>(param, prec, addr, rt); }},
		{ "bcd", [=]() { return create_register<val_op<bcd_modbus_op>>(param, prec, addr, rt); }},
		{ "long", [=]() { return create_register<val_op<long_modbus_op>>(param, prec, addr, rt); }},
		{ "float", [=]() { return create_register<val_op<float_modbus_op>>(param, prec, addr, rt); }},
		{ "double", [=]() { return create_register<val_op<double_modbus_op>>(param, prec, addr, rt); }},
		{ "decimal3", [=]() { return create_register<val_op<decimal3_modbus_op>>(param, prec, addr, rt); }},
	};

	auto fun_it = type_fun_map.find(*val_type);
	if (fun_it == type_fun_map.end())
		throw std::runtime_error(std::string("Invalid type ") + *val_type + std::string(" specified in param ") + get_param_name(param));
	
	return (fun_it->second)();
}

template <typename val_op>
val_op* modbus_unit::create_register(TAttribHolder* param, int prec, unsigned short addr, REGISTER_TYPE rt, void*) {
	std::array<uint32_t, val_op::N_REGS> r_addrs = m_regs_factory->get_addresses<val_op::N_REGS>(param, addr);
	std::array<std::shared_ptr<register_iface>, val_op::N_REGS> registers;

	int32_t i = 0;
	for (auto r_addr: r_addrs) {
		registers[i++] = register_for_addr(r_addr, rt, (val_op*) nullptr); // tag dispatch
	}
	
	m_log->log(8, "Param %s mapped to unit: %u, register %hu", get_param_name(param).c_str(), m_id, addr);
	return new val_op(new modbus_registers_holder<val_op::N_REGS>(registers), m_nodata_value, prec, m_log);
}

template <typename val_op>
val_op* modbus_unit::create_register(TAttribHolder* param, int prec, unsigned short addr, REGISTER_TYPE rt, sent_const_val_op<typename val_op::val_impl>*) {

	if (auto v = param->getOptAttribute<typename val_op::value_type>("value")) {
		std::array<uint32_t, val_op::N_REGS> r_addrs = m_regs_factory->get_addresses<val_op::N_REGS>(param, addr);
		std::array<std::shared_ptr<register_iface>, val_op::N_REGS> regs;
		int i = 0;

		for (auto addr: r_addrs) {
			m_sent.insert(std::make_pair(rt, addr));
			if (m_registers.find(addr) == m_registers.end()) {
				regs[i++] = m_registers[addr] = std::make_shared<const_val_register>();
			} else {
				throw std::runtime_error("Register for given address already exists");
			}
		}

		modbus_registers_holder<val_op::N_REGS> regs_holder(regs);
		val_op::val_impl::set_value_to_regs(regs_holder, *v);
		m_log->log(8, "Configured sent value, mapped to register %hu", addr);
	} else {
		throw std::runtime_error("Couldn't configure sent value as no value was given");
	}

	return nullptr;
}


void modbus_unit::configure_unit(UnitInfo* u) {
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
		configure_param(p);
	}

	for (auto p: u->GetSendParams()) {
		configure_send(p);
	}
}

int modbus_unit::configure(UnitInfo *unit, size_t read, size_t send) {
	m_regs_factory->configure(unit);

	m_expiration_time = unit->getAttribute("extra:nodata-timeout", 60);
	m_expiration_time *= 1000000000;

	m_nodata_value = unit->getAttribute("extra:nodata-value", sz4::no_data<float>());
	m_log->log(9, "No data value set to: %f", m_nodata_value);

	m_read = read;
	m_send = send;

	try {
		configure_unit(unit);
		return 0;
	} catch (const std::exception& e) {
		m_log->log(1, "Encountered error while configuring unit: %s", e.what());
	}

	return 1;
}

modbus_client::modbus_client(BaseConnection* conn, bc_handler* _handler, boruta_daemon* boruta, slog log) : modbus_unit(boruta->get_zmq(), log), handler(_handler), m_connection(conn), m_state(IDLE), next_query_timer(), query_deadline_timer() {
	next_query_timer.set_callback(new FnPtrScheduler([this](){ send_query();}));
	query_deadline_timer.set_callback(new FnPtrScheduler([this](){ timeout() ;}));
}

void modbus_client::reset_cycle() {
	m_state = IDLE;

	query_deadline_timer.cancel();
	next_query_timer.cancel();

	m_received_iterator = m_received.begin();
	m_sent_iterator = m_sent.begin();
}

void modbus_client::start_cycle() {
	if (m_state != IDLE) {
		m_log->log(1, "Did not finish polling on time, dropping next cycle!");
		return;
	}

	m_state = READING_FROM_PEER;
	send_next_query();
}

void modbus_client::send_next_query() {
	switch (m_state) {
		case IDLE:
			break;
		case READING_FROM_PEER:
		case WRITING_TO_PEER:
			next_query();
			break;
		default:
			ASSERT(false);
			break;
	}
}

void modbus_client::send_write_query() {
	m_du.func_code = MB_F_WMR;	
	m_du.data.resize(0);
	m_du.data.push_back(m_start_addr >> 8);
	m_du.data.push_back(m_start_addr & 0xff);
	m_du.data.push_back(m_regs_count >> 8);
	m_du.data.push_back(m_regs_count & 0xff);
	m_du.data.push_back(m_regs_count * 2);

	m_log->log(7, "Sending write multiple registers command, start register: %hu, registers count: %hu", m_start_addr, m_regs_count);
	for (size_t i = 0; i < m_regs_count; i++) {
		uint16_t v = m_registers[m_start_addr + i]->get_val();
		m_du.data.push_back(v >> 8);
		m_du.data.push_back(v & 0xff);
		m_log->log(9, "Sending register: %zu, value: %hu:", m_start_addr + i, v);
	}

	m_du.unit_id = m_id;
	handler->send_frame(m_connection, m_du);
}

void modbus_client::send_read_query() {
	m_log->log(7, "Sending read holding registers command, unit_id: %u,  address: %hu, count: %hu", m_id, m_start_addr, m_regs_count);
	switch (m_register_type) {
		case INPUT_REGISTER:
			m_du.func_code = MB_F_RIR;
			break;
		case HOLDING_REGISTER:
			m_du.func_code = MB_F_RHR;
			break;
	}
	m_du.data.resize(0);
	m_du.data.push_back(m_start_addr >> 8);
	m_du.data.push_back(m_start_addr & 0xff);
	m_du.data.push_back(m_regs_count >> 8);
	m_du.data.push_back(m_regs_count & 0xff);

	m_du.unit_id = m_id;
	handler->send_frame(m_connection, m_du);
}

void modbus_client::next_query() {
	switch (m_state) {
		case IDLE:
			m_state = READING_FROM_PEER;
			//[fallthrough]
		case READING_FROM_PEER:
			if (m_received_iterator != m_received.end()) {
				find_continuous_reg_block(m_received_iterator, m_received);
				next_query_timer.schedule();
				query_deadline_timer.reschedule();
				break;
			} 
			m_state = WRITING_TO_PEER;
		case WRITING_TO_PEER:
			if (m_sent_iterator != m_sent.end()) {
				find_continuous_reg_block(m_sent_iterator, m_sent);
				next_query_timer.schedule();
				query_deadline_timer.reschedule();
				break;
			}
			m_state = IDLE;
			reset_cycle();
			break;
	}
}

void modbus_client::send_query() {
	m_log->log(10, "send_query");
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

	query_deadline_timer.reschedule();
}

void modbus_client::timeout() {
	query_deadline_timer.cancel();

	switch (m_state) {
		case READING_FROM_PEER:
			m_log->log(1, "Timeout while reading data, unit_id: %d, address: %hu, registers count: %hu, progressing with queries",
					(int)m_id, m_start_addr, m_regs_count);

			send_next_query();
			break;
		case WRITING_TO_PEER:
			m_log->log(1, "Timeout while writing data, unit_id: %d, address: %hu, registers count: %hu, progressing with queries",
					(int)m_id, m_start_addr, m_regs_count);
			send_next_query();
			break;
		default:
			m_log->log(1, "Received unexpected timeout from connection, ignoring.");
			break;
	}
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

int modbus_client::configure(UnitInfo *unit, size_t read, size_t send) {
	m_single_register_pdu = unit->getAttribute("extra:single-register-pdu", false);

	auto query_interval = unit->getAttribute("extra:query-interval", 1);
	next_query_timer.set_timeout(ms(query_interval));

	auto request_timeout = unit->getAttribute("extra:request-timeout", 200);
	query_deadline_timer.set_timeout(ms(request_timeout));

	reset_cycle();
	return modbus_unit::configure(unit, read, send);
}

void modbus_client::adu_received(BaseConnection* conn, ADU &adu) {
	query_deadline_timer.cancel();
	if (adu.unit_id != m_id && adu.unit_id != 0) {
		m_log->log(1, "Received PDU from unit %d while we wait for response from unit %d, ignoring it!", (int) adu.unit_id, (int)m_id);
		return;
	}

	switch (m_state) {
		case READING_FROM_PEER:
			try {
				consume_read_regs_response(m_start_addr, m_regs_count, adu);
			} catch (std::out_of_range&) {	//message was distorted
			}

			send_next_query();
			break;
		case WRITING_TO_PEER:
			try {
				consume_write_regs_response(m_start_addr, m_regs_count, adu);
			} catch (std::out_of_range&) {	//message was distorted
			}

			send_next_query();
			break;
		default:
			m_log->log(10, "Received unexpected response from slave - ignoring it.");
			break;
	}
}

bc_modbus_driver::bc_modbus_driver(BaseConnection* conn, boruta_daemon* boruta, slog log): bc_driver(conn), cycle_scheduler(this), m_log(log) {
	mode_map["client"] = std::function<modbus_unit*()>([=](){ return new modbus_client(conn, this, boruta, m_log); });
	mode_map["server"] = std::function<modbus_unit*()>([=](){ return new modbus_server(this, boruta, m_log); });

	proto_map["tcp"] = std::function<bc_parser*()>([=](){ return new bc_tcp_parser(this, m_log); });
	proto_map["rtu"] = std::function<bc_parser*()>([=](){ return new bc_serial_rtu_parser(conn, this, m_log); });
	proto_map["ascii"] = std::function<bc_parser*()>([=](){ return new bc_serial_ascii_parser(conn, this, m_log); });
}

void bc_modbus_driver::start_cycle() {
	m_log->log(7, "Starting cycle");
	m_unit->do_ipc();

	reconnection_timer.cancel();

	if (!m_connection->Ready()) {
		m_connection->Close();
		m_connection->Open();
	} else {
		m_unit->start_cycle();
	}
}

int bc_modbus_driver::configure(UnitInfo* unit, size_t read, size_t send, const SerialPortConfiguration &conf) {
	configure_unit(unit, read, send);
	configure_parser(unit, conf);

	cycle_scheduler.configure(unit);

	reconnection_timer.set_timeout(ms(4000));
	reconnection_timer.set_callback(new FnPtrScheduler([this](){
		if (!m_connection->Ready()) {
			m_connection->Close();
			m_connection->Open();
		}
	}));
	return 0;
}

int bc_modbus_driver::configure_unit(UnitInfo* unit, size_t read, size_t send) {
	auto mode = unit->getAttribute<std::string>("extra:mode", "client");
	auto fun_it = mode_map.find(mode);
	if (fun_it == mode_map.end()) {
		m_log->log(1, "Unsupported mode: %s, unit not configured", mode.c_str());
		return 1;
	}

	m_unit = std::unique_ptr<modbus_unit>((fun_it->second)());

	if (m_unit->configure(unit, read, send)) {
		m_log->log(1, "Couldn't configure parser!");
		return 1;
	}

	return 0;
}

int bc_modbus_driver::configure_parser(UnitInfo* unit, const SerialPortConfiguration &conf) {
	std::string proto = "";
	auto medium = unit->getAttribute<std::string>("extra:medium", "tcp");
	if (medium == "tcp") {
		if (unit->getAttribute<bool>("extra:use_tcp_2_serial_proxy", false)) {
			proto = "rtu";
		} else {
			proto = "tcp";
		}
	} else if (medium == "serial") {
		proto = unit->getAttribute<std::string>("extra:serial_protocol_variant", "rtu");
	} else {
		m_log->log(1, "Unsupported medium: %s, unit not configured", medium.c_str());
		return 1;
	}

	auto fun_it = proto_map.find(proto);
	if (fun_it == proto_map.end()) {
		m_log->log(1, "Unsupported protocol variant: %s, unit not configured", proto.c_str());
		return 1;
	}

	m_parser = std::unique_ptr<bc_parser>((fun_it->second)());

	if (m_parser->configure(unit, conf)) {
		m_log->log(1, "Couldn't configure parser!");
		return 1;
	}

	return 0;
}

void bc_modbus_driver::ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) {
	m_parser->read_data(conn, data);
}

void bc_modbus_driver::ReadError(const BaseConnection *conn, short int event) {
	m_log->log(3, "Connection error, resetting");
	m_connection->Close();
	m_parser->reset();
	m_unit->reset_cycle();

	reconnection_timer.schedule();
}

void bc_modbus_driver::OpenFinished(const BaseConnection *conn) {
	reconnection_timer.cancel();
	m_unit->start_cycle();
}

void bc_modbus_driver::SetConfigurationFinished(const BaseConnection *conn) {}

void bc_modbus_driver::frame_parsed(ADU &adu, BaseConnection* conn) {
	auto adu_str = adu2string(adu);
	m_log->log(10, "Got frame %s", adu_str.c_str());

	m_unit->adu_received(conn, adu);
}

void bc_modbus_driver::send_frame(BaseConnection* conn, ADU &adu) {
	try {
		auto adu_str = adu2string(adu);
		m_log->log(10, "Sending %s", adu_str.c_str());

		m_parser->send_adu(conn, adu);
	} catch (...) {
		m_log->log(3, "Couldn't send frame!");

	}
}

void bc_modbus_driver::error(const ERROR_CONDITION e) {
	auto error_str = error_condition_str(e);
	m_log->log(5, "Error during frame parsing: %s, resetting", error_str.c_str());

	m_parser->reset();
	m_unit->timeout();
}

void bc_modbus_tcp_client::send_frame(BaseConnection* conn, ADU &adu) {
	adu.trans_id = ++m_trans_id;
	valid_ids.insert(m_trans_id);
	bc_modbus_driver::send_frame(conn, adu);
}

void bc_modbus_tcp_client::frame_parsed(ADU &adu, BaseConnection* conn) {
	if (!adu.trans_id) {
		ASSERT(false);
		error(FRAME_ERROR);
		return;
	}

	if (valid_ids.count(*adu.trans_id) == 0) {
		m_log->log(4, "Received unexpected tranasction id, expected: %u, ignoring the response", unsigned(m_trans_id));
		error(FRAME_ERROR);
		return;
	}

	valid_ids.erase(*adu.trans_id);
	bc_modbus_driver::frame_parsed(adu, conn);
}

void bc_modbus_tcp_client::start_cycle() {
	valid_ids.clear();
	bc_modbus_driver::start_cycle();
}


int bc_tcp_parser::configure(UnitInfo* unit, const SerialPortConfiguration &) {
	return 0;
}

void bc_tcp_parser::send_adu(BaseConnection* conn, ADU &adu) {
	std::vector<unsigned char> head_data;

	auto trans_id = htons(*adu.trans_id);
	auto len = htons(2 + adu.data.size());

	head_data.push_back(trans_id & 0xff);
	head_data.push_back(trans_id >> 8);
	head_data.push_back(0);
	head_data.push_back(0);
	head_data.push_back(len & 0xff);
	head_data.push_back(len >> 8);
	head_data.push_back(adu.unit_id);
	head_data.push_back(adu.func_code);

	conn->WriteData(&head_data[0], head_data.size());
	conn->WriteData(&adu.data[0], adu.data.size());
}

void bc_tcp_parser::read_data(const BaseConnection* conn, const std::vector<unsigned char>& data) {
	ADU adu;
	const int HEADER_SIZE = 6;
	auto it = data.begin();
	if (std::distance(it, data.end()) < HEADER_SIZE) {
		m_log->log(4, "Received too few bytes: %u, ignoring the response", unsigned(std::distance(it, data.end())));
		handler->error(FRAME_ERROR);
		return;
	}

	adu.trans_id = ntohs(*it | (*++it << 8));

	// ommited proto_id (2 bytes)
	it += 2;

	auto length = ntohs(*++it | (*++it << 8));
	auto received_length = std::distance(it, data.end());

	if (received_length < length) {
		m_log->log(4, "Received invalid length in response %u: %li, expected at least %u, ignoring the response", unsigned(*adu.trans_id), received_length, unsigned(length));
		handler->error(FRAME_ERROR);
		return;
	}

	// local unit_id?
	adu.unit_id = *++it;
	adu.func_code = *++it;

	const auto data_len = length - 2;
	adu.data.resize(data_len);
	for (int i = 0; i < data_len; ++i) {
		adu.data[i] = *++it;
	}

	handler->frame_parsed(adu, const_cast<BaseConnection*>(conn));
}

void bc_serial_rtu_parser::calculate_crc_table() {
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

unsigned short bc_serial_rtu_parser::update_crc(unsigned short crc, unsigned char c) {
	if (crc_table_calculated == false)
		calculate_crc_table();
	unsigned short tmp;
	tmp = crc ^ c;
	crc = (crc >> 8) ^ crc16[tmp & 0xff];
	return crc;
}

bool bc_serial_rtu_parser::check_crc() {
	size_t read_bytes = m_input_buffer.size();
	if (read_bytes <= 2)
		return false;

	unsigned short crc = 0xffff;
	for (size_t i = 0; i < read_bytes - 2; i++) 
		crc = update_crc(crc, m_input_buffer[i]);

	unsigned short frame_crc = m_input_buffer[read_bytes - 2] | (m_input_buffer[read_bytes - 1] << 8);
	bool ok = crc == frame_crc;
	m_log->log(8, "Checking crc, result: %s, calculated crc: %hx, frame crc: %hx", ok? "OK" : "ERROR", crc, frame_crc);
	return ok;
}


bc_serial_parser::bc_serial_parser(BaseConnection* conn, bc_handler *serial_handler, slog log): m_log(log), m_connection(conn), m_serial_handler(serial_handler), read_timer(), write_timer() {
	read_timer.set_callback(new FnPtrScheduler([this](){ read_timer_event(); }));
	write_timer.set_callback(new FnPtrScheduler([this](){ write_timer_event(); }));
}

void bc_serial_parser::write_timer_event() {
	if (m_delay_between_chars && m_output_buffer.size()) {
		unsigned char c = m_output_buffer.front();
		m_output_buffer.pop_front();
		m_connection->WriteData(&c, 1);
		if (m_output_buffer.size())
			write_timer.schedule();
	}
	
}

void bc_serial_parser::write_finished() {
	if (m_delay_between_chars && m_output_buffer.size())
		write_timer.schedule();
}

int bc_serial_rtu_parser::configure(UnitInfo* unit, const SerialPortConfiguration &conf) {
	m_delay_between_chars = unit->getAttribute("extra:out-intra-character-delay", 0);
	if (m_delay_between_chars == 0)
		m_log->log(10, "Serial port configuration, delay between chars not given (or 0) assuming no delay");
	else
		m_log->log(10, "Serial port configuration, delay between chars set to %d miliseconds", m_delay_between_chars);

	int bits_per_char = conf.stop_bits
				+ conf.parity == SerialPortConfiguration::NONE ? 0 : 1
				+ conf.GetCharSizeInt();

	float chars_per_sec = conf.speed / bits_per_char;
	/*according to protocol specification, intra-character
	 * delay cannot exceed 1.5 * (time of transmittion of one character) */
	if (chars_per_sec) {
		m_timeout_1_5_c = us((1000000 / chars_per_sec) * (1.5));
		m_timeout_3_5_c = us((1000000 / chars_per_sec) * (3.5));
	}

	if (auto read_timeout = unit->getOptAttribute<int64_t>("extra:read-timeout")) {
		m_timeout_3_5_c = us(1000 * *read_timeout);
	}

	m_log->log(8, "Setting 1.5Tc timer to %ld us", (int64_t) m_timeout_1_5_c);
	m_log->log(8, "Setting 3.5Tc timer to %ld us",  (int64_t) m_timeout_3_5_c);

	if (m_delay_between_chars) {
		write_timer.set_timeout(ms(m_delay_between_chars));
	}

	return 0;
}

void bc_serial_rtu_parser::reset() {
	m_state = ADDR;	
	m_input_buffer.resize(0);
	m_is_3_5_c_timeout = false;
	write_timer.cancel();
	read_timer.cancel();
	m_output_buffer.resize(0);
}

void bc_serial_rtu_parser::read_data(const BaseConnection* conn, const std::vector<unsigned char>& data) {
	for (const auto b: data) {
		m_input_buffer.push_back(b);
	}

	m_log->log(8, "serial_rtu_parser::read_data: got data, resetting timers");

	m_is_3_5_c_timeout = false;
	read_timer.set_timeout(m_timeout_1_5_c);
	read_timer.reschedule();
}

void bc_serial_rtu_parser::read_timer_event() {
	if (m_input_buffer.size() < 2) {
		reset();
		return;
	}

	m_log->log(8, "serial_rtu_parser - read timer expired, didn't get any data for %fTc , checking crc",
		m_is_3_5_c_timeout ? 3.5 : 1.5);

	if (check_crc()) {
		m_is_3_5_c_timeout = false;

		m_adu.unit_id = m_input_buffer.front();
		m_input_buffer.pop_front();
		m_adu.func_code = m_input_buffer.front();
		m_input_buffer.pop_front();

		// remove crc
		m_input_buffer.pop_back();
		m_input_buffer.pop_back();

		m_adu.data.resize(0);
		m_adu.data.insert(m_adu.data.end(), m_input_buffer.begin(), m_input_buffer.end());//std::move(m_input_buffer);

		m_serial_handler->frame_parsed(m_adu, m_connection);
		m_input_buffer.resize(0);
	} else {
		if (m_is_3_5_c_timeout) {
			m_log->log(8, "crc check failed, erroring out");
			m_serial_handler->error(TIMEOUT);
			reset();
		} else {
			m_log->log(8, "crc check failed, scheduling 3.5Tc timer");
			m_is_3_5_c_timeout = true;
			read_timer.schedule(m_timeout_3_5_c - m_timeout_1_5_c);
		}
	}
}

void bc_serial_rtu_parser::send_adu(BaseConnection* conn, ADU &adu) {
	unsigned short crc = 0xffff;
	crc = update_crc(crc, adu.unit_id);
	crc = update_crc(crc, adu.func_code);
	for (size_t i = 0; i < adu.data.size(); i++)
		crc = update_crc(crc, adu.data[i]);

	unsigned char cl, cm;
	cl = crc & 0xff;
	cm = crc >> 8;

	m_output_buffer.resize(0);
	m_output_buffer.push_back(adu.unit_id);
	m_output_buffer.push_back(adu.func_code);
	m_output_buffer.insert(m_output_buffer.end(), adu.data.begin(), adu.data.end());
	m_output_buffer.push_back(cl);
	m_output_buffer.push_back(cm);

	if (m_delay_between_chars) {
		write_timer_event();
	} else {
		conn->WriteData(&m_output_buffer[0], m_output_buffer.size());
	}
}


unsigned char bc_serial_ascii_parser::update_crc(unsigned char c, unsigned char crc) const {
	return crc + c;
}

unsigned char bc_serial_ascii_parser::finish_crc(unsigned char crc) const {
	return 0x100 - crc; 
}

bc_serial_ascii_parser::bc_serial_ascii_parser(BaseConnection* conn, bc_handler* serial_handler, slog log) : bc_serial_parser(conn, serial_handler, log), m_state(COLON) { }

bool bc_serial_ascii_parser::check_crc() {
	unsigned char crc = 0;
	std::vector<unsigned char>& d = m_adu.data;
	crc = update_crc(crc, m_adu.unit_id);
	crc = update_crc(crc, m_adu.func_code);
	if (d.size() == 0)
		return false;
	for (size_t i = 0; i < d.size() - 1; i++)
		crc = update_crc(crc, d[i]);
	crc = finish_crc(crc);
	unsigned char frame_crc = d[d.size() - 1];
	d.resize(d.size() - 1);
	return crc == frame_crc;
}

void bc_serial_ascii_parser::read_data(const BaseConnection* conn, const std::vector<unsigned char>& data) {
	read_timer.cancel();
	for (auto c: data) switch (m_state) {
		case COLON:
			if (c != ':') {
				m_log->log(1, "Modbus ascii frame did not start with the colon");
				m_serial_handler->error(FRAME_ERROR);
				return;
			}
			m_state = ADDR_1; 
			break;
		case ADDR_1:
			m_previous_char = c;
			m_state = ADDR_2;
			break;
		case ADDR_2:
			if (ascii::from_ascii(m_previous_char, c, m_adu.unit_id)) {
				m_log->log(1, "Invalid value received in request");
				m_serial_handler->error(FRAME_ERROR);
				m_state = COLON;
				return;
			}
			m_log->log(9, "Parsing new frame, unit_id: %d", (int) m_adu.unit_id);
			m_state = FUNC_CODE_1;
			break;
		case FUNC_CODE_1:
			m_previous_char = c;
			m_state = FUNC_CODE_2; 
			break;
		case FUNC_CODE_2:
			if (ascii::from_ascii(m_previous_char, c, m_adu.func_code)) {
				m_log->log(1, "Invalid value received in request");
				m_serial_handler->error(FRAME_ERROR);
				m_state = COLON;
				return;
			}
			m_adu.data.resize(0);
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
				m_serial_handler->error(FRAME_ERROR);
				m_state = COLON;
				return;
			}
			m_adu.data.push_back(c);
			m_state = DATA_1;
			break;
		case LF:
			if (c != '\n') {
				m_log->log(1, "Modbus ascii frame did not end with crlf");
				m_state = COLON;
				m_serial_handler->error(FRAME_ERROR);
				return;
			}
			if (check_crc()) 
				m_serial_handler->frame_parsed(m_adu, const_cast<BaseConnection*>(conn));
			else 
				m_serial_handler->error(CRC_ERROR);
			m_state = COLON;
			return;
	}

	read_timer.schedule();
}

void bc_serial_ascii_parser::read_timer_event() {
	m_serial_handler->error(TIMEOUT);
	reset();
}

void bc_serial_ascii_parser::send_adu(BaseConnection* conn, ADU &adu) {
	unsigned char crc = 0;
	unsigned char c1, c2;

	std::vector<unsigned char> out_ascii;

	m_output_buffer.resize(0);

	out_ascii.push_back(adu.unit_id);
	out_ascii.push_back(adu.func_code);

	crc = update_crc(crc, adu.unit_id);
	crc = update_crc(crc, adu.func_code);
	
	for (size_t i = 0; i < adu.data.size(); i++) {
		crc = update_crc(crc, adu.data[i]);
		out_ascii.push_back(adu.data[i]);
	}

	crc = finish_crc(crc);
	out_ascii.push_back(crc);

	m_output_buffer.push_back(':');

	for (const auto c: out_ascii) {
		ascii::to_ascii(c, c1, c2);
		m_output_buffer.push_back(c1);
		m_output_buffer.push_back(c2);
	}

	m_output_buffer.push_back('\r');
	m_output_buffer.push_back('\n');

	conn->WriteData(&m_output_buffer[0], m_output_buffer.size());
}

int bc_serial_ascii_parser::configure(UnitInfo* unit, const SerialPortConfiguration &) {
	m_delay_between_chars = 0;
	ms read_timeout = unit->getAttribute("extra:read-timeout", 1000);
	read_timer.set_timeout(read_timeout);
	m_log->log(9, "serial_parser read timeout: %ld", (int64_t) read_timeout);
	return 0;
}

void bc_serial_ascii_parser::reset() {
	m_state = COLON;
}


driver_iface* create_bc_modbus_tcp_client(BaseConnection* conn, boruta_daemon* boruta, slog log) {
	return new bc_modbus_tcp_client(conn, boruta, log);
}

driver_iface* create_bc_modbus_driver(BaseConnection* conn, boruta_daemon* boruta, slog log) {
	return new bc_modbus_driver(conn, boruta, log);
}
