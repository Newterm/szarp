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

#include "ssexception.h"
#include "config.h"
#include "libpar.h"
#include "liblog.h"
#include "ssutil.h"

#include <errno.h>
#include <stdio.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include <fcntl.h>

#include <string>

#ifndef MINGW32
bool InitLogging(int *argc, char **argv, const char* progname) {
	int loglevel = loginit_cmdline(2, NULL, argc, argv);

	char *logfile = libpar_getpar(argv[0], "log", 0);
	if (logfile == NULL) 
		asprintf(&logfile, PREFIX"/logs/%s.log", progname);

	loglevel = loginit(loglevel, logfile);
	if (loglevel < 0) {
		sz_log(0, "%s: cannot initalize log file '%s', errno %d",
		       argv[0], logfile, errno);
		return false;
	}
	return true;
}
#endif

void InitSSL()
{
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_crypto_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
	if(SSL_COMP_add_compression_method(0xe0, COMP_zlib())) {
		sz_log(1, "Failed to add zlib compression method");
	}
}

int PassphraseCallback(char *buf, int size, int rwflag, void *userdata) {
		strncpy(buf, (const char*) userdata, size);
		buf[size - 1] = '\0';
		return strlen(buf);
}

