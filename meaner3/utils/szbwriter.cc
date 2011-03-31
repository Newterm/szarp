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

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
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

#include "liblog.h"
#include "libpar.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "tokens.h"
#include "tsaveparam.h"

#include "conversion.h"

#define SZARP_CFG_SECTION	"szbwriter"
#define MEANER3_CFG_SECTION	"meaner3"
#define SZARP_CFG		"/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg"
#define DEFAULT_LOG		PREFIX"/logs/szbwriter.log"

using namespace std::tr1;

class SzbaseWriter : public TSzarpConfig {
public:
	SzbaseWriter(const std::wstring &ipk_path, const std::wstring& title, const std::wstring &double_pattern,
			const std::wstring& data_dir, const std::wstring &cache_dir, bool add_new_pars,
			bool write_10sec, int _fill_how_many);
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

	enum PROBE_TYPE { MIN10 = 0, SEC10, LAST_PROBE_TYPE };

	/** Saves data for current index.
	 * @return 0 on success, 1 on error */
	int save_data(PROBE_TYPE pt);

	/** Fill gaps in data. Range: [begin, end)
	 * @return 0 on success, 1 on error */
	int fill_gaps(PROBE_TYPE pt, int begin, int end, double sum, int count);

	int close_data();
	
	/** Checks if param should be saved in two words.
	 * @param name name of param
	 * @return 1 for double params, 0 otherwise */
	int is_double(const std::wstring &name);

	std::wstring m_double_pattern;	/**< shell pattern for names of params
					  that should be saved in two words,
					  may be NULL */

	std::vector<std::wstring> m_dir;	/**< szbase data directory */

	TParam *m_cur_par;		/**< pointer to currently saved param */
	TSaveParam* m_save_param[LAST_PROBE_TYPE][2];
	std::wstring m_cur_name;		/**< name of currently opened file;
					(virtual - real names may be different
					becasue of low/high words) */
	int m_cur_t[LAST_PROBE_TYPE];		/**< current time*/
	double m_cur_sum[LAST_PROBE_TYPE];	/**< sum of values to save */
	int m_cur_cnt[LAST_PROBE_TYPE];		/**< count of values to save */
	int m_probe_length[LAST_PROBE_TYPE];		/**< lenght of probe */

	unordered_map<std::wstring, int> m_draws_count;
	bool m_add_new_pars; 	/** flag denoting if we add new pars*/
	bool m_new_par; 	/** if new parameter was added */
	size_t m_last_type;	/** last type of probe to write + 1 */

	int m_fill_how_many;	/** fill gaps in data when distance between two probes is <=  m_fill_how_many **/
};

SzbaseWriter::SzbaseWriter(const std::wstring &ipk_path, const std::wstring& _title, 
		const std::wstring& double_pattern,
		const std::wstring& data_dir, const std::wstring& cache_dir, 
		bool add_new_pars,
		bool write_10sec,
		int _fill_how_many):
	m_double_pattern(double_pattern),
	m_add_new_pars(add_new_pars),
	m_last_type(write_10sec ? LAST_PROBE_TYPE : SEC10),
	m_fill_how_many(_fill_how_many)
{
	m_dir.push_back(data_dir);
	m_dir.push_back(cache_dir);
	m_cur_par = NULL;
	for (size_t i = 0; i < m_last_type; i++) {
		m_save_param[i][0] = 
			m_save_param[i][1] = NULL;
		m_cur_t[i] = 0;
		m_cur_cnt[i] = 0;
		m_cur_sum[i] = 0;
	}
	m_probe_length[0] = 60 * 10;
	m_probe_length[1] = 10;
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
	title = _title;
	AddDevice(new TDevice(this, L"/bin/true", L"fake"));
	GetFirstDevice()->AddRadio(new TRadio(devices));
	GetFirstDevice()->GetFirstRadio()->AddUnit(new TUnit(
				GetFirstDevice()->GetFirstRadio(), 'x'));
	TParam* p = new TParam(GetFirstDevice()->
			GetFirstRadio()->GetFirstUnit());
	GetFirstDevice()->GetFirstRadio()->GetFirstUnit()->AddParam(p);
	
	p->SetFormula(L"fake");
	p->Configure(L"fake:fake:fake", L"fake", L"fake"
			, NULL, 0);
	
	p->SetAutoBase();

	m_new_par = true;
}

