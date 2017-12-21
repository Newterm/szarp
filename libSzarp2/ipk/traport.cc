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
 * IPK 'radio' class implementation.
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
#include "conversion.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"

using namespace std;

#define FREE(x)	if (x != NULL) free(x)

TRaport::~TRaport()
{
	delete next;
}

TRaport* TRaport::GetNext()
{
	if (next != nullptr) {
		return next;
	} else {
		for (TParam * p = param->GetNextGlobal(); p; p = p->GetNextGlobal())
			for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
				if (title == r->GetTitle())
					return r;
	}
}

void TRaport::Append(TRaport* rap)
{
	TRaport* t = this;
	while (t->next)
		t = t->next;
	t->next = rap;
}

xmlNodePtr TRaport::GenerateXMLNode()
{
#define X (unsigned char *)
	xmlNodePtr r;
#define BUFSIZE 32
	wchar_t buffer[BUFSIZE];
	
	r = xmlNewNode(NULL, (unsigned char*)"raport");
	xmlSetProp(r, X"title", SC::S2U(title).c_str());
	if (!IsDescrDefault())
		xmlSetProp(r, X"description", SC::S2U(descr).c_str());
	if (order >= 0.0) {
		SWPRINTF(buffer, BUFSIZE, L"%g", order);
		buffer[BUFSIZE-1] = 0;
		xmlSetProp(r, X"order", SC::S2U(buffer).c_str());
	}
	return r;
#undef BUFSIZE
#undef X
}

std::wstring TRaport::GetDescr()
{
	if (!descr.empty())
		return descr;
	std::wstring n = param->GetName();
	size_t sp = n.rfind(L':');
	if (sp != std::string::npos)
		return n.substr(sp + 1);
	return n;
}

int TRaport::IsDescrDefault()
{
	if (descr.empty())
		return 1;
	std::wstring n = param->GetName();

	std::wstring d;
	size_t sp = n.rfind(L':');
	if (sp != std::string::npos)
		d = n.substr(sp + 1);
	else
		d = n;

	return d == descr;
}

