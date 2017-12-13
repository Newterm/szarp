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
 * $Id$

 * Paweł Pałucha <pawel@praterm.com.pl>
 *
 * Parses sorted output from process_csv script and writes to szarp base. Also
 * can alter params.xml file (adds new parameters).
 * 
 * 
 * Expected input lines format:
 * "<name of parameter> <year> <month> <day> <hour> <min> <value>\n"
 * 
 * Lines must be sorted (at least by date within single param). The easiest
 * way to do this is to make sure that date and time fields are zero-padded
 * to correct length and then just use 'sort' program.
 * If parameter doesn't exist, it is added to configuration (params.xml is
 * modified). Draw window names' and short param names' are guessed. 
 * Exisiting precision is used; for new parameters it is guessed from number 
 * of digits after period.
 * Unit name can be extracted from name of parameter - if name end with string
 * in brackets ([..]), this string is treated as unit name.
 * Maximum and minimum values for draws are based on range of values in base.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fnmatch.h>
#include <tr1/unordered_map>

#include <argp.h>

#include <sys/types.h>
#include <utime.h>

#include "liblog.h"
#include "libpar.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "tokens.h"

#include "conversion.h"

#include "szbwriter_cache.h"

#define SZARP_CFG_SECTION	"szbwriter"
#define MEANER3_CFG_SECTION	"meaner3"
#define SZARP_CFG		"/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg"
#define DEFAULT_LOG		PREFIX"/logs/szbwriter.log"

using namespace std::tr1;


/**
 * incremental evaluating time from tm struct. In first 
 * version it was also adding hours, but DST was a problem.
 * Maybe we should all time to DST, not local.
 */
time_t update_time( struct tm& newtm , time_t oldt , const struct tm& oldtm )
{
	if( newtm.tm_year != oldtm.tm_year 
	 || newtm.tm_mon  != oldtm.tm_mon 
	 || newtm.tm_mday != oldtm.tm_mday
	 || newtm.tm_hour != oldtm.tm_hour )
		return mktime(&newtm);

//        int dh = newtm.tm_hour - oldtm.tm_hour;
	int dm = newtm.tm_min  - oldtm.tm_min ;
	int ds = newtm.tm_sec  - oldtm.tm_sec ;

	return oldt + dm * 60 + ds;
//        return oldt + (dh*60+dm)*60+ds;
}

class SzbaseWriter;

class DataWriter {
	public:
		enum PROBE_TYPE { MIN10 = 0, SEC10, LAST_PROBE_TYPE };

		DataWriter(SzbaseWriter* parent, DataWriter::PROBE_TYPE ptype, const std::wstring& data_dir, int fill_how_many);
		~DataWriter();

		int add_data(const std::wstring& name, bool is_dbl, time_t t, double data);

		/** Fill gaps in data. Range: [begin, end)
		 * @return 0 on success, 1 on error */
		int fill_gaps(time_t begin, time_t end, double sum, int count);

		/** Saves data for current index.
		 * @return 0 on success, 1 on error */
		int save_data();

		int close_data();

	protected:
		SzbaseWriter* m_parent;
		PROBE_TYPE m_probe_type;
		int m_fill_how_many;
		time_t m_probe_length;

		SzProbeCache* m_cache;

		std::wstring m_cur_name{};

		bool m_is_double;

		time_t m_cur_t;
		int m_cur_cnt;
		double m_cur_sum;

};

