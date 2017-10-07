#include "argsmgr.h"
#include <iostream>
#include "conversion.h"

template <>
const char* ArgsManager::cast_val(const std::string& v_str) const {
	return v_str.c_str();
}

template <>
char* ArgsManager::cast_val(const std::string& v_str) const {
	return strdup(v_str.c_str());
}

template <>
const wchar_t* ArgsManager::cast_val(const std::string& v_str) const {
	return SC::A2S(v_str).c_str();
}

template <>
wchar_t* ArgsManager::cast_val(const std::string& v_str) const {
	return wcsdup(SC::A2S(v_str).c_str());
}

template <>
const std::string ArgsManager::cast_val(const std::string& v_str) const { return v_str; }

template <>
std::string ArgsManager::cast_val(const std::string& v_str) const { return v_str; }

template <>
const std::wstring ArgsManager::cast_val(const std::string& v_str) const { return SC::A2S(v_str); }

template <>
std::wstring ArgsManager::cast_val(const std::string& v_str) const { return SC::A2S(v_str); }


