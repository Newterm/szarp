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
 * SZARP

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Finds and eliminates peaks in szarp base files. See --help for options.
 *
 * Algorithm description:
 * - Constant size window is used to analyze data
 * - Searching starts from end of base, window is initialized with first sequence 
 * without no-data values
 * - Program moves window left in loop and counts delta - difference beetween 2 
 * values (absolute value).
 * - If delta is larger then PEAK_DELTA_FRACT * average_delta, the peak is found.
 * - Otherwise, if delta is larger then average_delta, average_delta is modified 
 * to contain delta (so, average_delta is counted from deltas larger then previous
 * average delta...)
 * - PEAK_DELTA_FRACT default value is 10.0, it can be modified from command line.
 * - If peak is found, program searches current window for nearest 'good' value -
 * within +/-delta range from current; if 'good' value is found, non-empty values 
 * between good and current can be interpolated or reset to no-data.
 */
 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "liblog.h"
#include "libpar.h"
#include "szarp_config.h"
#include "ipkcontainer.h"
#include "szbase/szbbase.h"
#include "tsaveparam.h"
#include "conversion.h"

#include <iostream>
#include <vector>

using namespace std;


class CycleWindow {
protected:
	enum { WINDOW_SIZE = 12 };
	SZBASE_TYPE window[WINDOW_SIZE];
	int data_count;
	int index;
public:
	CycleWindow();
	size_t Size() { return WINDOW_SIZE; }
	size_t Count() { return data_count; }
	int Full() { return Size() == Count(); }
	void Push(SZBASE_TYPE val);
	SZBASE_TYPE& operator[] (size_t i);
};

class SzbException : public std::exception {
private:
	std::string m_cause;
public: 
	SzbException(const std::string& cause);
	virtual const char* what() const throw();
	virtual ~SzbException() throw() {};
};

CycleWindow::CycleWindow()
	: data_count(0), index(0)
{
	assert (data_count == 0);
	for (size_t i = 0; i < WINDOW_SIZE; i++)
		window[i] = SZB_NODATA;
}

SZBASE_TYPE& CycleWindow::operator[] (size_t i)
{
	if (((int)i >= WINDOW_SIZE) || ((int)i < 0))
		throw SzbException("Index out of range.");
	return window[(index+i) % WINDOW_SIZE];
}

void CycleWindow::Push(SZBASE_TYPE val)
{
	if (IS_SZB_NODATA(val)) {
		if (!IS_SZB_NODATA(window[index])) {
			assert (data_count > 0);
			data_count--;
		}
	} else {
		if (IS_SZB_NODATA(window[index])) {
			data_count++;
		}
	}
	window[index] = val;
	index = (index + 1) % WINDOW_SIZE;
}


class PeakEliminator
{
protected:
	double PEAK_DELTA_FACT;		/**< delta factor */
	CycleWindow window;		/**< buffer for data */
	szb_buffer_t* buf;		/**< pointer to szbase buffer */
	char last_answer;		/**< last answer provided by user */
	int fix_all;			/**< 1 if user want to fix all data */
private:
	SZBASE_TYPE delta_sum;		/**< sum of all deltas */
	size_t delta_count;		/**< count of deltas */
	SZBASE_TYPE delta_avg;		/**< delta_sum / delta_count */
public:
	PeakEliminator(TSzarpConfig* ipk, const char* dir, double delta);
	~PeakEliminator();
	/**
	 * Main method - check parameter data for peaks.
	 */
	void CheckParam(TParam *p);
protected:
	/**
	 * Start scanning of parameter - finds last window.Size() continous 
	 * non-empty values.
	 * @param first first data for param
	 * @param last last data for param
	 * @return date of end of sequence found
	 */
	time_t StartRun(TParam *p, time_t first, time_t last);
	/**
	 * Tries to fix data contained in window.
	 * @param window_last date of last value in window
	 * @return date of 
	 */
	time_t Fix(TParam *p, time_t window_last);
	/**
	 * Asks user what to do. 
	 * @return 0 - ignore; 1 - save buffer, 2 - save no data 
	 */
	int GetInput();
	/**
	 * Print buffer in color.
	 * @param upto 0 for no-color, index of first no-colored value
	 * @param red 1 for red, 0 for green
	 */
	void PrintBuffer(int upto, int red);
	/**
	 * Saves buffer content starting from date t.
	 */
	void SaveWindow(TParam *p, time_t t);
};

PeakEliminator::PeakEliminator(TSzarpConfig* ipk, const char* dir, double delta)
{
	ParamCachingIPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"");
	Szbase::Init(SC::L2S(PREFIX), NULL);
	buf = szb_create_buffer(Szbase::GetObject(), SC::L2S(dir), 1, ipk);
	if (buf == NULL) {
		throw SzbException("Error creating szbase buffer");
	}
	last_answer = 'y';
	fix_all = 0;
	PEAK_DELTA_FACT = delta;
	assert (PEAK_DELTA_FACT >= 2.0);
}

