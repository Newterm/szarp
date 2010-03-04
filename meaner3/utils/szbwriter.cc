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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <fnmatch.h>
#include <sys/fcntl.h>
#include <ctype.h>
#include <math.h>
#include <map>
#include <sstream>
#include <tr1/unordered_map>
#include "liblog.h"
#include "libpar.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "tokens.h"

#include "conversion.h"

#define SZARP_CFG_SECTION	"szbwriter"
#define MEANER3_CFG_SECTION	"meaner3"
#define SZARP_CFG		"/etc/szarp/szarp.cfg"

using namespace std::tr1;

class SzbaseWriter : public TSzarpConfig {
public:
	SzbaseWriter(const std::wstring &ipk_path, const std::wstring& title, const std::wstring &double_pattern,
			const std::wstring& data_dir, int fill_how_many);
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
			int hour, int min, const std::wstring &data);
	/** Process sigle input line.
	 * @return 1 on error, 0 on success*/
	int process_line(char *line);

	/** Saves data for current index.
	 * @return 0 on success, 1 on error */
	int save_data();
	
	/** Calls save_data(), closes opened files. 
	 * @return 0 on success, 1 on error (from save_data) */
	int close_data();

	/** Checks if param should be saved in two words.
	 * @param name name of param
	 * @return 1 for double params, 0 otherwise */
	int is_double(const std::wstring &name);

	std::wstring m_double_pattern;	/**< shell pattern for names of params
					  that should be saved in two words,
					  may be NULL */

	std::wstring m_dir;			/**< szbase data directory */

	TParam *m_cur_par;		/**< pointer to currently saved param */
	std::wstring m_cur_filename;		/**< name of currently opened file;
					(virtual - real names may be different
					becasue of low/high words) */
	int m_cur_fd;			/**< currently opened file descriptor */
	int m_cur_fd1;			/**< file descriptor for second word (-1 for
					non-double parameters) */
	int m_cur_ind;			/**< current index in file */
	double m_cur_sum;		/**< sum of values to save */
	int m_cur_cnt;			/**< count of values to save */

	int m_fill_how_many;		/** how many previous empty fields should 
					  we fill with current value during saving */
	unordered_map<std::wstring, int> m_draws_count;
	bool m_new_par; /** if new parameter was added */
};

SzbaseWriter::SzbaseWriter(const std::wstring &ipk_path, const std::wstring& _title, const std::wstring& double_pattern,
		const std::wstring& data_dir, int fill_how_many)
{
	m_double_pattern = double_pattern;
	m_dir = data_dir;
	
	m_cur_par = NULL;
	m_cur_fd = -1;
	m_cur_fd1 = -1;
	m_cur_ind = -1;
	m_cur_cnt = 0;
	m_cur_sum = 0;
	m_fill_how_many = fill_how_many;
	sz_log(10, "Parameter fill_how_many set to: %d", fill_how_many);
	m_new_par=false;

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

	m_new_par=true;
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
	
	p = getParamByName(name);
	
	if (p != NULL) {
		m_cur_par = p;
		return p->GetPrec();
	}
	
	sz_log(2, "Adding new parameter %ls\n", name.c_str());
	m_new_par=true;

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
		int hour, int min, const std::wstring& data)
{
	std::wstring filename;
	int index;
	struct tm tm;
	time_t t;
	int is_dbl;

	/* get UTC time */
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = 0;
	tm.tm_isdst = -1;
	
	t = mktime(&tm);

	gmtime_r(&t, &tm);
	year = tm.tm_year + 1900;
	month = tm.tm_mon + 1;
	
	filename = szb_createfilename(name, year, month);
	index = szb_probeind(t);
	is_dbl = is_double(name);
	
	if (m_cur_ind >= 0) {
		/* save previous data */
		if ((m_cur_filename != filename) || 
				(index >= szb_probecnt(year, month))) {
			if (close_data())
				return 1;
		} else if (m_cur_ind != index) {
			if (save_data())
				return 1;
		}
	}
	if (m_cur_ind < 0) {
		/* open new file */
		m_cur_filename = filename;
		if (is_dbl) {
			m_cur_fd = szb_open(m_dir, name + L" msw", year, month,
					O_CREAT | O_RDWR);
			if (m_cur_fd == -1) {
				sz_log(0, "Error opening file for '%ls', errno %d",
						(name + L" msw").c_str(), errno);
				return 1;
			}
			m_cur_fd1 = szb_open(m_dir, name + L" lsw", year, month,
					O_CREAT | O_RDWR);
			if (m_cur_fd1 == -1) {
				sz_log(0, "Error opening file for '%ls', errno %d",
						(name + L" lsw").c_str(), errno);
				return 1;
			}
			m_cur_ind = 0;
		} else {
			m_cur_fd = szb_open(m_dir, name, year, month,
					O_CREAT | O_RDWR);
			if (m_cur_fd == -1) {
				sz_log(0, "Error opening file for '%ls', errno %d",
						name.c_str(), errno);
				return 1;
			}
			m_cur_ind = 0;
		}
	}

	int prec;
	if (is_dbl)
		prec = 0;
	else
		prec = guess_prec(data);
	prec = add_param(name, unit, prec);

	m_cur_ind = index;
	m_cur_sum += wcstof(data.c_str(), NULL);
	m_cur_cnt++;
	
	return 0;
}

