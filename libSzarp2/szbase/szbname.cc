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
wchar2szb(std::wstring name)
{
    wchar_t pc = L'/';

    for (size_t i = 0; i < name.size(); i++) {
	if (L':' == name[i]) {
	    if (L'/' == pc)
		name[i] = L'_';
	    else
		name[i] = L'/';
	} else switch (name[i]) {
		case L'A'...L'Z':
		case L'a'...L'z':
		case L'0'...L'9':
			break;
		case L'\u0104': name[i] = L'A'; break;
		case L'\u0105': name[i] = L'a'; break;
		case L'\u0106': name[i] = L'C'; break;
		case L'\u0107': name[i] = L'c'; break;
		case L'\u0118': name[i] = L'E'; break;
		case L'\u0119': name[i] = L'e'; break;
		case L'\u0141': name[i] = L'L'; break;
		case L'\u0142': name[i] = L'l'; break;
		case L'\u0143': name[i] = L'N'; break;
		case L'\u0144': name[i] = L'n'; break;
		case L'\u00d3': name[i] = L'O'; break;
		case L'\u00f3': name[i] = L'o'; break;
		case L'\u015a': name[i] = L'S'; break;
		case L'\u015b': name[i] = L's'; break;
		case L'\u0179': name[i] = L'Z'; break;
		case L'\u017a': name[i] = L'z'; break;
		case L'\u017b': name[i] = L'Z'; break;
		case L'\u017c': name[i] = L'z'; break;
		default: name[i] = L'_';
	}

	pc = name[i];
    }
    return name;
}

