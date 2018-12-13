#ifndef _DEF_VERIFIER
#define _DEF_VERIFIER

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "defcfg.h"

class DefinedVerifier {
public:
	template <class T>
	void operator()(T&, const std::vector<DefinedParam*>&);

	DefinedVerifier(const ConfigManager* mgr): cfg_mgr(mgr) {}

private:
	template <typename T>
	const std::wstring getParamGlobalName(const T& p) const;

	bool globalNameIsValid(const std::wstring& param_name) const {
 		return std::count(param_name.begin(), param_name.end(), wchar_t(':')) == NUM_OF_SEPARATORS;
	}

	bool paramOfNameExists(const std::wstring& param_name, const std::vector<DefinedParam*>&) const;

	const ConfigManager* cfg_mgr;
	const int NUM_OF_SEPARATORS = 3;
};

template <typename T>
const std::wstring DefinedVerifier::getParamGlobalName(const T &p) const {
	std::string base(p.GetBasePrefix().mb_str(wxConvUTF8));
	std::string pname(p.GetParamName().mb_str(wxConvUTF8));
	// since wx3.0 ToStdWstring

	return SC::L2S(base + ":" + pname);
}

bool DefinedVerifier::paramOfNameExists(const std::wstring& param_name, const std::vector<DefinedParam*>& defined_params) const {
	for (const auto& dp: defined_params) {
		if (getParamGlobalName(*dp) == param_name) return true;
	}

	if (cfg_mgr->GetIPKs()->GetParam(param_name, true) != NULL) {
		return true;
	}

	return false;
}

template <>
void DefinedVerifier::operator()(DrawInfo& param, const std::vector<DefinedParam*>& defined_params) {
	const std::wstring param_name = getParamGlobalName(param);
	if (!globalNameIsValid(param_name)) {
		throw IllFormedParamException(wxString(_("Param ")+param.GetBasePrefix()+_T(":")+param.GetParamName()+_(" is ill-formed.")));
	}

	if (!paramOfNameExists(param_name, defined_params)) {
		throw IllFormedParamException(wxString(_("Param ")+param.GetBasePrefix()+_T(":")+param.GetParamName()+_(" does not exist in configuration.")));
	}
}

template <>
void DefinedVerifier::operator()(DefinedParam& param, const std::vector<DefinedParam*>&) {
	const std::wstring param_name = getParamGlobalName(param);
	if (!globalNameIsValid(param_name)) {
		throw IllFormedParamException(wxString(_("Param ")+param.GetBasePrefix()+_T(":")+param.GetParamName()+_(" is ill-formed.")));
	}

	// add LUA check
}

template <>
void DefinedVerifier::operator()(DefinedDrawSet& ds, const std::vector<DefinedParam*>& defined_params) {
	auto draws = ds.GetDraws();
	auto draw = draws->begin();
	auto draws_end = draws->end();

	while( draw != draws_end )
	{
		try {
			operator()( **draw, defined_params );
			std::advance(draw, 1);
		}
		catch (const SzException& e) {
			wxString msg = _("Failed to import defined draw param from configuration file.\n")
				+ wxString::FromUTF8(e.what())
				+ _("Do you want to delete invalid param from this configuration?");
			if( wxMessageBox(msg, _("Operation failed."),
						wxYES_NO | wxICON_ERROR, wxGetApp().GetTopWindow()) == wxYES )
				draw = ds.Delete( draw );
			else
				throw e;
		}
	}
}

#endif
