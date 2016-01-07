#ifndef SZ4_IKS_PARAM_INFO_H
#define SZ4_IKS_PARAM_INFO_H

namespace sz4 {

class param_info : public std::pair<std::wstring, std::wstring> {
public:
	param_info(const std::wstring& prefix, const std::wstring& name)
		 : std::pair<std::wstring, std::wstring>(prefix, name) {}

	const std::wstring& prefix() const { return this->first; }
	const std::wstring& name() const { return this->second; }
};


}

#endif