class SzbaseWriter : public TSzarpConfig {
public:
	SzbaseWriter(const std::wstring &ipk_path, const std::wstring &double_pattern,
			const std::wstring& data_dir, const std::wstring &cache_dir, bool add_new_pars,
			bool write_10sec, int _fill_how_many , int _fill_how_many_sec );
	~SzbaseWriter();
	/* Process input. 
	 * @return 1 on error, 0 on success*/
	int process_input();
	bool have_new_params();
protected:
	/** Checks parameter name for unit string. If found, unit name is
	 * stripped from name and stdup() copy of unit name is returned.
	 * @param parameter's name string, may be modified
	 * @return copy of unit name or NULL if no unit is available
	 */
	char *check_unit(char *name);
	/** Adds parameter to config (if doesn't exists already). 
	 * @param name name of parameter
	 * @param unit name of unit, may be NULL
	 * @param prec precision of parameter
	 * @return parameter precision, can be different from prec if parameter
	 * already exists with different precision (old one is used - otherwise we
	 * should modify values in base)
	 */
	int add_param(const std::wstring& name, const std::wstring& unit, int prec);
	/** Adds data to base.
	 * @return 0 on success, 1 on error */
	int add_data(const std::wstring& name, const std::wstring& unit, int year, int month, int day, 
			int hour, int min, int sec, const std::wstring &data);
	/** Process sigle input line.
	 * @return 1 on error, 0 on success*/
	int process_line(char *line);


	/** Checks if param should be saved in two words.
	 * @param name name of param
	 * @return 1 for double params, 0 otherwise */
	int is_double(const std::wstring &name);

	std::wstring m_double_pattern;	/**< shell pattern for names of params
					  that should be saved in two words,
					  may be NULL */

	std::vector<DataWriter*> m_writers;

	TParam *m_cur_par;		/**< pointer to currently saved param */
	std::wstring m_cur_name;		/**< name of currently opened file;
					(virtual - real names may be different
					becasue of low/high words) */

	unordered_map<std::wstring, int> m_draws_count;
	bool m_add_new_pars; 	/**< flag denoting if we add new pars*/
	bool m_new_par; 	/**< if new parameter was added */
	size_t m_last_type;	/**< last type of probe to write + 1 */

	struct tm m_tm{}; /**< last time */
	time_t m_t{}; /**< last time */
};

DataWriter::DataWriter(SzbaseWriter* parent, DataWriter::PROBE_TYPE ptype, const std::wstring& data_dir, int fill_how_many)
	: m_parent(parent), m_probe_type(ptype), m_fill_how_many(fill_how_many)
{
	switch (m_probe_type) {
		case MIN10:
			m_probe_length = 60 * 10;
			break;
		case SEC10:
			m_probe_length = 10;
			break;
		default:
			sz_log(0, "Unsupported probe type, exiting");
			exit(1);
	}

	m_cur_t = 0;
	m_cur_cnt = 0;
	m_cur_sum = 0;

	m_cache = new SzProbeCache(parent, data_dir, m_probe_length);
}

DataWriter::~DataWriter()
{
	if (m_cache)
		delete m_cache;
}


int DataWriter::add_data( const std::wstring& name, bool is_dbl, time_t t, double data)
{
	sz_log(10, "DataWriter::add_data begin: m_cur_name=%s, name=%s, t=%ld, data=%f",
		SC::S2U(m_cur_name).c_str(), SC::S2U(name).c_str(), t, data);

	if (name != m_cur_name) {
		if (close_data())
			return 1;

		m_is_double = is_dbl;

		m_cur_sum = data;
		m_cur_cnt = 1;

		m_cur_t = t / m_probe_length * m_probe_length;
		sz_log(10, "DataWriter::add_data: (name != n_cur_name) t = %ld, m_cur_t = %ld", t, m_cur_t);

		m_cur_name = name;
	}

	sz_log(10, "DataWriter::add_data: t=%ld, m_cur_t=%ld, t-m_cur=%ld, m_len=%ld",
			t, m_cur_t, t - m_cur_t, m_probe_length);

	if (t >= m_cur_t && t - m_cur_t < m_probe_length)
		return 0;

	// save data for gaps
	time_t _time = m_cur_t;
	double _sum = m_cur_sum;
	time_t _count = m_cur_cnt;

	if (save_data())
		return 1;

	sz_log(10, "DataWriter::add_data: t = %ld", t);
	m_cur_t = t / m_probe_length * m_probe_length;
	sz_log(10, "DataWriter::add_data: m_cur_t = %ld", m_cur_t);

	time_t gap = m_cur_t - _time;
	sz_log(10, "DataWriter::add_data: check time gap: %ld", gap);

	// if there is a gap bigger than normal probe length, fill gaps
	if( gap > m_probe_length && fill_gaps(_time + m_probe_length , _time + gap, _sum, _count) )
		return 1;

	m_cur_sum += data;
	m_cur_cnt++;

	sz_log(10, "DataWriter::add_data end: t=%ld, sum=%lf, cnt=%d, len=%ld",
		m_cur_t, m_cur_sum, m_cur_cnt, m_probe_length);

	return 0;
}

