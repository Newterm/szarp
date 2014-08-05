/* 
  libSzarp - SZARP library

*/
/* $Id$ */

#ifndef CONVERSION_H
#define CONVERSION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef MINGW32
#define SWPRINTF snwprintf
#else
#define SWPRINTF swprintf
#endif

#include <iostream>
#include <string>

namespace SC {

std::wstring U2S(const std::basic_string<unsigned char>& c);

std::basic_string<unsigned char> S2U(const std::wstring& c);

std::wstring A2S(const std::basic_string<char>& c);

std::string S2A(const std::basic_string<wchar_t>& c);

std::basic_string<unsigned char> A2U(const std::basic_string<char>& c, bool fallback = false);

std::string U2A(const std::basic_string<unsigned char>& c);

std::wstring L2S(const std::basic_string<char>& c, bool fallback = false);

std::string S2L(const std::basic_string<wchar_t>& c);

std::basic_string<unsigned char> L2U(const std::basic_string<char>& c, bool fallback = false);

std::string U2L(const std::basic_string<unsigned char>& c);

/** lua library returns error string which may contain a slit in half
 * unicode 2-byte, which makes iconv() call in U2S fail */
std::wstring lua_error2szarp(const char* lua_error);
}

#endif
