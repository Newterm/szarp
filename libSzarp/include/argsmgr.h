#ifndef ARGSLINEPARSER__H__
#define ARGSLINEPARSER__H__

#include <boost/optional.hpp>
#include "libpar.h"
#include "cmdlineparser.h"

class ArgsManager {
	std::string base_arg;
public:
	ArgsManager(const std::string& _name): name(_name), cmd(name) {}
	ArgsManager(std::string&& _name): name(_name), cmd(name) {}

	ArgsManager& operator=(ArgsManager&& other) = default; 
	ArgsManager(ArgsManager&& other) = default;

	ArgsManager(const ArgsManager&) = default;
	ArgsManager& operator=(const ArgsManager&) = default;

	~ArgsManager() {
		// Uncomment this once libpar is not used outside ArgsManager
		//libpar_done();
	}

	template <typename... As>
	void parse(int argc, const char ** argv, const As&... as) {
		cmd.parse(argc, argv, as...);
		base_arg = cmd.get("base").get_value_or("");
	}

	template <typename... As>
	void parse(int argc, char ** argv, const As&... as) {
		parse(argc, const_cast<const char **>(argv), as...);
	}

	void initLibpar() {
		const std::string SZARP_CFG = "/etc/szarp/szarp.cfg";
		if (!base_arg.empty()) {
			#ifndef MINGW32
				libpar_init_from_folder("/opt/szarp/"+base_arg+"/");
			#else
				libpar_init_with_filename("%HOMEDRIVE%\\%HOMEPATH%\\.szarp\\"+base_arg+"\\config\\params.xml");
			#endif
		} else {
			#ifndef MINGW32
				libpar_init_with_filename(SZARP_CFG.c_str(), 0);
			#else
				throw std::runtime_error("Cannot initialize without base prefix");
			#endif
		}
	}

	template <typename T = char*>
	boost::optional<T> get(const std::string& section, const std::string& arg) const;

	template <typename T = char*>
	boost::optional <T> get(const std::string& arg) const { 
		return get<T>(name, arg);
	}

	bool has(const std::string& arg) const {
		return has(name, arg);
	}

	bool has(const std::string& section, const std::string& arg) const {
		return cmd.count(arg) || libpar_getpar(section.c_str(), arg.c_str(), 0) != NULL;
	}

	size_t count(const std::string& arg) const {
		return cmd.count(arg);
	}

private:
	// Getting -D... parameters from cmd line. This is for backward compatibility.
	template <typename T>
	boost::optional<T> get_overriden(const std::string& section, const std::string& arg) const;

	// Getting parameters from cfg file (to refactor later on).
	template <typename T>
	boost::optional<T> get_libparval(const std::string& section, const std::string& arg) const;

	template <typename RT>
	RT cast_val(const std::string& at) const;

	std::string name;
	CmdLineParser cmd;
};

template <typename T>
boost::optional<T> ArgsManager::get(const std::string& section, const std::string& arg) const { 
	auto cmd_parser_ret = cmd.get<T>(arg);
	if (cmd_parser_ret) {
		try {
			return *cmd_parser_ret;
		} catch(...) {}
	}

	auto ov_ret = get_overriden<T>(section, arg);
	if (ov_ret) return *ov_ret;

	auto lp_ret = get_libparval<T>(section, arg);
	if (lp_ret) return *lp_ret;

	return boost::none;
}

template <typename T>
boost::optional<T> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	char* libpar_val;
	if (section.empty()) 
		libpar_val = libpar_getpar(name.c_str(), arg.c_str(), 0);
	else
		libpar_val = libpar_getpar(section.c_str(), arg.c_str(), 0);
	
	if (libpar_val == NULL) return boost::none;

	try {
		return cast_val<T>(std::string(libpar_val));
	} catch(const boost::bad_lexical_cast& e) {
		return boost::none;
	}
}

template <typename T>
boost::optional<T> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const { 
	if (cmd.vm.count("override") > 0) {
		auto overwritten_params = cmd.vm["override"].as<std::vector<std::string>>();
		auto found_it = std::find(overwritten_params.begin(), overwritten_params.end(), arg);
		if (found_it != overwritten_params.end()) {
			try {
				auto val = cast_val<T>(*found_it);
				return val;
			} catch(const boost::bad_lexical_cast& e) {}
		}
	}

	return boost::none;
}

template <typename RT>
RT ArgsManager::cast_val(const std::string& v_str) const {
	return boost::lexical_cast<RT>(v_str);
}

template <>
const char* ArgsManager::cast_val(const std::string& v_str) const;

template <>
char* ArgsManager::cast_val(const std::string& v_str) const;

template <>
const wchar_t* ArgsManager::cast_val(const std::string& v_str) const;

template <>
wchar_t* ArgsManager::cast_val(const std::string& v_str) const;

template <>
const std::string ArgsManager::cast_val(const std::string& v_str) const;

template <>
std::string ArgsManager::cast_val(const std::string& v_str) const;

template <>
const std::wstring ArgsManager::cast_val(const std::string& v_str) const;

template <>
std::wstring ArgsManager::cast_val(const std::string& v_str) const;

#endif
