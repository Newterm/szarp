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
#define WRITE_EACH 6

TProber::TProber() : TWriter(sec10), buffer(NULL), all_written(true)
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

void TProber::WaitForCycle(time_t period, time_t t)
{
	time_t tosleep = period - (t % period);
	while (tosleep > 0) {
		tosleep =  sleep(tosleep);
	}
}

int TProber::LoadConfig(const char *section)
{
	return TWriter::LoadConfig(section, "cachedir");
}

int TProber::LoadIPK()
{
	int ret = TWriter::LoadIPK();
	if (ret == -1) {
		return -1;
	}
	assert (params_len > 0);
	buffer = (short int*) malloc(params_len * sizeof(short int) * WRITE_EACH);
	assert (buffer != NULL);
	for (int i = 0; i < params_len; i++) {
		buffer[i] = SZB_FILE_NODATA;
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
		
