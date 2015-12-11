#include <boost/system/error_code.hpp>
#include "iks_cmd_id.h"

namespace detail {

const char* iks_client_category::name() const BOOST_SYSTEM_NOEXCEPT
{
	return "iks";
}

std::string iks_client_category::message(int value) const
{
	switch (value) {
		case iks_client_error::not_connected_to_peer:
			return "not connected to requested base";
		case iks_client_error::invalid_server_response:
			return "server response can't be processed";
		default:
			return "iks client error";				
	};

}

}

const boost::system::error_category& get_iks_client_category()
{
  static detail::iks_client_category instance;
  return instance;
}

