#include "argsmgr.h"
#include <iostream>
#include "conversion.h"
#include <boost/algorithm/string.hpp>

template <>
bool cast_val(const std::string& v_str) {
	if (v_str.empty()) return false;
	switch (v_str.front()) {
	case 'y':
	case 'Y':
	case '1':
	case 't':
	case 'T':
		return true;
	case 'n':
	case 'N':
	case '0':
	case 'f':
	case 'F':
		return false;
	default:
		throw std::runtime_error(std::string("Unknown bool operand ")+v_str);
	}
}

template <>
const char* cast_val(const std::string& v_str) {
	return v_str.c_str();
}

template <>
char* cast_val(const std::string& v_str) {
	return strdup(v_str.c_str());
}

template <>
const wchar_t* cast_val(const std::string& v_str) {
	return SC::A2S(v_str).c_str();
}

template <>
wchar_t* cast_val(const std::string& v_str) {
	return wcsdup(SC::A2S(v_str).c_str());
}

template <>
const std::string cast_val(const std::string& v_str) { return v_str; }

template <>
std::string cast_val(const std::string& v_str) { return v_str; }

template <>
const std::wstring cast_val(const std::string& v_str) { return SC::A2S(v_str); }

template <>
std::wstring cast_val(const std::string& v_str) { return SC::A2S(v_str); }
