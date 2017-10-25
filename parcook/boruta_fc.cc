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
	daemon="/opt/szarp/bin/borutadmn"
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
			extra:prec="0"
				conversion index of parameter you want to poll
				you can get it from documents of inverters
				if negative, enter the absolute value to prec
				e.g. if conversion index is -2 give prec="2" (not extra:!)
			extra:val_op="lsw"
				optional operator for converting long or float values to
				parameter values; SZARP holds parameters as 2 bytes integers
				with fixed precision; if val_op is not given, 4 bytes float or
				long value is simply converted to short integer (reflecting
				precision)
				you can also divide one values into 2 szarp parameters (called
				'combined parameters');	you need to configure these 2 parameters
				with the same parameter-number and precision, but one with
				extra:val_op="msw" (it will hold most significant word of value)
				second with extra:val_op="lsw"
			>
		</param>
	</unit>
</device>

 @config_example.pl
<device
	daemon="/opt/szarp/bin/borutadmn"
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
			extra:prec="0"
				conversion index parametru, który chcesz odpytaæ
				uzyskasz to z odpowiedniej dokumentacji falownika
				je¶li ujemny, podaj warto¶æ bezwzglêdn± do atrybutu prec
				np. je¶li conversion index jest -2 podaj prec="2" (bez extra:!)
			extra:val_op="lsw"
				opcjolany operator pozwalaj±y na konwersjê warto¶ci
				typu float i long na warto¶ci parametrów SZARP
				domy¶lnie warto¶ci te zamieniane s± na 2 bajtow± reprezentacjê
				warto¶ci w systemie SZARP bezpo¶rednio, jedynie z uwzglêdnieniem
				precyzji parametru w SZARP mo¿liwe jest jednak przepisanie tych
				warto¶ci do dwóch parametrów SZARP (tak zwane parametry
				'kombinowane') co pazwala na nietracenie precyzji i uwzglednianie
				wiêkszego zakresu
				w tym celu nale¿y skonfigurowaæ 2 parametry SZARP z takimi
				samymi parametrami dotycz±cymi numeru parametru i jego precyzji
				przy czym jeden z nich powinien mieæ extra:val_op="lsw" a
				drugi extra:val_op="msw"
				przyjm± warto¶ci odpowiednio mniej i bardziej znacz±cego s³owa
				warto¶ci parametru
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
#include "ipchandler.h"
#include "tokens.h"
#include "borutadmn.h"

const unsigned char STE = 0x02;	// always 02 HEX
const unsigned char LGE = 0x0E;	// 14 bytes of telegram
const unsigned char AK = 0x1;	// AK=1 means read value data
const unsigned char RESPONSE_SIZE = 16; // 16 bytes of response telegram
const unsigned char MIN_EXTRA_ID = 1; // min address of inverter
const unsigned char MAX_EXTRA_ID = 32; // max address of inverter
const unsigned char SCALING_BOUNDARY = 2; // max scaling precision
const unsigned char SCALING_PREC = 74; // special scaling precision

namespace {
class fc_register
{
	/* Parameter number */
	const unsigned short m_pnu;
	long int m_val;
	driver_logger *m_log;
public:
	fc_register(unsigned short pnu, driver_logger *log) :
		m_pnu(pnu), m_val(SZARP_NO_DATA), m_log(log) {};
	unsigned short get_pnu() const { return m_pnu; }
	void set_val(long int val);
	long int get_val() const;
};

using FCRMAP = std::map<unsigned char, fc_register *>;

class read_val_op
{
protected:
	fc_register *m_reg;
	double m_prec;
	void clear_val();
public:
	read_val_op(fc_register *reg, double prec) : m_reg(reg), m_prec(prec) {};
	virtual short val() = 0;
};

void read_val_op::clear_val()
{
	m_reg->set_val(SZARP_NO_DATA);
}

class short_read_val_op : public read_val_op
{
public:
	short_read_val_op(fc_register *reg, double prec) : read_val_op(reg, prec) {};
	short val() override;
};

class long_read_val_op : public read_val_op
{
protected:
	bool m_lsw;
public:
	long_read_val_op(fc_register *reg, double prec, bool lsw) : read_val_op(reg, prec), m_lsw(lsw) {};
	short val() override;
};

void fc_register::set_val(long int val)
{
	m_val = val;
}

long int fc_register::get_val() const
{
	m_log->log(10, "fc_register::get_val value: %ld", m_val);
	return m_val;
}

short short_read_val_op::val()
{
	long int val = m_reg->get_val();
	clear_val();
	return static_cast<short>(val * m_prec);
}

short long_read_val_op::val()
{
	long int val = static_cast<long int>(m_reg->get_val() * m_prec);
	clear_val();
	return m_lsw ? static_cast<short>(val & 0xFFFF) : static_cast<short>(val >> 16);
}

} //namespace

