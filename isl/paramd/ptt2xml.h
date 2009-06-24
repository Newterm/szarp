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
 * ptt2xml.h - PTT.act to XML conversion
 *
 * $Id$
 */

#ifndef __PTT2XML_H__
#define __PTT2XML_H__

#include <string>
#include <vector>
#include <libxml/parser.h>
#include "ptt_act.h"
#include "probes_types.h"

/**
 * Reading param values from parcook shared memory segment.
 */
class PTTParamProxy {
public:
	/** @param _parcook_path path to parcook - shared memory
	 * segment identity string */
	PTTParamProxy(char *_parcook_path) : 
		parcook_path(_parcook_path),
		values(VT_HOUR + 1)
		{ };
	virtual ~PTTParamProxy() {}; 
	virtual void update();
	virtual std::wstring getValue(int ind, int prec, PROBE_TYPE probe_type);
	virtual std::wstring getCombinedValue(int indmsw, int indlsw, int prec, PROBE_TYPE probe_type);
	virtual int setValue(int ind, int prec, const std::wstring &value);
protected:
	void update(std::vector<short>& values, int shm_type, int sem_type);
	int str2val(const std::wstring &value, int prec, int *val);
	char *parcook_path;			/**< path to parcook */
	std::vector<std::vector<short> > values;
};
#endif

