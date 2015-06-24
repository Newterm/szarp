#ifndef  __PSERVER_SERVICE_H
#define  __PSERVER_SERVICE_H

#include <string>

class PServerService
{
public:
	PServerService (void) { };
	~PServerService (void) { };

	void run(void);
	void process_request(std::string& msg_received);
};

#endif  /* __PSERVER_SERVICE_H */
