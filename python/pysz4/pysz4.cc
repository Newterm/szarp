#include "pysz4.hpp"

namespace libpysz4 {

Sz4::Sz4(std::wstring szarp_system_dir, std::wstring szarp_data_dir, std::wstring lang)
		: m_ipk(szarp_data_dir, szarp_system_dir, lang)
		, m_sz4(szarp_data_dir, &m_ipk)
{}

double Sz4::get_value(const std::wstring& param_name, double time, SZARP_PROBE_TYPE probe_type) {
	sz4::nanosecond_time_t sz4_time(time);
	TParam* param = get_param(param_name);

	sz4::weighted_sum<double, sz4::nanosecond_time_t> wsum;
	sz4::nanosecond_time_t sz4_end_time = szb_move_time(sz4_time, 1, probe_type, 0);
	m_sz4.get_weighted_sum(param, sz4_time, sz4_end_time, probe_type, wsum);

	return wsum.avg();
}

double Sz4::search_first(const std::wstring &param_name) {
	sz4::nanosecond_time_t ret;
	TParam* param = get_param(param_name);

	m_sz4.get_first_time(param, ret);

	return ret;
}

double Sz4::search_last(const std::wstring &param_name) {
	sz4::nanosecond_time_t ret;
	TParam* param = get_param(param_name);

	m_sz4.get_last_time(param, ret);

	return ret;
}

double Sz4::search(const std::wstring &param_name, double start, double end, int direction, SZARP_PROBE_TYPE probe) {
	sz4::nanosecond_time_t ret;
	TParam* param = get_param(param_name);

	if (direction < 0)
		ret = m_sz4.search_data_left(param, sz4::nanosecond_time_t(start), sz4::nanosecond_time_t(end), probe, sz4::no_data_search_condition());
	else
		ret = m_sz4.search_data_right(param, sz4::nanosecond_time_t(start), sz4::nanosecond_time_t(end), probe, sz4::no_data_search_condition());

	return ret;
}

TParam* Sz4::get_param(const std::wstring& param_name) {
	TParam* param = m_ipk.GetParam(param_name);

	if (!param)
		throw std::runtime_error(SC::S2L(L"Param :\"" + param_name + L"\" not found"));

	return param;
}

}

BOOST_PYTHON_MODULE(libpysz4)
{
	using namespace boost::python;

	enum_<SZARP_PROBE_TYPE>("PROBE_TYPE")
		.value("PT_MSEC10", PT_MSEC10)
		.value("PT_HALFSEC", PT_HALFSEC)
		.value("PT_SEC", PT_SEC)
		.value("PT_SEC10", PT_SEC10)
		.value("PT_MIN10", PT_MIN10)
		.value("PT_HOUR", PT_HOUR)
		.value("PT_HOUR8", PT_HOUR8)
		.value("PT_DAY", PT_DAY)
		.value("PT_WEEK", PT_WEEK)
		.value("PT_MONTH", PT_MONTH)
		.value("PT_YEAR", PT_YEAR)
		;

	class_<libpysz4::Sz4, boost::noncopyable>
        ("Sz4", init<optional<std::wstring, std::wstring, std::wstring> >())
        .def("get_value", &libpysz4::Sz4::get_value)
        .def("search_first", &libpysz4::Sz4::search_first)
        .def("search_last", &libpysz4::Sz4::search_last)
        .def("search", &libpysz4::Sz4::search)
        ;
}