PeakEliminator::~PeakEliminator()
{
	szb_free_buffer(buf);
	buf = NULL;
}

time_t PeakEliminator::StartRun(TParam *p, time_t first, time_t last)
{
	assert (last > first);
	for (time_t t = last; t >= first; t -= SZBASE_DATA_SPAN) {
		window.Push(szb_get_probe(buf, p, t, PT_MIN10, 0));
		if (window.Full())
			return t;
	}
	return (time_t) -1;
}

void PeakEliminator::PrintBuffer(int upto, int red)
{
	cout << " [ ";
	if (upto <= 1) {
		for (int i = (int) window.Size() - 1 ; i >= 0; i--) {
			cout << window[i] << ", ";
		}
	} else {
		const char *color;
		if (red) {
			color = "\033[01;31m";
		} else {
			color = "\033[0;32m";
		}
		for (int i = (int) window.Size() -1 ; i >= 0; i--) {
			if ((i >= 1) && (i < (int)upto) && !IS_SZB_NODATA(window[i])) {
				cout << color << window[i] << "\033[00m, ";
			} else {
				cout << window[i] << ", ";
			}
		}
	}
	cout << "]\n";
}

time_t PeakEliminator::Fix(TParam* p, time_t window_last)
{
	int ret;

	cout << "Buffer content: ";

	if (window.Count() <= 2) {
		PrintBuffer(0, 0);
		cout << "Cannot fix, only to points available!\n\n";
		return window_last;
	}
	size_t good = 2;
	while ((good < window.Size()) &&
			(IS_SZB_NODATA(window[good])
			|| (fabs(window[good] - window[0]) > PEAK_DELTA_FACT * delta_avg))) {
		good++;
	}
	PrintBuffer(good, 1);
	if (good >= window.Size()) {
		cout << "Cannot fix, no correct data found in buffer!\n\n";
		return window_last;
	}
	
	/* Fix data in window */
	SZBASE_TYPE diff = window[good] - window[0];
	for (size_t i = 1; i < good; i++) {
		if (!IS_SZB_NODATA(window[i])) {
			window[i] = window[0] + i * (diff / good);
		}
	}
	cout << "Modified buffer:";
	PrintBuffer(good, 0);
	cout << "\n";

	if ((ret =GetInput()) > 0) {
		if (ret == 2) {
			for (size_t i = 1; i < good; i++) {
				window[i] = SZB_NODATA;
			}
		}
		SaveWindow(p, window_last);
	}

	/* Move time and window */
	for (size_t i = 0; i < good; i++) {
		window_last -= SZBASE_DATA_SPAN;
		window.Push(szb_get_probe(buf, p, window_last, PT_MIN10, 0));
	}
	return window_last;
}


int PeakEliminator::GetInput()
{
	if (fix_all) {
		return 1;
	}
	while (1) {
		cout << "Modify? (y)es/(A)ll/set (n)o data/(i)gnore/(q)uit: [" << last_answer << "]: ";
		cout.flush();
		char tmp;
		cin.get(tmp);
		char prev = last_answer;
		if (tmp == '\n') {
			tmp = last_answer;
		} else {
			last_answer = tmp;
			char c;
			while (cin.get(c) && c != '\n');
		}
		switch (tmp) {
			case 'A':
				fix_all = 1;
			case 'y':
				return 1;
				break;
			case 'n':
				return 2;
				break;
			case 'i' :
				return 0;
				break;
			case 'q':
				exit(1);
				break;
			default:
				last_answer = prev;
				cout << "\n";
				break;
		}
	}
}

void PeakEliminator::SaveWindow(TParam* p, time_t t)
{
	TSaveParam sp(p);
	double pw = pow(10.0, p->GetPrec());
	for (size_t i = window.Size() - 1; i >= 1; i--, t+= SZBASE_DATA_SPAN) {
		sp.Write(
				buf->rootdir.c_str(), 
				t, 
				(SZB_FILE_TYPE)round((window[i] * pw)), 
				NULL, /* no status info */
				1, /* overwrite */
				1 /* force_nodata */
			);
	}
}

