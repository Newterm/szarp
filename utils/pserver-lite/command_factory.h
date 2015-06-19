#ifndef __COMMAND_FACTORY_H
#define __COMMAND_FACTORY_H

#include <memory>
#include <string>
#include "commands.h"

class CommandFactory {
public:
	static std::shared_ptr<CommandHandler> make_cmd (const std::string& tag) {
		if (GetCommand::get_tag() == tag)
			return std::shared_ptr<CommandHandler>(new GetCommand());
		else if (SearchCommand::get_tag() == tag)
			return std::shared_ptr<CommandHandler>(new SearchCommand());
		else if (RangeCommand::get_tag() == tag)
			return std::shared_ptr<CommandHandler>(new RangeCommand());
		else
			return std::shared_ptr<CommandHandler>(nullptr);
	}
};

#endif /* __COMMAND_FACTORY_H */
