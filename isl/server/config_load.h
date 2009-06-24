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
 * Configuration handling.
 * $Id$
 */

#ifndef __CONFIG_LOAD_H__
#define __CONFIG_LOAD_H__

#include "liblog.h"

/**
 * Class is responsible for obtaining and holding informations from
 * program configuration's file. It is wrapper for libparNT library (part of 
 * SZARP library). Only one object of this class should be created for 
 * and it should live during all program execution time.
 */
class ConfigLoader {
	public:
		ConfigLoader(const char *filename, const char *_section,
				int *argc, char **argv);
		~ConfigLoader();
		char * getString(const char *name, const char *defval) const;
		char * getStringSect(const char *sect, const char *name, const char *defval) const;
		int getInt(const char *name, int defval) const;
		int getIntSect(const char *sect, const char *name, int defval) const;
	private:
		const char *section;	/**< Name of default section to read params from. */
		mutable char *buffer;	/**< Buffer for reading params from libpar */
};
		
#endif

