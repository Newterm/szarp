#include <boost/python.hpp>
#include "szarp_config.h"
#include "ipkcontainer.h"
#include "sz4/base.h"
#include "conversion.h"


namespace libpysz4 {

class Sz4
{
public:
	Sz4(std::wstring szarp_system_dir = L"/opt/szarp", std::wstring szarp_data_dir = L"/opt/szarp", std::wstring lang = L"pl");

	double get_value(const std::wstring& param_name, double time, SZARP_PROBE_TYPE probe_type);
	
	double search_first(const std::wstring &param_name);

	double search_last(const std::wstring &param_name);

	double search(const std::wstring &param_name, double start, double end, int direction, SZARP_PROBE_TYPE probe);

	void close_logger();

private:
	TParam* get_param(const std::wstring& param_name);

	ParamCachingIPKContainer m_ipk;
	sz4::base m_sz4;


};

}
