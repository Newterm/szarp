#ifndef __COMMAND_HANDLER_H
#define __COMMAND_HANDLER_H

#include <string>
#include <vector>
#include <stdexcept>

// TODO: move this macro to some more appropriate place
#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#endif

class CommandHandler {
public:
	static std::vector<std::string> tokenize (std::string& msg_received);

	virtual void load_args (const std::vector<std::string>& args) = 0;
	virtual std::vector<unsigned char> exec (void) = 0;

	/* Exception class definitions */
#if GCC_VERSION >= 40800
	class Exception : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};
	class ParseError : public Exception {
		using Exception::Exception;
	};
	class ArgumentError : public Exception {
		using Exception::Exception;
	};
	class SzbCacheError : public Exception {
		using Exception::Exception;
	};
	class ConnectionError : public Exception {
		using Exception::Exception;
	};
#else
	class Exception : public std::runtime_error {
	public:
		explicit Exception (const std::string& what_arg)
			: std::runtime_error(what_arg) { }
	};
	class ParseError : public Exception {
	public:
		explicit ParseError (const std::string& what_arg)
			: Exception(what_arg) { }
	};
	class ArgumentError : public Exception {
	public:
		explicit ArgumentError (const std::string& what_arg)
			: Exception(what_arg) { }
	};
	class SzbCacheError : public Exception {
	public:
		explicit SzbCacheError (const std::string& what_arg)
			: Exception(what_arg) { }
	};
	class ConnectionError : public Exception {
	public:
		explicit ConnectionError (const std::string& what_arg)
			: Exception(what_arg) { }
	};
#endif
};

#endif /* __COMMAND_HANDLER_H */