SzbaseWriter::~SzbaseWriter()
{
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
	
	sz_log(2, "Adding new parameter %ls\n", name.c_str());
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
	sz_log(10,"add_data begin: name=%s, data=%s %d-%d-%d %d:%d:%d",SC::S2A(name).c_str(),SC::S2A(data).c_str(),year,month,day,hour,min,sec);

	std::wstring filename;
	struct tm tm;
	time_t t;
	int is_dbl;

	/* get UTC time */
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;
	tm.tm_isdst = -1;
	
	t = mktime(&tm);

	gmtime_r(&t, &tm);
	year = tm.tm_year + 1900;
	month = tm.tm_mon + 1;
	
	is_dbl = is_double(name);
	TParam *par = NULL, *par2 = NULL;
	int prec;

	sz_log(10,"add_data: name=%s, m_cur_name=%s",SC::S2A(name).c_str(),SC::S2A(m_cur_name).c_str());
	
	if (name != m_cur_name) {
		for (size_t i = 0; i < m_last_type; i++) {
			if (save_data((PROBE_TYPE)i))
				return 1;
			for (size_t j = 0; j < 2; j++) {
				delete m_save_param[i][j];
				m_save_param[i][j] = NULL;
			}
		}
		TParam* cur_par = getParamByName(name);
		if (cur_par == NULL) {
			if (!m_add_new_pars) {
				sz_log(1, "Param %ls not found in configuration and "
					"program run without -n flag, value ignored!",
					name.c_str());
				return 1;
			}
			m_cur_par = getParamByName(name);
			if (m_cur_par == NULL) {
				prec = add_param(name, unit, guess_prec(data));
				m_cur_par = getParamByName(name);
			} else {
				prec = m_cur_par->GetPrec();
			}
			prec = add_param(name, unit, guess_prec(data));
			cur_par = getParamByName(name);
		} else {
			prec = cur_par->GetPrec();
		}
		m_cur_par = cur_par;
		if (is_dbl) {
			std::wstring name1 = name + L" msw";
			std::wstring name2 = name + L" lsw";
			par = getParamByName(name1);
			par2 = getParamByName(name2);
			assert(par != NULL);
			assert(par2 != NULL);
		} else {
			par = m_cur_par;
			assert(par != NULL);
		}
		for (size_t i = 0; i < m_last_type; i++) {
			if (m_dir.at(i).empty())
				continue;
			m_save_param[i][0] = new TSaveParam(par);
			if (is_dbl)
				m_save_param[i][1] = new TSaveParam(par2);
			m_cur_sum[i] = 0;
			m_cur_cnt[i] = 0;
			sz_log(10,"add_data: (name != n_cur_name) i=%d, t = %ld",i,t);
			m_cur_t[i] = t / m_probe_length[i] * m_probe_length[i];
			sz_log(10,"add_data: (name != n_cur_name) i=%d, m_cur_t[i] = %d",i,m_cur_t[i]);
		}
		m_cur_name = name;
	}

	for (size_t i = 0; i < m_last_type; i++) {

		sz_log(10,"add_data: i=%d, t=%ld, m_cur_t[i]=%d, t-m_cur=%ld, m_len=%d",i,t,m_cur_t[i],t-m_cur_t[i],m_probe_length[i]);

		if (m_dir.at(i).empty())
			continue;
		if (t >= m_cur_t[i] && t - m_cur_t[i] < m_probe_length[i]) 
			continue;

		// save data for gaps
		int _time = m_cur_t[i];
		double _sum = m_cur_sum[i];
		int _count = m_cur_cnt[i];

		if (save_data((PROBE_TYPE)i))
			return 1;

		sz_log(10,"add_data: i=%d,  t = %ld",i,t);
		m_cur_t[i] = t / m_probe_length[i] * m_probe_length[i];
		sz_log(10,"add_data: i=%d,  m_cur_t[i] = %d",i,m_cur_t[i]);

		int gap = m_cur_t[i] - _time;
		sz_log(10,"add_data: i=%d, check time gap: %d",i,gap);
		assert(gap >= 0 && gap >= m_probe_length[i]);

		if ((PROBE_TYPE) i == MIN10 && m_fill_how_many > 0 && m_probe_length[i] < gap && gap <= (m_fill_how_many + 1) * m_probe_length[i])
		{
			if(fill_gaps((PROBE_TYPE)i, _time + m_probe_length[i] , _time + gap ,_sum,_count))
				return 1;
		}
	}

	for (size_t i = 0; i < m_last_type; i++) {
		m_cur_sum[i] += wcstof(data.c_str(), NULL);
		m_cur_cnt[i]++;
	}

	sz_log(10,"add_data end: t[0]=%d, t[1]=%d, sum[0]=%lf, sum[1]=%lf, cnt[0]=%d, cnt[1]=%d, len[0]=%d, len[1]=%d",
		m_cur_t[0],m_cur_t[1],m_cur_sum[0],m_cur_sum[1],m_cur_cnt[0],m_cur_cnt[1],m_probe_length[0],m_probe_length[1]);
	
	return 0;
}

