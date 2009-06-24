/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbname.c
 * $Id$
 * <pawel@praterm.com.pl>
 */

#include <szbname.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

std::wstring 
wchar2szb(const std::wstring& name)
{
    std::wstring r = name;
    wchar_t pc = L'/';

    for (size_t i = 0; i < r.size(); i++) {
	wchar_t& c = r[i];
	if (L':' == c) {
	    if (L'/' == pc)
		c = L'_';
	    else
		c = L'/';
	} else switch (c) {
		case L'A'...L'Z':
		case L'a'...L'z':
		case L'0'...L'9':
			break;
		case L'\u0104': c = L'A'; break;
		case L'\u0105': c = L'a'; break;
		case L'\u0106': c = L'C'; break;
		case L'\u0107': c = L'c'; break;
		case L'\u0118': c = L'E'; break;
		case L'\u0119': c = L'e'; break;
		case L'\u0141': c = L'L'; break;
		case L'\u0142': c = L'l'; break;
		case L'\u0143': c = L'N'; break;
		case L'\u0144': c = L'n'; break;
		case L'\u00d3': c = L'O'; break;
		case L'\u00f3': c = L'o'; break;
		case L'\u015a': c = L'S'; break;
		case L'\u015b': c = L's'; break;
		case L'\u0179': c = L'Z'; break;
		case L'\u017a': c = L'z'; break;
		case L'\u017b': c = L'Z'; break;
		case L'\u017c': c = L'z'; break;
		default: c = L'_';

	}

	pc = c;
    }
    return r;
}