/**
 * FIXME: What if m_fill_how_many is 0 and end-begin is also 0?
 *        Now it writes one probe exactly in begin==end.
 */
int DataWriter::fill_gaps(time_t begin, time_t end, double sum, int count) 
{
	if( (end - begin) / m_probe_length > m_fill_how_many )
		return 0;

	sz_log(10, "DataWriter::fill_gaps begin, begin=%ld, end=%ld", begin, end);

	// save current data
	time_t save_time = m_cur_t;
	time_t save_cnt = m_cur_cnt;
	double save_sum = m_cur_sum;

	// fill gaps
	for(time_t t = begin; t < end; t += m_probe_length )
	{
		sz_log(10, "DataWriter::fill_gaps for t=%ld", t);

		m_cur_t = t;
		m_cur_cnt = count;
		m_cur_sum = sum;

		if( save_data() )
			return 1;
	}

	// restore data
	m_cur_t = save_time;
	m_cur_cnt = save_cnt;
	m_cur_sum = save_sum;

	sz_log(10, "DataWriter::fill_gaps end");
	return 0;
}

int DataWriter::save_data()
{
	sz_log(10, "DataWriter::save_data %s", !m_cur_cnt ? "end - NO DATA" : "data exists");

	if (!m_cur_cnt)
		return 0;

	double d = m_cur_sum / m_cur_cnt;

	sz_log(10,"DataWriter::save_data: current sum = %lf, current count =%d, current time=%ld, value=%G",
			m_cur_sum, m_cur_cnt, m_cur_t, d);

	int year, month;
	assert( szb_time2my(m_cur_t, &year, &month) == 0 );

	try {
		m_cache->add(
			SzProbeCache::Key(
				m_cur_name,
				m_is_double,
				year,
				month),
			SzProbeCache::Value( d, m_cur_t ) );

	}
	catch( SzProbeCache::failure& f ) {
		return 1;
	}

	m_cur_cnt = 0;
	m_cur_sum = 0;

	sz_log(10, "DataWriter::save_data end - cnt[pt] = 0, sum[pt]= 0");

	return 0;
}

int DataWriter::close_data()
{
	sz_log(10, "DataWriter::close_data");
	if (save_data())
		return 1;

	m_cache->flush();

	return 0;
}

SzbaseWriter::SzbaseWriter(
		const std::wstring &ipk_path,
		const std::wstring& double_pattern,
		const std::wstring& data_dir, const std::wstring& cache_dir, 
		bool add_new_pars,
		bool write_10sec,
		int _fill_how_many , int _fill_how_many_sec ) 
	: m_double_pattern(double_pattern), m_add_new_pars(add_new_pars)
{

	m_writers.push_back(new DataWriter(this, DataWriter::MIN10, data_dir, _fill_how_many));
	if (write_10sec)
		m_writers.push_back(new DataWriter(this, DataWriter::SEC10, cache_dir, _fill_how_many_sec));

	m_new_par = false;

	if (loadXML(ipk_path) == 0) {
		for (TParam* p = GetFirstParam(); p; p = GetNextParam(p)) {
			for (TDraw* d = p->GetDraws(); d; d = d->GetNext()) {
				std::wstring w = d->GetWindow();
				std::wstring c;
				std::wstring::size_type s = w.find(L" (");
				if (s == std::wstring::npos)
					c = w;
				else
					c = w.substr(0, s);
				if (m_draws_count.find(w) == m_draws_count.end()) {
					m_draws_count[w] = 1;
				} else {
					m_draws_count[w] = m_draws_count[w] + 1;
				}
			}
		}
		return;
	}

	/* create empty configuration */
	read_freq = send_freq = 10;
	AddDevice(createDevice(L"/bin/true", L"fake"));
	TRadio * pr = GetFirstDevice()->AddRadio(createRadio(devices));
	TUnit * pu = pr->AddUnit(createUnit(pr, 'x'));
	TParam* p = new TParam(pu);
	pu->AddParam(p);
	
	p->SetFormula(L"fake");
	p->Configure(L"fake:fake:fake", L"fake", L"fake"
			, NULL, 0);
	
	p->SetAutoBase();

	m_new_par = true;
}