int SzbaseWriter::save_data(PROBE_TYPE pt)
{
	double d;

	if (m_dir[pt].empty())
		return 0;

	sz_log(10,"save_data begin: pt=%d, m_dir[pt]=%s",(int) pt, SC::S2A(m_dir[pt]).c_str());
	sz_log(10,"save_data %s",!m_cur_cnt[pt] ? "end - NO DATA" : "data exists");

	if (!m_cur_cnt[pt])
		return 0;

	d = m_cur_sum[pt] / m_cur_cnt[pt];

	sz_log(10,"save_data: current sum = %lf, current count =%d, current time=%d, value=%G",m_cur_sum[pt],m_cur_cnt[pt],m_cur_t[pt],d);

	if (m_save_param[pt][1]) {
		int v = rint(pow10(m_cur_par->GetPrec()) * d);
		unsigned *pv = (unsigned*) &v;
		short v1 = *pv >> 16, v2 = *pv & 0xffff;
		if (m_save_param[pt][0]->WriteBuffered(m_dir[pt], m_cur_t[pt], &v1, 1, NULL, 1, 0, m_probe_length[pt]))
			return 1;
		if (m_save_param[pt][1]->WriteBuffered(m_dir[pt], m_cur_t[pt], &v2, 1, NULL, 1, 0, m_probe_length[pt]))
			return 1;
	} else {
		short v = rint(pow10(m_cur_par->GetPrec()) * d);
		if (m_save_param[pt][0]->WriteBuffered(m_dir[pt], m_cur_t[pt], &v, 1, NULL, 1, 0, m_probe_length[pt]))
			return 1;
	}

	/* check for draw's min and max; it's strange but it works ;-) */
	if (m_cur_par->GetDraws()) {
		double min = m_cur_par->GetDraws()->GetMin();
		double max = m_cur_par->GetDraws()->GetMax();
		if (d > max) {
			if (d <= 0)
				max = 0;
			else
				max = pow10(ceil(log10(d)));
			m_cur_par->GetDraws()->SetMax(max);
		}

		if (min > d) {
			if (d >= 0)
				min = 0;
			else
				min = -pow10(ceil(log10(-d)));
			m_cur_par->GetDraws()->SetMin(min);
		}

	}
	m_cur_cnt[pt] = 0;
	m_cur_sum[pt] = 0;

	sz_log(10,"save_data end - cnt[pt] = 0, sum[pt]= 0");

	return 0;
}

