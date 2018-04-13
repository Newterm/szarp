/*
 SZARP: SCADA software (C) 2017

  Patryk Kulpanowski <pkulpanowski@newterm.pl>
*/

/*
 @description_start

 @class 4

 @devices This is a borutadmn subdriver for FC protocol, used by Danfoss Inverter VLT6000, Danfoss Inverter VLT5000
 @devices.pl Sterownik do demona borutadmn, obs³uguj±cy protokó³ FC, u¿ywany przez  Danfoss VLT6000, VLT5000.

 @protocol FC over RS485 (can be used over TCP/IP)
 @protocol.pl protokó³ FC poprzez port szeregowy RS485 (mo¿e byæ symulowane przez TCP/IP)

 @config Driver is configured as a unit subelement of device element in params.xml. See example for allowed attributes.
 @config.pl Sterownik jest konfigurowany w pliku params.xml, w podelemencie unit elementu device. Opis dodawnych atrybutów XML znajduje siê w przyk³adzie poni¿ej.

 @config_example
<device
	daemon="/opt/szarp/bin/borutadmn_z"
	path="/dev/null"
		ignored, should be /dev/null
	>
	<unit
		id="1"
			ignored, should be ASCII char
		type="1"
			ignored, should be 1
		subtype="1"
			ignored, should be 1
		bufsize="1"
			ignored, should be 1
		extra:proto="fc"
			protocol name, denoting boruta driver to use, should be "fc"
			for this driver
		extra:mode="client"
			unit working mode, this driver is used only as client
		extra:medium="serial"
			data transmission medium, may be "serial" or "tcp"
			for using serial as medium need 1 more attribute:
				extra:path="/dev/ttyS0"
			for using tcp as medium need 3 more attributes:
				extra:use_tcp_2_serial_proxy="yes"
				extra:tcp-address="192.168.1.150"
				extra:tcp-port="6969"
		extra:id="13"
			address of polling inverter (in range from 1 to 31)
		extra:speed="9600"
			optional serial port speed in bps (for medium "serial")
			default is 9600, allowed values are also 300, 600, 1200, 2400, 4800
		extra:parity="even"
			optional serial port parity (for medium "serial")
			default is none, other allowed values are odd and even
		extra:inter-unit-delay="100"
			optional delay time in ms between querying units under device
		>
		<param
			name="Falowniki:Wyci±g Lewy:Napiêcie ³±cza DC"
			...
			extra:parameter-number="518"
				number of parameter you want to poll
				you can get it from documents of inverters
			>
		</param>
	</unit>
</device>

 @config_example.pl
<device
	daemon="/opt/szarp/bin/borutadmn_z"
	path="/dev/null"
		ignorowany, zaleca siê ustawienie /dev/null
	>
	<unit
		id="1"
			ignorowany, zaleca siê wpisacie znaku ASCI
		type="1"
			ignorowany, zaleca siê ustawienie 1
		subtype="1"
			ignorowany, zaleca siê ustawienie 1
		bufsize="1"
			ignorowany, zaleca siê ustawienie 1
		extra:proto="fc"
			nazwa protoko³u, u¿ywana przez Borutê do ustalenia u¿ywanego
			sterownika, dla tego sterownika musi byæ fc
		extra:mode="client"
			tryb pracy jednostki, ten sterownik dzia³a tylko jako client
		extra:medium="serial"
			medium transmisyjne, mo¿e byæ serial albo tcp,
			w celu u¿ywania transmisji szeregowej nale¿y dodaæ atrybut:
				extra:path="/dev/ttyS0"
					¶cie¿ka do portu szeregowego
			w celu u¿ywania transmisji po ethernecie nale¿y dodaæ atrybuty:
				extra:use_tcp_2_serial_proxy="yes"
					pozwolenie na komunikacjê szeregow± poprzez tcp
				extra:tcp-address="192.168.1.150"
					adres IP do którego siê pod³±czamy
				extra:tcp-port="6969"
					port IP na który siê po³±czymy
		extra:id="13"
			adres odpytywanego falownika (od 1 do 31), odczytywane z falownika
		extra:speed="9600"
			opcjonalna, prêdko¶æ portu szeregowego w bps (dla medium serial)
			domy¶lna jest 9600, dopuszczalne warto¶ci 300, 600, 1200, 2400, 4800
		extra:parity="even"
			opcjonalna, parzysto¶æ portu (dla medium serial)
			domy¶lne jest none, dopuszczalne warto¶ci odd, even
		extra:inter-unit-delay="100"
			opcjonalna, czas opó¼nienia w ms miêdzy odpytywaniem jednostek w
			jednym urz±dzeniu
		>
		<param
			name="Falowniki:Wyci±g Lewy:Napiêcie ³±cza DC"
			...
			extra:parameter-number="518"
				numer parametru, który chcesz odpytaæ
				numer uzyskasz z odpowiedniej dokumentacji falownika
			>
		</param>
	</unit>
</device>

 @description_end
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdexcept>

#include <event.h>

#include <boost/algorithm/string/predicate.hpp>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "liblog.h"
#include "xmlutils.h"
#include "conversion.h"
#include "zmqhandler.h"
#include "tokens.h"
#include "borutadmn_z.h"
#include "sz4/defs.h"

const unsigned char STE = 0x02;	// always 02 HEX
const unsigned char LGE = 0x0E;	// 14 bytes of telegram
const unsigned char AK = 0x1;	// AK=1 means read value data
const unsigned char RESPONSE_SIZE = 16; // 16 bytes of response telegram
const unsigned char MIN_EXTRA_ID = 1; // min address of inverter
const unsigned char MAX_EXTRA_ID = 32; // max address of inverter

class fc_register;
using FCRMAP = std::map<unsigned int, fc_register *>;

class fc_proto : public bc_driver
{
	Scheduler read_timeout;

	zmqhandler* m_zmq;

	size_t m_read;
	size_t m_send;
	size_t m_read_count;
	size_t m_send_count;

	/* Address of Danfoss Inverter */
	unsigned char m_extra_id;

	slog m_log;

	/* Time variables */
	sz4::nanosecond_time_t m_current_time;
	long long m_expiration_time;

	/* Serial connection state */
	enum { IDLE, REQUEST, RESPONSE } m_state;

	/* Vector containing telegram */
	std::vector<unsigned char> m_request_buffer;
	std::vector<unsigned char> m_buffer;

	/* Latest checksum, checked in parse_frame */
	unsigned char m_last_bcc;

	/* Register of parameters under unit */
	FCRMAP m_registers;
	FCRMAP::iterator m_registers_iterator;

	/* Function to create checksum byte (xor of all previous bytes) */
	const char checksum (const std::vector<unsigned char>& buffer);

	void finished_cycle();
	void next_request();
	void send_request();
	void make_read_request();
	int parse_frame();
	void to_parcook();

	/* Parser of PWEhigh and PWElow octal chars, returns full 4 byte value */
	int parse_pwe(const std::vector<unsigned char>& val);

	void start_read_timer();
	void stop_read_timer();