void PeakEliminator::CheckParam(TParam *p)
{
	time_t first = szb_search_first(buf, p);
	time_t last = szb_search_last(buf, p);
	SZBASE_TYPE delta_sum = 0;
	size_t delta_count = 0;

	wcout.precision(p->GetPrec());
	wcout << "Checking parameter: \033[0;32m" << p->GetName() << "\033[0;30m\n\n";
	time_t t = StartRun(p, first, last);
	if (t == (time_t)-1) {
		wcout << "No data found for parameter '" << p->GetName() << "'\n";
		return;
	}
	for (size_t i = 0; i < window.Count() - 1; i++) {
		delta_sum += fabs(window[i] - window[i+1]);
		delta_count++;
	}
	delta_avg = delta_sum / delta_count;
	while (t >= first) {
		t -= SZBASE_DATA_SPAN;
		SZBASE_TYPE val = szb_get_probe(buf, p, t, PT_MIN10, 0);
		window.Push(val);
		if (IS_SZB_NODATA(window[0])) {
			continue;
		}
		size_t check = 1;
		while ((check < window.Size()) && IS_SZB_NODATA(window[check])) {
			check++;
		}
		if (check >= window.Size()) {
			continue;
		}

		//PrintBuffer(0, 0);
		SZBASE_TYPE delta = fabs(window[check] - window[0]);
		//cout << "DELTA: " << delta << " AVG: " << delta_avg << "\n";
		if ((delta_sum > 0) && (delta > PEAK_DELTA_FACT * delta_avg)) {
			time_t tmp = t + (window.Size()) * SZBASE_DATA_SPAN;
			char date_buf[100];
			strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M", localtime(&tmp));

			cout << "Something found before " << date_buf << "\n";
			cout << "Value is " << window[0] << ", previous is " << window[check] <<
				", average delta is " << delta_avg << "\n";
			t = Fix(p, t);
		} else if (delta > delta_avg) {
			delta_count++;
			delta_sum += delta;
			delta_avg = delta_sum / delta_count;
		}
	}
}

#include <argp.h>

vector<wstring> params;

/** Append file name to vector of files to process */
int add_param(char* param)
{
	params.push_back(SC::L2S(param));
	return 0;
}

struct arguments {
	double delta_fract;	/**< delta fraction */
} arguments = { 10.0 };

/** argp documentation info */
const char *argp_program_version = "peakator ""$Revision: 6750 $";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "Finds and removes peaks from szarp base files.\v\
Use libpar -Dprefix=PREFIX arguments to set configuration and base.\n\
";
static char args_doc[] = "[ PARAMETER NAME ] ...";
static struct argp_option options[] = {
	{ "delta", 'd', "DELTA", 0, "Set maximum allowed difference from averaged delta, \
as average delta multiplicity; default is 10.0", 1 },
	{ 0 }
};

/** argp parser function
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
        struct arguments *arguments = (struct arguments *) state->input;
	switch (key) {
		case 'd' : 
			char *end;
			arguments->delta_fract = strtod(arg, &end);
			if (!*arg || *end) {
				cerr << "Invalid DELTA!\n";
				argp_usage (state);
			}
			if (arguments->delta_fract < 2.0) {
				cerr << "DELTA to small - must be at least 2.0!\n";
				argp_usage (state);
			}
			break;
		case ARGP_KEY_ARG :
			if (add_param(arg))
				return EINVAL;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1)
				/* Not enough arguments. */
				argp_usage (state);
			break;
		default :
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/** argp parameters */
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char** argv)
{
	log_info(0);
	libpar_read_cmdline(&argc, argv);
	libpar_init();

	int err = argp_parse (&argp, argc, argv, 0, 0, &arguments);
	if (err)
		return err;
	
	char *path = libpar_getpar("", "IPK", 0);
	if (path == NULL) {
		cerr << "'IPK' not found in szarp.cfg\n";
		return 1;
	}
	char *dir = libpar_getpar("", "datadir", 0);
	if (dir == NULL) {
		cerr << "'datadir' not found in szarp.cfg\n";
		return 1;
	}

	TSzarpConfig ipk;
	if (ipk.loadXML(SC::L2S(path))) {
		wcerr << "Error loading IPK from file '" << path << "'\n";
		return 1;
	}
	free(path);
	PeakEliminator pe = PeakEliminator(&ipk, dir, arguments.delta_fract);

	cout << showpoint << fixed;

	for (size_t i = 0; i < params.size(); i++) {
		TParam *p = ipk.getParamByName(params[i].c_str());
		if (p == NULL) {
			wcerr << L"Parameter '" << params[i] << L"' not found in IPK\n\n";;
			continue;
		}
		if (!p->IsInBase()) {
			wcerr << L"Parameter '" << params[i] << L"' is not in base\n\n";;
			continue;
		}
		pe.CheckParam(p);
	}
	free(dir);
	
	return 0;
}


const char* SzbException::what() const throw () {
	return m_cause.c_str();
}

SzbException::SzbException(const string& cause) : m_cause(cause)
{}