SzbaseWriter::~SzbaseWriter()
{
	while (!m_writers.empty()) {
		delete (m_writers.back());
		m_writers.pop_back();
	}
}


/** Guess precision of value (number of digits after period). */
int guess_prec(const std::wstring &data)
{
	int i;
	std::wstring::size_type p = data.find(L'.');
	if (p == std::wstring::npos)
		return 0;
	for (i = 0, p++; iswdigit(data[p]) && p < data.size(); i++, p++);
	return i;
}

/** Create short name from full name of param. This is specific for
 * Samsons - get's prefix of last token after '-' character or after ' '
 * or just prefix of name */
std::wstring short_name(const std::wstring &name)
{
	std::wstring::size_type p;
	p = name.rfind(L'-');
	if (p == std::wstring::npos)
		p = name.rfind(L' ');
	if (p == std::wstring::npos) {
		return name.substr(0, 4);
	} else {
		return name.substr(p + 1, 4);
	}
}

char *SzbaseWriter::check_unit(char *name)
{
	int l;
	char *c, *ret;
	assert (name != NULL);
	l = strlen(name);
	if (l < 4)
		return NULL;
	if (name[l-1] != ']')
		return NULL;
	c = rindex(name, '[');
	if (c == NULL)
		return NULL;
	name[l-1] = 0;
	ret = strdup(c+1);
	while ((c > name) && (*(c-1) == ' ')) {
		c--;
	}
	*c = 0;
	if (strlen(ret) > 5)
		ret[5] = 0;
	return ret;
}

int SzbaseWriter::add_param(const std::wstring &name, const std::wstring& unit, int prec)
{
	TParam *p;
	std::wstring draww;
	int i;
	
	sz_log(2, "Adding new parameter: %s", SC::S2U(name).c_str());
	m_new_par = true;

	if (is_double(name)) {
		std::wstring n1, n2, formula;
		TParam *p1, *p2;
		n1 = name + L" msw";
		n2 = name + L" lsw";
		p1 = new TParam(NULL, this, L"null ", TParam::RPN);
		p2 = new TParam(NULL, this, L"null ", TParam::RPN);
		p1->Configure(n1, short_name(n1), L"",
			L"-", NULL, 0, -1, 1);
		p2->Configure(n2, short_name(n2), L"",
			!unit.empty() ? unit : L"-", NULL, 0, -1, 1);
		formula = std::wstring(L"(") + n1 + L") (" + n2 + L") : ";
		p = new TParam(NULL, this, formula, TParam::DEFINABLE);
		p->Configure(name, short_name(name), L"",
			!unit.empty() ? unit : L"-", NULL, 2, -1, 1);
		AddDefined(p1);
		AddDefined(p2);
		AddDrawDefinable(p);
		prec = 0;
	} else {
		p = new TParam(NULL, this, L"null ", TParam::RPN);
		p->Configure(name, short_name(name), L"",
				!unit.empty() ? unit : L"-", 
				NULL, prec, -1, 1);
		AddDefined(p);
	}

	/* name of draws window */
	std::wstring::size_type pos = name.rfind(L":");
	if (pos == std::wstring::npos)
		draww = name.substr(pos + 1);
	else
		draww = name;

	/* check numer of draws */
	int count;
	if (m_draws_count.find(draww) == m_draws_count.end())
		count = m_draws_count[draww] = 1;
	else
		count = m_draws_count[draww] = m_draws_count[draww] + 1;
	if (count > 12) {
		std::wstringstream wss;
		i = (count+1) / 12;
		wss << draww << L" " << i + 1;
		draww = wss.str();
	}
	
	p->AddDraw(new TDraw(L"", draww, -1, -1, 0, 1, 0, 0, 0));

	m_cur_par = p;
		
	return prec;
}