protected:
	void terminate_connection();

public:
	fc_proto(BaseConnection* conn, boruta_daemon* boruta, slog log);
	void starting_new_cycle() override;
	bool register_val_expired(const sz4::nanosecond_time_t& time);
	int configure(TUnit* unit, size_t read, size_t send, const SerialPortConfiguration&) override;
	void configure_register(TAttribHolder* param);
	void scheduled();
	void read_timer_event();
	static void read_timer_callback(int fd, short event, void *fc_proto);

	// ConnectionListener impl
	void OpenFinished(const BaseConnection *conn) override {}
	void ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) override;
	void ReadError(const BaseConnection *conn, short int event) override;
	void SetConfigurationFinished(const BaseConnection *conn) override {}

};

using register_iface = register_iface_t<int32_t>;
class fc_register: public register_iface, public parcook_val_op {
	fc_proto *m_fc_proto;
	sz4::nanosecond_time_t m_mod_time = sz4::time_trait<sz4::nanosecond_time_t>::invalid_value;

public:
	fc_register(fc_proto* daemon): m_fc_proto(daemon) {};

	bool is_valid() const override {
		return sz4::time_trait<sz4::nanosecond_time_t>::is_valid(m_mod_time) && !(m_fc_proto->register_val_expired(m_mod_time));
	}

	sz4::nanosecond_time_t get_mod_time() const override {
		return m_mod_time;
	}

	void set_mod_time(const sz4::nanosecond_time_t& time) override {
		m_mod_time = time;
	}
};

template <typename value_type>
class fc_register_impl: public fc_register {
	value_type m_val = sz4::no_data<value_type>();

public:
	using fc_register::fc_register;

	int32_t get_val() const override { return (int32_t) m_val; } // unused in fc (no sends)

	void set_val(int val) override {
		m_val = (value_type) val;
	}

	void publish_val(zmqhandler *handler, size_t index) override {
		if (is_valid())
			handler->set_value(index, get_mod_time(), m_val);
	}
};


fc_proto::fc_proto(BaseConnection* conn, boruta_daemon* boruta, slog log): bc_driver(conn), read_timeout(boruta->get_event_base()), m_zmq(boruta->get_zmq()), m_log(log), m_state(IDLE) {
	auto callback = [this](){ read_timer_event(); };
	read_timeout.set_callback(new LambdaScheduler<decltype(callback)>(std::move(callback)));
}

