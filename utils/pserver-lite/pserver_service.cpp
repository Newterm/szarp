
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
		throw ParseError("no proper EOL sign, line not ended with CRLF");
	}
	msg_rcvd.resize(len-2);

	/* tokenize received line */
	std::vector<std::string> toks;
	boost::split(toks, msg_rcvd, boost::is_any_of("\t "),
			boost::algorithm::token_compress_on);

	if (0 == toks.size()) {
		throw ParseError("too few tokens in line"); // probably won't happen
	}

	/* fetch command tag and arguments */
	std::string cmd_tag = toks.front();
	if (cmd_tag.length() == 0) {
		throw ParseError("command tag is empty, line begins with whitespaces");
	}

	std::vector<std::string> args(++toks.begin(), toks.end());

	/* execute command */
	auto cmd_hdlr = CommandFactory::make_cmd(cmd_tag);

	if (cmd_hdlr) {
		cmd_hdlr->parse_args(args);
	}
	else {
		throw ParseError("no such command");
	}

	/* TODO: send answer to client */
}