int SzbaseWriter::is_double(const std::wstring& name)
{
	if (m_double_pattern.empty())
		return 0;
#ifndef FNM_EXTMATCH
	return !(fnmatch(SC::S2A(m_double_pattern).c_str(), SC::S2A(name).c_str(), 0));
#else
	return !(fnmatch(SC::S2A(m_double_pattern).c_str(), SC::S2A(name).c_str(), FNM_EXTMATCH));
#endif
}

int SzbaseWriter::add_data(const std::wstring &name, const std::wstring &unit, int year, int month, int day, 
		int hour, int min, int sec, const std::wstring& data)
{

	sz_log(10, "SzbaseWriter::add_data begin: name=%s, [%d-%d-%d %d:%d:%d] data=%s",
			SC::S2U(name).c_str(), year, month, day, hour, min, sec, SC::S2U(data).c_str());

	struct tm tm{};

	// get UTC time 
	tm.tm_year  = year  - 1900;
	tm.tm_mon   = month - 1;
	tm.tm_mday  = day;
	tm.tm_hour  = hour;
	tm.tm_min   = min;
	tm.tm_sec   = sec;
	tm.tm_isdst = -1;

	m_t = update_time(tm, m_t, m_tm);
	m_tm = tm;

	int is_dbl = is_double(name);

	sz_log(10, "SzbaseWriter::add_data: name=%s, m_cur_name=%s",
		       	SC::S2U(name).c_str(), SC::S2U(m_cur_name).c_str());
	
	if (name != m_cur_name) {
		TParam* cur_par = getParamByName(name);
		if (NULL == cur_par) {
			if (m_add_new_pars) {
				add_param(name, unit, guess_prec(data));
				cur_par = getParamByName(name);
			} else {
			sz_log(1, "Param %s not found in configuration and "
					"program run without -n flag, value ignored!",
					SC::S2U(name).c_str());
				return 1;
			}
		}
		assert(cur_par != nullptr);
		// here we should have params added
		if (is_dbl) {
			std::wstring name1 = name + L" msw";
			std::wstring name2 = name + L" lsw";

			TParam * par = getParamByName(name1);
			if (m_add_new_pars) assert(par != nullptr);
			if (NULL == par) {
				sz_log(1, "Param %s not found in configuration", SC::S2U(name1).c_str());
				return 1;
			}

			par = getParamByName(name2);
			if (m_add_new_pars) assert(par != nullptr);
			if (NULL == par) {
				sz_log(1, "Param %s not found in configuration", SC::S2U(name1).c_str());
				return 1;
			}
		}
		m_cur_par = cur_par;
		assert(m_cur_par != nullptr);
		m_cur_name = name;
	}

	double new_value = wcstof(data.c_str(), NULL);

	std::vector<DataWriter *>::iterator it = m_writers.begin();
	for (; it != m_writers.end(); it++) {
		int ret = (*it)->add_data(name, is_dbl, m_t, new_value);
		if (ret != 0)
			return 1;
	}

	// check for draw's min and max; it's strange but it works ;-)
	if (m_cur_par->GetDraws()) {
		double min = m_cur_par->GetDraws()->GetMin();
		double max = m_cur_par->GetDraws()->GetMax();
		if (new_value > max) {
			if (new_value <= 0)
				max = 0;
			else
				max = pow10(ceil(log10(new_value)));
			m_cur_par->GetDraws()->SetMax(max);
		}

		if (min > new_value) {
			if (new_value >= 0)
				min = 0;
			else
				min = -pow10(ceil(log10(-new_value)));
			m_cur_par->GetDraws()->SetMin(min);
		}

	}

	return 0;
}