const char fc_proto::checksum (const std::vector<unsigned char>& buffer)
{
	m_log->log(10, "checksum");
	char exor = 0x00;
	/* Checksum used for make_read_request and validate last byte of buffer */
	for (size_t i = 0; (i < buffer.size()) && (i < RESPONSE_SIZE - 1); ++i)
		exor ^= buffer.at(i);
	return exor;
}

void fc_proto::finished_cycle()
{
	to_parcook();
}

void fc_proto::next_request()
{
	m_log->log(10, "next_request");

	++m_registers_iterator;
	if (m_registers_iterator == m_registers.end()) {
		m_log->log(10, "next_request, no more registers to query, driver finished job");
		m_state = IDLE;
		return;
	}

	send_request();
}

void fc_proto::send_request()
{
	m_log->log(10, "send_request");
	m_buffer.clear();
	make_read_request();
	if(m_request_buffer.size() == RESPONSE_SIZE) {
		m_connection->WriteData(&m_request_buffer[0], m_request_buffer.size());
		m_state = REQUEST;
		start_read_timer();
	} else {
		m_log->log(2, "send_request error - wrong request buffer size, terminating cycle");
		m_registers_iterator = --m_registers.end();
	}
}

void fc_proto::make_read_request()
{
	m_log->log(10, "make_read_request");
	m_request_buffer.clear();

	/* Telegram contains STE - LGE - ADR - PKE - IND - PWE - PCD - BCC */
	m_request_buffer.push_back(STE);
	m_request_buffer.push_back(LGE);
	m_request_buffer.push_back(m_extra_id);

	/* Creating PKE - AK + PNU */
	m_request_buffer.push_back(AK << 4 | ((m_registers_iterator->first & 0x0f00) >> 8));
	m_request_buffer.push_back(m_registers_iterator->first & 0x00ff);

	/* Expanding telegram to 14 bytes, IND - PWE - PCD */
	for (size_t i = m_request_buffer.size(); i <= LGE; ++i) {
		m_request_buffer.push_back(0x00);
	}

	/* BCC is xor of every byte in request */
	m_request_buffer.push_back(checksum(m_request_buffer));
}

int fc_proto::parse_frame()
{
	if (m_buffer.size() != RESPONSE_SIZE) {
		m_log->log(5, "parse_frame error - received message is wrong (misreceived 16 bytes)");
		m_registers_iterator = --m_registers.end();
		return 1;
	}
	m_log->log(10, "parse_frame, length: %zu", m_buffer.size());

	/* 16 byte is BCC */
	/* Checking wheter checksum is the same as received */
	if (m_buffer.at(15) != m_last_bcc) {
		m_log->log(2, "parse_frame error - received wrong checksum");
		m_registers_iterator = --m_registers.end();
		return 1;
	}

	/* First 7 bytes of buffer should be the same as request_buffer */
	/* Is it possbile to receive AK 0x02, it means we get double world */
	const uint8_t byte = 7;
	for (uint8_t i = 0; i < byte; ++i) {
		if(i == 3)
			continue;
		if(m_buffer.at(i) != m_request_buffer.at(i)) {
			m_log->log(2, "parse_frame error - received buffer is different from requested buffer at %u byte", i);
			m_registers_iterator = --m_registers.end();
			return 1;
		}
	}

	std::vector<unsigned char> pwe_chars;
	/* Next 4 bytes are values PWEhigh PWElow */
	const size_t pwe_byte = 4;
	for (size_t i = byte; i < (byte + pwe_byte); ++i) {
		if (m_buffer.at(i) == 0)
			continue;
		pwe_chars.push_back(m_buffer[i]);
	}

	/* Convert octal char to int and set 4 bytes from PWE to register */
	int value = parse_pwe(pwe_chars);
	m_log->log(10, "Setting value: %d", value);
	m_registers_iterator->second->set_val(value);
	m_registers_iterator->second->set_mod_time(m_current_time);


	/* Next 2 bytes are PCD1 status word */
	const size_t status_word = 2;
	for (size_t i = (byte + pwe_byte); i < (byte + pwe_byte + status_word); ++i) {
		const unsigned char PCD1 = m_buffer.at(i);
		if (PCD1 != 0)
			m_log->log(7, "parse_frame warning - status word PCD1 (byte %zu) is %X", i, PCD1);
	}

	/* Ignoring 15 byte - output frequency */
	/* It informs wheter inverter has start signal (output_freq > 0) */
	return 0;
}

int fc_proto::parse_pwe(const std::vector<unsigned char>& pwe)
{
	int value = 0;
	for (const int& value_buffer: pwe) {
		value = (value << 8) | value_buffer;
	}
	return value;

}

void fc_proto::to_parcook()
{
	size_t i = 0;
	m_log->log(10, "to_parcook, m_read_count: %zu", m_read_count);
	for (auto reg: m_registers) {
		reg.second->publish_val(m_zmq, i + m_read);
		++i;
	}
}