class fc_proto_impl : public serial_client_driver
{
	unsigned char m_id;
	short *m_read;
	short *m_send;
	size_t m_read_count;
	size_t m_send_count;
	struct bufferevent *m_bufev;
	struct event m_read_timer;
	bool m_read_timer_started;

	/* Address of Danfoss Inverter */
	unsigned char m_extra_id;

	int m_timeout;
	driver_logger m_log;

	/* Serial connection state */
	enum { IDLE, REQUEST, RESPONSE  } m_state;

	/* Vector containing telegram */
	std::vector<unsigned char> m_request_buffer;
	std::vector<unsigned char> m_buffer;

	/* Latest checksum, checked in parse_frame */
	unsigned char m_last_bcc;

	/* Register of parameters under unit */
	FCRMAP m_registers;
	FCRMAP::iterator m_registers_iterator;
	std::vector<read_val_op *> m_read_operators;

	/* Function to create checksum byte (xor of all previous bytes) */
	const char checksum (const std::vector<unsigned char>& buffer);

	void finished_cycle();
	void next_request();
	void send_request();
	void make_read_request();
	int parse_frame();
	void to_parcook();
	bool read_line(struct bufferevent *bufev);

	/* Parser of PWEhigh and PWElow octal chars, returns full 4 byte value */
	long int parse_pwe(const std::vector<unsigned char>& val);

	void start_read_timer();
	void stop_read_timer();

protected:
	void driver_finished_job();
	void terminate_connection();
	struct event_base *get_event_base();

public:
	fc_proto_impl();
	const char *driver_name() override { return "fc_serial_client"; }
	void starting_new_cycle() override;
	void data_ready(struct bufferevent *bufev, int fd) override;
	void connection_error(struct bufferevent *bufev) override;
	int configure(TUnit *unit, xmlNodePtr node, short *read, short *send, serial_port_configuration& spc) override;
	void scheduled(struct bufferevent *bufev, int fd) override;
	void read_timer_event();
	static void read_timer_callback(int fd, short event, void *fc_proto_impl);

};

fc_proto_impl::fc_proto_impl() : m_log(this), m_state(IDLE) {}

const char fc_proto_impl::checksum (const std::vector<unsigned char>& buffer)
{
	m_log.log(10, "checksum");
	char exor = 0x00;
	/* Checksum used for make_read_request and validate last byte of buffer */
	for (size_t i = 0; (i < buffer.size()) && (i < RESPONSE_SIZE - 1); ++i)
		exor ^= buffer.at(i);
	return exor;
}

void fc_proto_impl::finished_cycle()
{
	to_parcook();
}

void fc_proto_impl::next_request()
{
	m_log.log(10, "next_request");

	++m_registers_iterator;
	if (m_registers_iterator == m_registers.end()) {
		m_log.log(10, "next_request, no more registers to query, driver finished job");
		m_state = IDLE;
		m_manager->driver_finished_job(this);
		return;
	}

	send_request();
}

void fc_proto_impl::send_request()
{
	m_log.log(10, "send_request");
	m_buffer.clear();
	make_read_request();
	if(m_request_buffer.size() == RESPONSE_SIZE) {
		bufferevent_write(m_bufev, &m_request_buffer[0], m_request_buffer.size());
		m_state = REQUEST;
		start_read_timer();
	}
	else {
		m_log.log(2, "send_request error - wrong request buffer size, terminating cycle");
		m_registers_iterator = --m_registers.end();
	}
}

void fc_proto_impl::make_read_request()
{
	m_log.log(10, "make_read_request");
	m_request_buffer.clear();
	fc_register *reg = m_registers_iterator->second;

	/* Telegram contains STE - LGE - ADR - PKE - IND - PWE - PCD - BCC */
	m_request_buffer.push_back(STE);
	m_request_buffer.push_back(LGE);
	m_request_buffer.push_back(m_extra_id);

	/* Creating PKE - AK + PNU */
	m_request_buffer.push_back(AK << 4 | ((reg->get_pnu() & 0x0f00) >> 8));
	m_request_buffer.push_back(reg->get_pnu() & 0x00ff);

	/* Expanding telegram to 14 bytes, IND - PWE - PCD */
	for (size_t i = m_request_buffer.size(); i <= LGE; ++i) {
		m_request_buffer.push_back(0x00);
	}

	/* BCC is xor of every byte in request */
	m_request_buffer.push_back(checksum(m_request_buffer));
}