int SzbaseWriter::fill_gaps(PROBE_TYPE pt, int begin, int end, double sum, int count) 
{
	if (m_fill_how_many <= 0)
		return 0;

	assert(pt == MIN10 || pt == SEC10);

	sz_log(10,"fill_gaps begin, pt=%d",(int)pt);

	// save current data
	int save_time = m_cur_t[pt];
	int save_cnt = m_cur_cnt[pt];
	double save_sum = m_cur_sum[pt];

	// fill gaps
	for (int t = begin; t < end; t += m_probe_length[pt])
	{
		sz_log(10,"fill gap for t=%d",t);

		m_cur_t[pt] = t;
		m_cur_cnt[pt] = count;
		m_cur_sum[pt] = sum;
		
		if (save_data(pt))
			return 1;
	}

	// restore data
	m_cur_t[pt] = save_time;
	m_cur_cnt[pt] = save_cnt;
	m_cur_sum[pt] = save_sum;

	// call fill_gaps for 10sec
	if (pt == MIN10 && m_last_type > SEC10)
		if (fill_gaps(SEC10,begin,end,sum,count))
			return 1;

	sz_log(10,"fill_gaps end, pt=%d", (int)pt);
	return 0;
}

int SzbaseWriter::close_data()
{
	for (size_t i = 0; i < m_last_type; i++)
		if (save_data((PROBE_TYPE)i))
			return 1;
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

	if (add_data(SC::A2S(name), unit != NULL ? SC::A2S(unit) : L"", year, month, day, hour, min, sec, SC::A2S(data))) {
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
		if(process_line(buffer)==1)
			res=1;
	}
	close_data();
	return res;
}

bool SzbaseWriter::have_new_params()
{
	return m_new_par;
}

/* arguments handling */
const char *argp_program_version = "szbwriter " VERSION;
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
	std::wstring title;
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
		case ARGP_KEY_ARG:
			if (state->arg_num >= 2)
				return ARGP_ERR_UNKNOWN;
			arguments->title = SC::A2S(arg);
			break;
                default:
			if (state->arg_num >= 2)
				return ARGP_ERR_UNKNOWN;
			break;
        }
        return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[])
{
	int loglevel;
	char *ipk_path;
	char *data_dir;
	char *cache_dir;
	char *double_pattern;
	char *fill_how_many;
	struct arguments arguments;

	loglevel = loginit_cmdline(2, NULL, &argc, argv);
	
	libpar_read_cmdline(&argc, argv);
	libpar_init_with_filename(SZARP_CFG, 1);

	char *c;

	ipk_path = libpar_getpar(SZARP_CFG_SECTION, "IPK", 1);
	data_dir = libpar_getpar(MEANER3_CFG_SECTION, "datadir", 1);
	cache_dir = libpar_getpar("prober", "cachedir", 0);

	double_pattern = libpar_getpar(SZARP_CFG_SECTION, "double_match", 0);
	fill_how_many = libpar_getpar(SZARP_CFG_SECTION, "fill_how_many", 0);

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
	if (arguments.title.empty()) {
		arguments.title = SC::A2S("Wêz³y Samson");
	}

	SzbaseWriter *szbw = new SzbaseWriter(SC::A2S(ipk_path), arguments.title, 
			double_pattern != NULL ? SC::A2S(double_pattern) : L"",
			SC::A2S(data_dir), cache_dir ? SC::A2S(cache_dir) : std::wstring(),
			arguments.add_new_params, not arguments.no_probes,
			atoi(fill_how_many));
	assert (szbw != NULL);

#ifndef FNM_EXTMATCH
	sz_log(1, "szbwriter: extended pattern matching not available!");
#endif
	
	if (szbw->process_input())
		return 1;

	if(szbw->have_new_params())
		szbw->saveXML(SC::A2S(ipk_path));
	
	sz_log(2, "Completed successfully");
	
	delete szbw;

	return 0;
	
}
