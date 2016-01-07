#ifndef __LOCATIONS_ERROR_CODES_H__
#define __LOCATIONS_ERROR_CODES_H__

#include <boost/system/error_code.hpp>
#include <sstream>

typedef unsigned int error_code_t;

enum class ErrorCodes : error_code_t { 
	ill_formed       = 2,
	unknown_command  = 4,
	invalid_id       = 6,
	id_used          = 8,
	response_timeout = 10,
	unknown_remote   = 20,
	unknown_param    = 100,
	invalid_pin      = 102,
	cannot_set_value = 104,
	not_summaric     = 106,
	unknown_set      = 200,
	invalid_set_hash = 202,
	set_read_only    = 204,
	unknown_option   = 300,
	invalid_timestamp= 400,
	wrong_probe_type = 402,
	szbase_error     = 404,
	internal_error   = 500,
	protocol_not_set = 502,
};

namespace boost { namespace system {

template<> struct is_error_code_enum<ErrorCodes>
{
	static const bool value = true;
};

} }

namespace detail {

class iks_category : public boost::system::error_category
{
public:
	const char* name() const BOOST_SYSTEM_NOEXCEPT
	{
		return "iks";
	}

	std::string message(int value) const
	{
		switch (static_cast<ErrorCodes>(value)) {
			case ErrorCodes::ill_formed:
				return "ill formed request";
			case ErrorCodes::unknown_command:
				return "unknown command";
			case ErrorCodes::invalid_id:
				return "invalid id";
			case ErrorCodes::id_used:
				return "id in use";
			case ErrorCodes::response_timeout:
				return "response timeout";
			case ErrorCodes::unknown_remote:
				return "unknown remote";
			case ErrorCodes::unknown_param:
				return "unknown param";
			case ErrorCodes::invalid_pin:
				return "invaid pin";
			case ErrorCodes::cannot_set_value:
				return "cannot set value";
			case ErrorCodes::not_summaric:
				return "not a summary value";
			case ErrorCodes::unknown_set:
				return "unknown set";
			case ErrorCodes::invalid_set_hash:
				return "invalid set hash";
			case ErrorCodes::set_read_only:
				return "set read only";
			case ErrorCodes::unknown_option:
				return "unknown option";
			case ErrorCodes::invalid_timestamp:
				return "invalid timestamp";
			case ErrorCodes::wrong_probe_type:
				return "wrong probe type";
			case ErrorCodes::szbase_error:
				return "szbase error";
			case ErrorCodes::internal_error:
				return "internal error";
			case ErrorCodes::protocol_not_set:
				return "protocol not set";
			default:
				return "iks error";				
		};
  	}
};

}

inline const boost::system::error_category& get_iks_category()
{
  static detail::iks_category instance;
  return instance;
}

inline boost::system::error_code make_error_code(ErrorCodes e)
{
	return boost::system::error_code( static_cast<int>(e) , get_iks_category() );
}

inline boost::system::error_code make_iks_error_code(const std::string& error)
{
	int e;
	std::istringstream(error) >> e;

	return boost::system::error_code( e , get_iks_category() );
}

#endif /* end of include guard: __LOCATIONS_ERROR_CODES_H__ */