int fc_proto_impl::parse_frame()
{
	if (m_buffer.size() != RESPONSE_SIZE) {
		m_log.log(5, "parse_frame error - received message is wrong (misreceived 16 bytes)");
		m_registers_iterator = --m_registers.end();
		return 1;
	}
	m_log.log(10, "parse_frame, length: %zu", m_buffer.size());

	/* 16 byte is BCC */
	/* Checking wheter checksum is the same as received */
	if (m_buffer.at(15) != m_last_bcc) {
		m_log.log(5, "parse_frame error - received wrong checksum");
		m_registers_iterator = --m_registers.end();
		return 1;
	}

	/* First 7 bytes of buffer should be the same as request_buffer */
	const size_t byte = 7;
	for (size_t i = 0; i < byte; ++i) {
		if(m_buffer.at(i) != m_request_buffer.at(i)) {
			m_log.log(5, "parse_frame error - received buffer is different from requested buffer at %zu byte", i);
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
	long int value = parse_pwe(pwe_chars);
	m_registers_iterator->second->set_val(value);

	/* Next 2 bytes are PCD1 status word */
	const size_t status_word = 2;
	for (size_t i = (byte + pwe_byte); i < (byte + pwe_byte + status_word); ++i) {
		const unsigned char PCD1 = m_buffer.at(i);
		if (PCD1 != 0)
			m_log.log(7, "parse_frame warning - status word PCD1 (byte %zu) is %X", i, PCD1);
	}

	/* Ignoring 15 byte - output frequency */
	/* It informs wheter inverter has start signal (output_freq > 0) */
	return 0;
}

long int fc_proto_impl::parse_pwe(const std::vector<unsigned char>& pwe)
{
	long int value = 0;
	for (const int& value_buffer: pwe) {
		value = (value << 8) | value_buffer;
	}
	return value;

}

void fc_proto_impl::to_parcook()
{
	m_log.log(10, "to_parcook, m_read_count: %zu", m_read_count);
	for (size_t i = 0; i < m_read_count; ++i) {
		m_read[i] = m_read_operators[i]->val();
		m_log.log(10, "Parcook param #%zu set to %hu", i, m_read[i]);
	}
}

void fc_proto_impl::start_read_timer()
{
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 500000;
	evtimer_add(&m_read_timer, &tv);
}

void fc_proto_impl::stop_read_timer()
{
	event_del(&m_read_timer);
}

void fc_proto_impl::driver_finished_job()
{
	m_manager->driver_finished_job(this);
}

void fc_proto_impl::terminate_connection()
{
	m_manager->terminate_connection(this);
}

struct event_base *fc_proto_impl::get_event_base()
{
	return m_event_base;
}

int fc_proto_impl::configure(TUnit *unit, xmlNodePtr node, short *read, short *send, serial_port_configuration& spc)
{
	m_id = unit->GetId();
	m_read_count = unit->GetParamsCount();
	m_send_count = unit->GetSendParamsCount();
	m_read = read;
	m_send = send;

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(node->doc);
	xp_ctx->node = node;
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra", BAD_CAST IPKEXTRA_NAMESPACE_STRING);

	std::string _extra_id;
	if (get_xml_extra_prop(node, "id", _extra_id)) {
		m_log.log(1, "Invalid or missing extra:id attribute in param element at line: %ld", xmlGetLineNo(node));
		return 1;
	}

	const int l = boost::lexical_cast<int>(_extra_id);
	if (l < MIN_EXTRA_ID || l > MAX_EXTRA_ID) {
		m_log.log(1, "Invalid value of extra:id value %d, expected value between 1 and 31 (line %ld)", l, xmlGetLineNo(node));
		return 1;
	}
	/* ADR is address + 128 */
	m_extra_id = l + 128;
	m_log.log(10, "configure extra:id: %02X", m_extra_id);

	for(size_t i = 0; i < m_read_count; ++i) {
		std::stringstream ss;
		ss << ".//ipk:param[position()=" << i+1 << "]";
		const char *expr = ss.str().c_str();
		xmlNodePtr pnode = uxmlXPathGetNode(xmlCharStrdup(expr), xp_ctx, false);
		if (pnode == NULL)
			throw std::runtime_error("Error occured when getting parameter node");

		std::string _pnu;
		if (get_xml_extra_prop(pnode, "parameter-number", _pnu, false)) {
			m_log.log(1, "Invalid or missing extra:parameter-number attribute in param element at line %ld", xmlGetLineNo(pnode));
			return 1;
		}

		const unsigned short pnu = boost::lexical_cast<unsigned short>(_pnu);
		m_log.log(10, "configure extra:parameter-number: %u", pnu);

		std::string _prec;
		if (get_xml_extra_prop(pnode, "prec", _prec, true)) {
			m_log.log(1, "Invalid extra:prec attribute in param element at line: %ld", xmlGetLineNo(pnode));
			return 1;
		}

		const int l = boost::lexical_cast<int>(_prec);
		if (l < 0 || ( l > SCALING_BOUNDARY && l != SCALING_PREC)) {
			m_log.log(1, "Invalid extra:prec attribute value: %d (line %ld)", l, xmlGetLineNo(pnode));
			m_log.log(1, "If conversion-index is negative put it absolute value to prec attribute (not extra:prec)");
			return 1;
		}

		/* Preparing scaling number, if get SCALING_PREC we have to multiply result by 3.4 */
		double prec = 0;
		if (l == SCALING_PREC) {
			prec = 3.4;
		}
		else {
			prec = pow10(l);
		}
		m_log.log(10, "configure extra:prec: %f", prec);

		std::string val_op;
		if (get_xml_extra_prop(pnode, "val_op", val_op, true)) {
			m_log.log(1, "Invalid val_op attribute in param element at line: %ld", xmlGetLineNo(pnode));
			return 1;
		}

		if (val_op.empty()) {
			if (m_registers.find(pnu) != m_registers.end()) {
				m_log.log(1, "Already configured register with extra:parameter-number (%hd) in param element at line: %ld", pnu, xmlGetLineNo(pnode));
				return 1;
			}
			fc_register *reg = new fc_register(pnu, &m_log);
			m_registers[pnu] = reg;
			m_read_operators.push_back(new short_read_val_op(reg, prec));
		}
		else {
			fc_register *reg = nullptr;
			if (m_registers.find(pnu) == m_registers.end()) {
				reg = new fc_register(pnu, &m_log);
				m_registers[pnu] = reg;
			}
			else {
				reg = m_registers[pnu];
			}

			/* Case insensitive comparison */
			if (boost::iequals(val_op, "lsw")) {
				m_read_operators.push_back(new long_read_val_op(reg, prec, true));
			}
			else if (boost::iequals(val_op, "msw")) {
				m_read_operators.push_back(new long_read_val_op(reg, prec, false));
			}
			else {
				m_log.log(1, "Unsupported extra:val_op attribute value - %s, line %ld", val_op.c_str(), xmlGetLineNo(pnode));
				return 1;
			}
		}
	}

	evtimer_set(&m_read_timer, read_timer_callback, this);
	event_base_set(get_event_base(), &m_read_timer);
	return 0;
}

void fc_proto_impl::scheduled(struct bufferevent *bufev, int fd)
{
	m_bufev = bufev;
	m_log.log(10, "scheduled");
	switch (m_state) {
	case IDLE:
		m_registers_iterator = m_registers.begin();
		send_request();
		break;
	case REQUEST:
	case RESPONSE:
		m_log.log(7, "New cycle before end of querying");
		break;
	}
}

void fc_proto_impl::connection_error(struct bufferevent *bufev)
{
	m_log.log(10, "connection_error");
	m_state = IDLE;
	m_bufev = nullptr;
	m_buffer.clear();
	stop_read_timer();
}

void fc_proto_impl::starting_new_cycle()
{
	m_log.log(10, "starting_new_cycle");
	stop_read_timer();
}

void fc_proto_impl::data_ready(struct bufferevent *bufev, int fd)
{
	unsigned char c;
	switch (m_state) {
		case IDLE:
			m_log.log(5, "Got unrequested message, ignoring");
			/* Drain data from bufferevent */
		   /* m_buffer would be cleared before draining response telegram */
			while (bufferevent_read(bufev, &c, 1) != 0) {
				m_log.log(10, "ignored %c", c);
			}
			break;
		case REQUEST:
			while (bufferevent_read(bufev, &c, 1) != 0) {
				/* STE is a starting frame, prepare buffer */
				if (c == STE) {
					m_buffer.clear();
					break;
				}
			}
			m_buffer.push_back(c);
			stop_read_timer();
			m_state = RESPONSE;
		case RESPONSE:
			if(!read_line(bufev) && m_buffer.size() < RESPONSE_SIZE)
				break;
			parse_frame();
			next_request();
			break;
	}
}

bool fc_proto_impl::read_line(struct bufferevent *bufev)
{
	unsigned char c[RESPONSE_SIZE] = "";
	size_t ret = 0;

	while ((ret = bufferevent_read(bufev, c, sizeof(c))) > 0) {
		for (size_t i = 0; i < ret; ++i) {
			m_buffer.push_back(c[i]);
			if (m_buffer.size() == RESPONSE_SIZE) {
				m_last_bcc = checksum(m_buffer);
				return true;
			}
		}
	}

	return false;
}

void fc_proto_impl::read_timer_event()
{
	m_log.log(7, "read_timer_event, state: %d, reg: %d", m_state, m_registers_iterator->first);
	next_request();
}

void fc_proto_impl::read_timer_callback(int fd, short event, void *client)
{
	fc_proto_impl *fc = (fc_proto_impl *) client;
	fc->m_log.log(10, "read_timer_callback");
	fc->read_timer_event();
}

serial_client_driver *create_fc_serial_client()
{
	return new fc_proto_impl();
}

