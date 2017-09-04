#include "argsmgr.h"
#include <iostream>
#include "conversion.h"

void ArgsManager::parse(int argc, const char ** argv, const std::vector<ArgsHolder*>& opt_args) {
	cmd.parse(argc, argv, opt_args);
	const std::string SZARP_CFG = "/etc/szarp/szarp.cfg";

	auto base = cmd.get<std::string>("base");
	if (base) {
		#ifndef MINGW32
			libpar_init_from_folder("/opt/szarp/"+*base+"/");
		#else
			libpar_init_with_filename("%HOMEDRIVE%\\%HOMEPATH%\\.szarp\\"+base+"\\config\\params.xml");
		#endif
	} else {
		#ifndef MINGW32
			libpar_init_with_filename(SZARP_CFG.c_str(), 0);
		#else
			throw std::runtime_error("Cannot initialize without base prefix");
		#endif
	}
}

template <>
boost::optional<char*> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	char* libpar_val;
	if (section.empty()) 
		libpar_val = libpar_getpar(name.c_str(), arg.c_str(), 0);
	else
		libpar_val = libpar_getpar(section.c_str(), arg.c_str(), 0);
	
	if (libpar_val == NULL) return boost::none;

	return libpar_val;
}

template <>
boost::optional<wchar_t*> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	char* libpar_val;
	if (section.empty()) 
		libpar_val = libpar_getpar(name.c_str(), arg.c_str(), 0);
	else
		libpar_val = libpar_getpar(section.c_str(), arg.c_str(), 0);
	
	if (libpar_val == NULL) return boost::none;

	wchar_t* translated_val = wcsdup(SC::A2S(std::string(libpar_val)).c_str());

	return translated_val;
}

template <>
boost::optional<const char*> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	auto val = get_libparval<char*>(section, arg);
	if (val) return *val;
	return boost::none;
}

template <>
boost::optional<const wchar_t*> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	auto val = get_libparval<wchar_t*>(section, arg);
	if (val) return *val;
	return boost::none;
}

template <>
boost::optional<std::string> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const {
	auto val = get_libparval<char*>(section, arg);
	if (val) return std::string(*val);
	return boost::none;
}

template <>
boost::optional<std::wstring> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const {
	auto val = get_libparval<wchar_t*>(section, arg);
	if (val) return std::wstring(*val);
	return boost::none;
}

template <>
boost::optional<const std::string> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const {
	auto ret = get_libparval<std::string>(section, arg);

	if (ret) return *ret;
	return boost::none;
}

template <>
boost::optional<const std::wstring> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const {
	auto ret = get_libparval<std::wstring>(section, arg);

	if (ret) return *ret;
	return boost::none;
}

template <>
boost::optional<const std::string> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	if (cmd.vm.count("override") > 0) {
		auto overwritten_params = cmd.vm["override"].as<std::vector<std::string>>();
		auto found_it = std::find(overwritten_params.begin(), overwritten_params.end(), arg);
		if (found_it != overwritten_params.end()) return *found_it;
	}

	return boost::none;
}

template <>
boost::optional<const std::wstring> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	if (cmd.vm.count("override") > 0) {
		auto overwritten_params = cmd.vm["override"].as<std::vector<std::string>>();
		auto found_it = std::find(overwritten_params.begin(), overwritten_params.end(), arg);
		if (found_it != overwritten_params.end()) return SC::A2S(*found_it);
	}

	return boost::none;
}

template <>
boost::optional<const char*> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = get_overriden<const std::string>(section, arg);
	if (ret) return ret->c_str();
	return boost::none;
}

template <>
boost::optional<const wchar_t*> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = get_overriden<const std::wstring>(section, arg);
	if (ret) return ret->c_str();
	return boost::none;
}


template <>
boost::optional<std::string> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = get_overriden<const std::string>(section, arg);
	if (ret) return *ret;
	return boost::none;
}

template <>
boost::optional<std::wstring> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = get_overriden<const std::wstring>(section, arg);
	if (ret) return *ret;
	return boost::none;
}


template <>
boost::optional<char*> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = get_overriden<const char*>(section, arg);
	if (ret) return const_cast<char*>(*ret);
	return boost::none;
}

template <>
boost::optional<wchar_t*> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const {
	auto ret = get_overriden<const wchar_t*>(section, arg);
	if (ret) return const_cast<wchar_t*>(*ret);
	return boost::none;
}


