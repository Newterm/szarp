#ifndef __COMMAND_FACTORY_H
#define __COMMAND_FACTORY_H

#include <memory>
#include <string>
#include "commands.h"

class CommandFactory {
public:
	static std::shared_ptr<CommandHandler> make_cmd (const std::string& tag) {
		if (tag.length() == 0)
			throw CommandHandler::ParseError("command tag is empty");
		if (GetCommand::get_tag() == tag)
			return std::shared_ptr<CommandHandler>(new GetCommand());
		else if (SearchCommand::get_tag() == tag)
			return std::shared_ptr<CommandHandler>(new SearchCommand());
		else if (RangeCommand::get_tag() == tag)
			return std::shared_ptr<CommandHandler>(new RangeCommand());
		else
			throw CommandHandler::ParseError("no such command");
	}
};

#endif /* __COMMAND_FACTORY_H */