void fc_proto::start_read_timer()
{
	read_timeout.schedule();
}

void fc_proto::stop_read_timer()
{
	read_timeout.cancel();
}


void fc_proto::terminate_connection()
{
	m_connection->Close();
}

int fc_proto::configure(TUnit *unit, size_t read, size_t send, const SerialPortConfiguration&)
{
	m_read_count = unit->GetParamsCount();
	m_send_count = unit->GetSendParamsCount();
	m_read = read;
	m_send = send;

	const long long nanosec_conv_factor = 1000000000;
	m_expiration_time = unit->getAttribute<int>("extra:nodata-timeout", 60) * nanosec_conv_factor;

	auto l = unit->getAttribute<int>("extra:id", -1);
	if (l < MIN_EXTRA_ID || l > MAX_EXTRA_ID) {
		m_log->log(1, "Invalid value of extra:id value %d, expected value between 1 and 31 (unit %d)", l, unit->GetUnitNo());
		return 1;
	}
	/* ADR is address + 128 */
	m_extra_id = l + 128;
	m_log->log(10, "configure extra:id: %02X", m_extra_id);

	for(auto param: unit->GetParams()) {
		try {
			configure_register(param);
		} catch(const std::exception& e) {
			m_log->log(1, "Error while configuring param %ls", param->GetName().c_str());
			throw;
		}
	}

	read_timeout.set_timeout(util::ms(1500));
	return 0;
}

void fc_proto::configure_register(TAttribHolder* param) {
	auto pnu = param->getOptAttribute<unsigned>("extra:parameter-number");
	if (!pnu)
		throw std::runtime_error("No option extra:parameter-number passed!");

	m_log->log(10, "configure extra:parameter-number: %d", *pnu);

	std::string val_type = param->getAttribute<std::string>("extra:val_type");
	if (val_type == "ushort") {
		m_registers[*pnu] = new fc_register_impl<uint16_t>(this);
	}
	else if (val_type == "uinteger") {
		m_registers[*pnu] = new fc_register_impl<uint32_t>(this);
	}
	else if (val_type == "integer") {
		m_registers[*pnu] = new fc_register_impl<int32_t>(this);
	}
	else {
		m_log->log(1, "Unsupported value type: %s", val_type.c_str());
		throw std::runtime_error("Invalid value type");
	}
}

void fc_proto::scheduled()
{
	m_log->log(10, "scheduled");
	switch (m_state) {
	case IDLE:
		m_registers_iterator = m_registers.begin();
		send_request();
		break;
	case REQUEST:
	case RESPONSE:
		m_log->log(7, "New cycle before end of querying");
		break;
	}
}

void fc_proto::ReadError(const BaseConnection *conn, short int event) {
	m_log->log(10, "connection error");
	m_state = IDLE;
	m_buffer.clear();
	stop_read_timer();
}

void fc_proto::starting_new_cycle()
{
	m_log->log(10, "starting_new_cycle");
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	m_current_time.second = time.tv_sec;
	m_current_time.nanosecond = time.tv_nsec;
	stop_read_timer();

	try {
		if (!m_connection->Ready()) {
			m_connection->Close();
			m_connection->Open();
		}
	} catch (...) {
	}

	scheduled();
}

void fc_proto::ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) {
	for (auto c: data) switch (m_state) {
		case IDLE:
			m_log->log(5, "Got unrequested message, ignoring");
			/* Drain data from bufferevent */
		   /* m_buffer would be cleared before draining response telegram */
			m_log->log(10, "ignored %c", c);
			break;
		case REQUEST:
			/* STE is a starting frame, prepare buffer */
			if (c != STE)
				break;

			m_buffer.clear();
			stop_read_timer();
			m_state = RESPONSE;
		case RESPONSE:
			m_buffer.push_back(c);
			if (m_buffer.size() == RESPONSE_SIZE) {
				m_last_bcc = checksum(m_buffer);
				parse_frame();
				next_request();
			}

			break;
	}
}

bool fc_proto::register_val_expired(const sz4::nanosecond_time_t& time) {
	if (m_expiration_time == 0) {
		return false;
	}
	else {
		auto t = time;
		t += m_expiration_time;
		return t <= m_current_time;
	}
}

void fc_proto::read_timer_event()
{
	m_log->log(7, "read_timer_event, state: %d, reg: %d", m_state, m_registers_iterator->first);
	next_request();
}

void fc_proto::read_timer_callback(int fd, short event, void *client)
{
	fc_proto *fc = (fc_proto *) client;
	fc->m_log->log(10, "read_timer_callback");
	fc->read_timer_event();
}

driver_iface* create_fc_serial_client(BaseConnection* conn, boruta_daemon* boruta, slog log)
{
	return new fc_proto(conn, boruta, log);
}

