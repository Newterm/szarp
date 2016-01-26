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
 * proper - daemon for writing 10-seconds data probes to disk
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#include "tprober.h"
#include "prober.h"
#include "conversion.h"

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include "liblog.h"
#include "libpar.h"

#include "sumator.h"

/** write data to file each WRITE_EACH cycle */
#define WRITE_EACH 1


TProber::TProber(int probes_buffer_size)
: TWriter(sec10), buffer(NULL), all_written(true), m_probes_buffer_size(probes_buffer_size)
{
}

TProber::~TProber()
{
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
	g_prober = NULL;
}

time_t TProber::WaitForCycle(time_t period, time_t t)
{
	time_t tosleep = period - (t % period);
	time_t ret = tosleep;
	tosleep -= m_fr.Go(time(NULL) + tosleep);
	while (tosleep > 0) {
		tosleep = sleep(tosleep);
	}
	return ret;
}

bool TProber::IsBuffered()
{
	if (m_probes_buffer_size != 0) return true;
	return false;
}

int TProber::GetBuffSize()
{
	return m_probes_buffer_size;
}

int TProber::LoadConfig(const char *section)
{
	int ret = TWriter::LoadConfig(section, "cachedir");
	//rest of the code assumes that cachedir exists, so if it is not the case -
	//let's create it 
	if (!fs::exists(data_dir))
		fs::create_directory(data_dir);
	m_fr.Init(path(SC::S2A(data_dir)), atoi(libpar_getpar(section, "months_count", 1)));
	return ret;
}

int TProber::LoadIPK()
{
	int ret = TWriter::LoadIPK();
	if (ret == -1) {
		return -1;
	}
	assert (params_len > 0);

	if (IsBuffered()) {
		buffer = (short int*) malloc(params_len * sizeof(short int) * GetBuffSize());
		assert (buffer != NULL);
		for (int i = 0; i < params_len * GetBuffSize(); i++) {
			buffer[i] = SZB_FILE_NODATA;
		}
	} else {
		buffer = (short int*) malloc(params_len * sizeof(short int) * WRITE_EACH);
		assert (buffer != NULL);
		for (int i = 0; i < params_len; i++) {
			buffer[i] = SZB_FILE_NODATA;
		}
	}
	return ret;
}


void TProber::WriteParams(bool force_write)
{
	time_t t;
	int err = 0, ok = 0;

	if (force_write and all_written) {
		return;
	}
	time(&t);
	t = t - (t % BASE_PERIOD);
	int cycle = (t % (BASE_PERIOD * WRITE_EACH)) / BASE_PERIOD;
	sz_log(9, "Cycle %d of %d", cycle, WRITE_EACH);

	/* block signals */
	g_signals_blocked = 1;

	for (int i = 0; i < params_len; i++) {
		if (params[i] != NULL) {
			buffer[WRITE_EACH * i + cycle] = parcook->GetData(i);
			if ((cycle == WRITE_EACH - 1) or force_write) {
				/* last cycle, write data */
				if (params[i]->WriteProbes(fs::wpath(data_dir), 
							t, 
							buffer + WRITE_EACH * i, 
							cycle + 1)) {
					err++;
				} else {
					ok++;
				}
			}
			
		}
	}
	if (cycle == WRITE_EACH - 1) {
		all_written = true;
	} else {
		all_written = false;
	}
	sz_log(5, "prober: writing params: %d ok, %d errors",
			ok, err);
	/* unblock signals */
	g_signals_blocked = 0;
	if (g_should_exit and !force_write) {
		/* block signals */
		sigset_t sigmask;
		sigfillset(&sigmask);
		sigdelset(&sigmask, SIGKILL);
		sigdelset(&sigmask, SIGSTOP);
		sigprocmask(SIG_SETMASK, &sigmask, NULL);
		/* call terminate handler */
		g_TerminateHandler(0);
		/* unblock terminate signal raised by g_TerminateHandler */
		sigemptyset(&sigmask);
		sigaddset(&sigmask, SIGTERM);
		sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
	}
}

void TProber::WriteParamsBuffered(bool force_write)
{
	time_t t;
	int err = 0, ok = 0;

	time(&t);
	t = t - (t % BASE_PERIOD);
	sz_log(9, "Write params buffered %ld", (long int)t);

	/* block signals */
	g_signals_blocked = 1;

	for (int i = 0; i < params_len; i++) {
		if (params[i] != NULL) {
			int param_off = GetBuffSize() * i;
			int read_count = parcook->GetData(i, buffer + param_off);
			sz_log(9, "Read count: %d, buffer size: %d", read_count, GetBuffSize());
			
			std::ostringstream os;
			for (int j = 0; j < read_count; ++j) {
				os << buffer[param_off + j] << " ";
			} 	
			sz_log(3, "Buf(%d):[%s] read_count:%d",i,os.str().c_str(),read_count); 

			if (read_count > 0) {
				if (params[i]->WriteProbes(fs::wpath(data_dir), t, 
					buffer + param_off,
					read_count)) {
					err++;
				} else {
					ok++;
				}
			}
		}
	}

	all_written = true;

	sz_log(5, "prober: writing params: %d ok, %d errors",
			ok, err);

	/* unblock signals */
	g_signals_blocked = 0;
	if (g_should_exit and !force_write) {
		/* block signals */
		sigset_t sigmask;
		sigfillset(&sigmask);
		sigdelset(&sigmask, SIGKILL);
		sigdelset(&sigmask, SIGSTOP);
		sigprocmask(SIG_SETMASK, &sigmask, NULL);
		/* call terminate handler */
		g_TerminateHandler(0);
		/* unblock terminate signal raised by g_TerminateHandler */
		sigemptyset(&sigmask);
		sigaddset(&sigmask, SIGTERM);
		sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
	}
}
