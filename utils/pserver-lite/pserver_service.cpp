
#include <boost/algorithm/string.hpp>
#include "pserver_service.h"
#include "command_factory.h"

void PServerService::run (void)
{
	/* TODO: main server loop */;
}

void PServerService::process_request (std::string& msg_rcvd)
{
	/* check terminate string CRLF */
	auto len = msg_rcvd.length();
	if (len < 2 || msg_rcvd[len-2] != '\r' || msg_rcvd[len-1] != '\n') {
		std::cout << "no proper end sign CRLF" << std::endl;
		return;
	}
	msg_rcvd.resize(len-2);

	/* tokenize received line */
	std::vector<std::string> toks;
	boost::split(toks, msg_rcvd, boost::is_any_of("\t "));

	if (0 == toks.size()) {
		std::cout << "too few tokens" << std::endl;
		return;
	}

	/* fetch aommand tag and arguments */
	std::string cmd_tag = toks.front();
	std::vector<std::string> args;

	if (cmd_tag.length() == 0) {
		std::cout << "command empty" << std::endl;
		return;
	}

	for (auto beg = ++toks.begin(); beg != toks.end(); ++beg) {
		if (0 != (*beg).length())
			args.push_back(*beg);
	}

	/* execute command */
	auto cmd_hdlr = CommandFactory::make_cmd(cmd_tag);

	if (cmd_hdlr) {
		cmd_hdlr->parse_args(args);
	}
	else {
		std::cout << "bad command" << std::endl;
	}

	/* TODO: send anwser to client */
}