int SzbaseWriter::save_data()
{
	long pos, pos1 = -1;
	short data = SZB_FILE_NODATA;
	short data1;
	double d;

	/* no files opened, return */
	if (m_cur_fd == -1)
		return 0;
	
	/* no data to save, return */
	if (m_cur_cnt <= 0)
		return 0;
	
	/* get current file position */
	pos = lseek(m_cur_fd, 0, SEEK_END);
	pos /= sizeof(data);
	if (pos > m_cur_ind) {
		sz_log(3, "Index before current position (input not sorted?) (%ls)",
				m_cur_filename.c_str());
		pos = lseek(m_cur_fd, m_cur_ind * sizeof(data), SEEK_SET);
		pos /= sizeof(data);
	}
	
	if (m_cur_fd1 != -1) {
		pos1 = lseek(m_cur_fd1, 0, SEEK_END);
		pos1 /= sizeof(data);
		if (pos1 > m_cur_ind) {
			sz_log(3, "Index 2 before current position (input not sorted?) (%ls)",
					m_cur_filename.c_str());
			pos1 = lseek(m_cur_fd1, m_cur_ind * sizeof(data), SEEK_SET);
			pos1 /= sizeof(data);
		}
	}

	/* what data to save ? */
	d = m_cur_sum / m_cur_cnt;
	m_cur_sum = 0;
	m_cur_cnt = 0;

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

	for (int i = 0; i < m_cur_par->GetPrec(); i++) {
		d *= 10;
	}
	
	if (m_cur_fd1 == -1) {
		/* one file */
		data = (short)rint(d);
	} else {
		/* two files */
		long ldata;
		ldata = (long)rint(d);
		data1 = (short)(ldata & 0xFFFF);
		data = (short) ((ldata >> 16) & 0XFFFF);
	}

	/* fill in empty fields */
	short filldata, filldata1;
	filldata = filldata1 = SZB_FILE_NODATA;
	while (pos++ < m_cur_ind) {
		if (pos - 1 + m_fill_how_many >= m_cur_ind) {
			filldata = data;
		}
		if (write(m_cur_fd, &filldata, sizeof(data)) != sizeof(data)) {
			sz_log(0, "Write error for '%ls', errno %d",
					m_cur_filename.c_str(), errno);
			return 1;
		}
	}
	if (m_cur_fd1 != -1) while (pos1++ < m_cur_ind) {
		if (pos1 - 1 + m_fill_how_many >= m_cur_ind) {
			filldata1 = data1;
		}
		if (write(m_cur_fd1, &filldata1, sizeof(data)) != sizeof(data)) {
			sz_log(0, "Write error for '%ls' (lsw), errno %d",
					m_cur_filename.c_str(), errno);
			return 1;
		}
	}
	
	
	/* write value to first file */
	if (write(m_cur_fd, &data, sizeof(data)) != sizeof(data)) {
			sz_log(0, "Write error for '%ls', errno %d",
					m_cur_filename.c_str(), errno);
			return 1;
	}
		
	if (m_cur_fd1 == -1)
		return 0;

	/* write value for second file */
	if (write(m_cur_fd1, &data1, sizeof(data1)) != sizeof(data1)) {
			sz_log(0, "Write error for '%ls' (lsw), errno %d",
					m_cur_filename.c_str(), errno);
			return 1;
	}

	return 0;
}


int SzbaseWriter::close_data()
{
	if (m_cur_fd == -1)
		return 0;
	if (save_data())
		return 1;
	close(m_cur_fd);
	
	m_cur_fd = -1;
	m_cur_ind = -1;
	m_cur_filename = L"";
	if (m_cur_fd1 == -1)
		return 0;
	close(m_cur_fd1);
	m_cur_fd1 = -1;
	return 0;
}

int SzbaseWriter::process_line(char *line)
{
	int tokc = 0;
	char **toks;
	tokenize(line, &toks, &tokc);

	if (tokc != 7) {
		sz_log(0, "szbwriter: incorrect tokens number (%d - expected 7) in line '%s'", 
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
	char *data = toks[6];

	char *unit = check_unit(name);	

	if (add_data(SC::A2S(name), unit != NULL ? SC::A2S(unit) : L"", year, month, day, hour, min, SC::A2S(data))) {
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

int main(int argc, char *argv[])
{
	int loglevel;
	char *ipk_path;
	char *data_dir;
	char *double_pattern;
	int fill_how_many;

	loglevel = loginit_cmdline(2, NULL, &argc, argv);
	
	libpar_read_cmdline(&argc, argv);
	libpar_init_with_filename(SZARP_CFG, 1);

	char *c;

	ipk_path = libpar_getpar(SZARP_CFG_SECTION, "IPK", 1);
	data_dir = libpar_getpar(MEANER3_CFG_SECTION, "datadir", 1);

	double_pattern = libpar_getpar(SZARP_CFG_SECTION, "double_match", 0);

	c = libpar_getpar(SZARP_CFG_SECTION, "fill_how_many", 0);
	if (c) {
		fill_how_many = atoi(c);
		free(c);
	} else
		fill_how_many = 0;

	c = libpar_getpar(SZARP_CFG_SECTION, "log_level", 0);
	if (c == NULL)
		loglevel = 2;
	else {
		loglevel = atoi(c);
		free(c);
	}
	
	c = libpar_getpar(SZARP_CFG_SECTION, "log", 0);
	if (c == NULL)
		c = strdup(PREFIX"/logs/szbwriter");
	loglevel = loginit(loglevel, c);
	if (loglevel < 0) {
		sz_log(0, "szbwriter: cannot inialize log file %s, errno %d", c, errno);
		free(c);
		return 1;
	}
	free(c);

	const char *title;
	if (argc <= 1) {
		title = "Wêz³y Samson";
	} else {
		title = argv[1];
	}
	SzbaseWriter *szbw = new SzbaseWriter(SC::A2S(ipk_path), SC::A2S(title), 
			double_pattern != NULL ? SC::A2S(double_pattern) : L"",
			SC::A2S(data_dir), fill_how_many);
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