int SzbaseWriter::process_line(char *line)
{
	int tokc = 0;
	char **toks;
	tokenize(line, &toks, &tokc);

	if (tokc != 7 && tokc != 8) {
		sz_log(0, "szbwriter: incorrect tokens number (%d - expected 7 or 8) in line '%s'", 
				tokc, line);
		tokenize(NULL, &toks, &tokc);
		return 1;
	}
	
	char *name = toks[0];
	int year = atoi(toks[1]);
	if (year <= 1970) {
		sz_log(0, "szbwriter: incorrect year in line '%s'", line);
		tokenize(NULL, &toks, &tokc);
		return 1;
	}
	int month = atoi(toks[2]);
	if ((month < 1) || (month > 12)) {
		sz_log(0, "szbwriter: incorrect month in line '%s'", line);
		tokenize(NULL, &toks, &tokc);
		return 1;
	}
	int day = atoi(toks[3]);
	if ((day < 1) || (day > 31)) {
		sz_log(0, "szbwriter: incorrect day in line '%s'", line);
		tokenize(NULL, &toks, &tokc);
		return 1;
	}
	int hour = atoi(toks[4]);
	if ((hour < 0) || (hour > 23)) {
		sz_log(0, "szbwriter: incorrect hour in line '%s'", line);
		tokenize(NULL, &toks, &tokc);
		return 1;
	}
	int min = atoi(toks[5]);
	if ((min < 0) || (min > 59)) {
		sz_log(0, "szbwriter: incorrect min in line '%s'", line);
		tokenize(NULL, &toks, &tokc);
		return 1;
	}
	int sec;
	char *data;
	if (tokc == 8) {
		sec = atoi(toks[6]);
		data = toks[7];
	} else {
		sec = 10;
		data = toks[6];
	}

	char *unit = check_unit(name);	

	if (add_data(SC::L2S(name), unit != NULL ? SC::L2S(unit) : L"", year, month, day, hour, min, sec, SC::L2S(data))) {
		tokenize(NULL, &toks, &tokc);
		return 1;
	}
		
	// free memory
	tokenize(NULL, &toks, &tokc);
	if (unit)
		free(unit);
		
	return 0;
}

/* Process input. */
int SzbaseWriter::process_input()
{
#define BUF_SIZE 255
	char buffer[BUF_SIZE];
	int res = 0;

	while (fgets(buffer, BUF_SIZE, stdin)) {
		buffer[BUF_SIZE-1] = 0;
		if (process_line(buffer) == 1)
			res = 1;
	}

	sz_log(9, ">>>   SzbaseWriter::process_input end while loop");

	std::vector<DataWriter *>::iterator it = m_writers.begin();
	for (; it != m_writers.end(); it++) {
		(*it)->close_data();
	}

	return res;
}

bool SzbaseWriter::have_new_params()
{
	return m_new_par;
}

/* arguments handling */
const char *argp_program_version = "szbwriter ";
const char *argp_program_bug_address = "coders@szarp.org";
static char doc[] = "SZARP batch database writer.\v\
Config file:\n\
Configuration options are read from file " SZARP_CFG ".\n\
These options are read from 'meaner3' section and are mandatory:\n\
	datadir		path to main szbase data directory\n\
	IPK		path to SZARP instance XML config file\n\
These options are read from section 'prober' and are optional:\n\
      	cachedir	path to main 10-second probes directory\n\
These options are read from section 'szbwriter' and are optional:\n\
        log             path to log file, default is " DEFAULT_LOG "\n\
        log_level       log level, from 0 to 10, default is 2.\n\
	double_match	shell pattern for names of 'combined' (two\n\
		      	words) parameters, for example:\n\
\n\
:szbwriter\n\
double_match=@(*-energy|*-volume)\n\
\n\
	See 'info fnmatch' for shell patterns' syntax.\n\
	fill_how_many	max number of 10min probes which should be fill\n\
			if gap between last two probes is lower then\n\
			value of fill_how_many, for example:\n\
:szbwriter\n\
fill_how_many=2\n\
\n\
Program read data from standard input, each input line is in format:\n\
\"<parameter name> [<unit>]\" <year> <month> <day of month> <hour> <minute> <value>\n\
\n\
For optimal performance input lines should be sorted by parameter name and\n\
then by date/time. [<unit>] is optional.\n\
";
static struct argp_option options[] = {
        {"debug", 'd', "n", 0, "Set initial debug level to n, n is from 0 \
(error only), to 10 (extreme debug), default is 2; this is overwritten by \
settings in config file"},
        {"D<param>", 0, "val", 0,
         "Set initial value of libpar variable <param> to val"},
        {"add-new-params", 'n', 0, 0,
         "Add newly found parameters to IPK configuration"},
	{"no-probes", 'p', 0, 0,
	 "Do not write 10-seconds probes in addition to 10-minutes probes"},
        {0}
};
static char args_doc[] = "[CONFIG_TITLE]\n\
CONFIG_TITLE command line argument is an optional title of newly created \
configuration, used only with -n option, when changes to IPK configuration \
are written. \
";

