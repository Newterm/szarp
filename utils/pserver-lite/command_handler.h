#ifndef __COMMAND_HANDLER_H
#define __COMMAND_HANDLER_H

#include <string>
#include <vector>

class CommandHandler {
public:
	virtual void parse_args (const std::vector<std::string>& args) = 0;
};

#endif /* __COMMAND_HANDLER_H */
