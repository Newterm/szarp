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
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
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

int TValue::getSzarpId(void)
{
	int checkTN[2] = {0, 0};
	int checkPM[2] = {0, 0};
	int checkPZS[3] = {0, 0, 0};
	int cTN = 0, cPM = 0, cPZS = 0;
	TValue *t;
	
	for (t = this; t; t = t->next) {
		if (t->name == L"Tak") {
			if (t->intValue <= 0)
				return 0;
			if (checkTN[1])
				return 0;
			checkTN[1] = 1;
			cTN++;
			continue;
		}
		if (t->name == L"Nie") {
			if (t->intValue != 0)
				return 0;
			if (checkTN[0])
				return 0;
			checkTN[0] = 1;
			cTN++;
			continue;
		}
		if (t->name ==  L"Plus") {
			if (t->intValue < 0)
				return 0;
			if (checkPM[1])
				return 0;
			checkPM[1] = 1;
			cPM++;
			continue;
		}
		if (t->name == L"Minus") {
			if (t->intValue > 0)
				return 0;
			if (checkPM[0])
				return 0;
			checkPM[0] = 1;
			cPM++;
			continue;
		}
		if (t->name == L"Pochmurno") {
			if (t->intValue != 0)
				return 0;
			if (checkPZS[0])
				return 0;
			checkPZS[0] = 1;
			cPZS++;
			continue;
		}
		if (t->name == L"Zmiennie") {
			if (t->intValue != 1)
				return 0;
			if (checkPZS[1])
				return 0;
			checkPZS[1] = 1;
			cPZS++;
			continue;
		}
		if (t->name == L"S\u0142onecznie") {
			if (intValue != 2)
				return 0;
			if (checkPZS[2])
				return 0;
			checkPZS[2] = 1;
			cPZS++;
			continue;
		}
		return 0;
	}
	if (cTN) {
		if (cPM || cPZS)
			return 0;
		return 5;
	}
	if (cPM) {
		if (cTN || cPZS)
			return 0;
		return 7;
	}
	if (cPZS) {
		if (cPM || cTN)
			return 0;
		return 6;
	}
	return 0;
}

xmlNodePtr TValue::generateXMLNode(void)
{
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define BUF X(buffer)
	char buffer[10];
	xmlNodePtr r;
	
	r = xmlNewNode(NULL, X"value");
	ITOA(intValue);
	xmlSetProp(r, X"int", BUF);
	xmlSetProp(r, X"name", SC::S2U(name).c_str());
	return r;
#undef X
#undef ITOA
#undef BUF
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

