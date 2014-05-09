#ifndef __UTILS_EXCEPTIONS_H__
#define __UTILS_EXCEPTIONS_H__

#include <exception>

/**
 * @brief Wyjątek z ustawialną treścią
 */
class msg_error : public std::exception {
public:
	msg_error(const char* _msg) throw() :msg(_msg)
	{}

	msg_error(const std::string& _msg) throw() :msg(_msg)
	{}

	virtual ~msg_error() throw() {}

	virtual const char* what() const throw()
	{	return msg.c_str(); }

protected:
	std::string msg;
};

#define DECLARE_MSG_BASE_EXCEPT(type,base) \
	class type : public base { \
	public: \
		type(const char* m) throw() : base(m) {} \
		type(const std::string& m) throw() : base(m) {} \
	}

#define DECLARE_MSG_EXCEPT(type) DECLARE_MSG_BASE_EXCEPT(type,msg_error) 

DECLARE_MSG_EXCEPT(config_error);
DECLARE_MSG_EXCEPT(file_not_found_error);

DECLARE_MSG_EXCEPT(parse_error);
DECLARE_MSG_BASE_EXCEPT(xml_parse_error,parse_error);
DECLARE_MSG_BASE_EXCEPT(uri_parse_error,parse_error);

DECLARE_MSG_EXCEPT(szbase_error);
DECLARE_MSG_BASE_EXCEPT(szbase_init_error,szbase_error);
DECLARE_MSG_BASE_EXCEPT(szbase_get_value_error,szbase_error);

#endif /* __UTILS_EXCEPTIONS_H__ */

