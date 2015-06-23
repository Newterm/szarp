#ifndef  __PSERVER_SERVICE_H
#define  __PSERVER_SERVICE_H

#include <string>
#include <stdexcept>

class PServerService
{
public:
	PServerService (void) { };
	~PServerService (void) { };

	void run(void);
	void process_request(std::string& msg_rcvd);

	/* Exception class definitions */
	class Exception : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};
	class ParseError : public Exception {
		using Exception::Exception;
	};
};

#endif  /* __PSERVER_SERVICE_H */
