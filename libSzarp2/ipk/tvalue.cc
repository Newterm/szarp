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
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'value' class implementation.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <string>
#include <dirent.h>
#include <assert.h>

#include "szarp_config.h"
#include "parcook_cfg.h"
#include "line_cfg.h"
#include "liblog.h"
#include "conversion.h"

using namespace std;

#define FREE(x)	if (x != NULL) free(x)

TValue *TValue::createValue(int prec)
{
	switch (prec) {
		case 5:
			return new TValue(0, L"Nie", 
				new TValue(1, L"Tak"));
		case 6:
			return new TValue(0, L"Pochmurno", 
				new TValue(1, L"Zmiennie",
				new TValue(2, L"S\u0142onecznie")));
		case 7:
			return new TValue(-1, L"Minus",
				new TValue(1, L"Plus"));
		default:
			return NULL;
	}
}

TValue* TValue::Append(TValue* v)
{
	TValue* t = this;
	while (t->next)
		t = t->next;
	t->next = v;
	return v;
}

TValue::~TValue()
{
	delete next;
}
	
TValue* TValue::SearchValue(int val)
{
	for (TValue *t = this; t; t = t->next)
		if (t->intValue == val)
			return t;
	return NULL;
}

