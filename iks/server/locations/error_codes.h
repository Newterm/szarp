#ifndef __LOCATIONS_ERROR_CODES_H__
#define __LOCATIONS_ERROR_CODES_H__

typedef unsigned int error_code_t;

enum class ErrorCodes : error_code_t { 
	ill_formed       = 2,
	unknown_command  = 4,
	invalid_id       = 6,
	id_used          = 8,
	response_timeout = 10,
	unknown_param    = 100,
	invalid_pin      = 102,
	cannot_set_value = 104,
	unknown_set      = 200,
	invalid_set_hash = 202,
	set_read_only    = 204,
	unknown_option   = 300,
	invalid_timestamp= 400,
	internal_error   = 500,
	protocol_not_set = 502,
};

#endif /* end of include guard: __LOCATIONS_ERROR_CODES_H__ */

