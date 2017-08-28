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
 * Pawe³ Pa³ucha 2002
 *
 * Configration handling.
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "libpar.h"
#include "liblog.h"

#include "config_load.h"

/**
 * Loads configuration from given file. 
 * Also starts log facility.
 * @param filename name of configuration file, if NULL, default is used
 * (szarp.cfg). 
 * @param section name of default section to read params from
 * @param argc pointer to first main() argument
 * @param argv second main() argument (for getting command line definitions)
 */
ConfigLoader::ConfigLoader(const char *filename, const char *_section,
		int *argc, char **argv) : section(_section), buffer(NULL)
{
	libpar_read_cmdline(argc, argv);
	libpar_init_with_filename(filename, 1);
}

/**
 * Destroy configuration info handler, closes log facility.
 * @note log is not closed beacuse it can be used by other processes
 */
ConfigLoader::~ConfigLoader(void)
{
	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	libpar_done();
	//logdone();
}

/**
 * Reads configuration parameter integer value from given section.
 * @param sect name of section
 * @param name name of param to read
 * @param defval default value of param, if NULL param must exist - otherwise
 * program exits with error
 * @return integer representation of param value, 
 * or defval if param is not found
 */
int ConfigLoader::getIntSect(const char *sect, const char *name, int defval) const
{
	if (buffer != NULL)
		free(buffer);
	buffer = libpar_getpar(sect, name, 0);
	if (buffer == NULL)
		return defval;
	return atoi(buffer);
}

/**
 * Reads configuration parameter integer value from default section.
 * @param name name of param to read
 * @param defval default value of param, if NULL param must exist - otherwise
 * program exits with error
 * @return integer representation of param value, 
 * or defval if param is not found
 */
int ConfigLoader::getInt(const char *name, int defval) const
{
	return getIntSect(section, name, defval);
}


/**
 * Read value of given param from given section from configuration file.
 * @param sect name of section
 * @param name param name
 * @param defval default value of param, if NULL, program exits with error
 * message when param is not found
 * @return defval if defval is not NULL and param is not found, or pointer to
 * buffer containing param value
 */
char * ConfigLoader::getStringSect(const char * sect, const char *name, const char *defval) const
{
	if (buffer != NULL)
		free(buffer);
	buffer = libpar_getpar(sect, name, (defval == NULL));
	if (buffer == NULL)
		buffer = strdup(defval);
	return buffer;
}/**
 * Read value of given param from default section from configuration file.
 * @param name param name
 * @param defval default value of param, if NULL, program exits with error
 * message when param is not found
 * @return defval if defval is not NULL and param is not found, or pointer to
 * static buffer containing param value
 */
char * ConfigLoader::getString(const char *name, const char *defval) const 
{
	return getStringSect(section, name, defval);
}