struct arguments
{
        bool add_new_params;
	bool no_probes;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
        struct arguments *arguments = (struct arguments *) state->input;

        switch (key) {
                case 'd':
                        break;
                case 'n':
                        arguments->add_new_params = true;
                        break;
                case 'p':
                        arguments->no_probes = true;
                        break;
        }
        return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

#define ERRXIT(msg) do{ fprintf(stderr,"%s\n",msg) ; /*return 1;*/ } while(0);

int main(int argc, char *argv[])
{
	int loglevel;
	char *ipk_path;
	char *stamp;
	char *data_dir;
	const char *cache_dir;
	const char *double_pattern;
	char *fill_how_many;
	char *fill_how_many_sec;
	int fill_how_many_num = 0;
	int fill_how_many_sec_num = 0;
	struct arguments arguments;

	loglevel = loginit_cmdline(2, NULL, &argc, argv);
	
	libpar_read_cmdline(&argc, argv);
	libpar_init_with_filename(SZARP_CFG, 1);

	char *c;

	if( !(ipk_path  = libpar_getpar(SZARP_CFG_SECTION  , "IPK"     , 1))) ERRXIT("Cannot find IPK variable");
	if( !(data_dir  = libpar_getpar(MEANER3_CFG_SECTION, "datadir" , 1))) ERRXIT("Cannot find datadir");
	if( !(cache_dir = libpar_getpar("prober"           , "cachedir", 0)))
		cache_dir = "";

	if( !(double_pattern = libpar_getpar(SZARP_CFG_SECTION, "double_match" , 0)))
		double_pattern = "";
	if( (fill_how_many      = libpar_getpar(SZARP_CFG_SECTION, "fill_how_many"    , 0)) )
		fill_how_many_num     = atoi(fill_how_many    );
	if( (fill_how_many_sec  = libpar_getpar(SZARP_CFG_SECTION, "fill_how_many_sec", 0)) )
		fill_how_many_sec_num = atoi(fill_how_many_sec);

	c = libpar_getpar(SZARP_CFG_SECTION, "log_level", 0);
	if (c == NULL)
		loglevel = 2;
	else {
		loglevel = atoi(c);
		free(c);
	}
	
	c = libpar_getpar(SZARP_CFG_SECTION, "log", 0);
	if (c == NULL)
		c = strdup(DEFAULT_LOG);
	loglevel = loginit(loglevel, c);
	if (loglevel < 0) {
		sz_log(0, "szbwriter: cannot inialize log file %s, errno %d", c, errno);
		free(c);
		return 1;
	}
	free(c);

	arguments.add_new_params = false;
	arguments.no_probes = false;
        argp_parse(&argp, argc, argv, 0, 0, &arguments);

	SzbaseWriter *szbw = new SzbaseWriter(SC::L2S(ipk_path),
			SC::L2S(double_pattern),
		       	SC::L2S(data_dir),
		       	SC::L2S(cache_dir),
			arguments.add_new_params,
		       	not arguments.no_probes,
			fill_how_many_num,
			fill_how_many_sec_num);

	assert (szbw != NULL);

#ifndef FNM_EXTMATCH
	sz_log(1, "szbwriter: extended pattern matching not available!");
#endif
	
	if (szbw->process_input())
		return 1;

	if(szbw->have_new_params())
		szbw->saveXML(SC::L2S(ipk_path));
	
	delete szbw;

	// update database stamp file
	if( (stamp  = libpar_getpar(SZARP_CFG_SECTION  , "szbase_stamp", 0)) )
		utime( stamp , NULL );

	sz_log(2, "Completed successfully");
	
	return 0;
	
}

