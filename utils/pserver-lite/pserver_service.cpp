
#include "pserver_service.h"
#include "command_factory.h"

void PServerService::run (void)
{
	/* TODO: main server loop */;
}

void PServerService::process_request (std::string& msg_received)
{
	auto tokens = CommandHandler::tokenize(msg_received);

	/* fetch command tag and remove it from the vector of tokens */
#if GCC_VERSION >= 40900
	auto iter = tokens.cbegin();
#else
	auto iter = tokens.begin();
#endif
	std::string cmd_tag = *iter;
	tokens.erase(iter);

	/* execute command */
	auto cmd_handler = CommandFactory::make_cmd(cmd_tag);

	cmd_handler->load_args(tokens);
	cmd_handler->exec();
}
