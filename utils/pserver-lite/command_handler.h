#ifndef __COMMAND_HANDLER_H
#define __COMMAND_HANDLER_H

#include <string>
#include <vector>
#include <stdexcept>

class CommandHandler {
public:
	virtual void parse_args (const std::vector<std::string>& args) = 0;
	virtual void exec (void) = 0;

	/* Exception class definitions */
	class Exception : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};
	class ParseArgsError : public Exception {
		using Exception::Exception;
	};
	class SzbCacheError : public Exception {
		using Exception::Exception;
	};
	class ConnectionError : public Exception {
		using Exception::Exception;
	};
};

#endif /* __COMMAND_HANDLER_H */
