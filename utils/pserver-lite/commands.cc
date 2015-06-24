
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "commands.h"

/*** Static methods (Command Handler) ***/

std::vector<std::string> CommandHandler::tokenize (std::string& msg_received)
{
	/* check terminate string CRLF */
	auto len = msg_received.length();
	if (len < 2 || msg_received[len-2] != '\r' || msg_received[len-1] != '\n') {
		throw ParseError("no proper EOL sign, line not ended with CRLF");
	}
	msg_received.resize(len-2);

	/* tokenize received line */
	std::vector<std::string> tokens;
	boost::split(tokens, msg_received, boost::is_any_of("\t "),
			boost::algorithm::token_compress_on);

	if (0 == tokens.size()) {
		throw ParseError("too few tokens in line"); // probably won't happen
	}

	return tokens;
}


/*** Command implementations (CommandHandler derivatives) ***/

/** "GET" Command **/
void GetCommand::parse_args (const std::vector<std::string>& args)
{
	std::cout << "Command: " << get_tag() << "\n";
	std::cout << "Args:    ";

	for (auto &a : args) {
		std::cout << a << ", ";
	}
	std::cout << std::endl;
}

/** "SEARCH" Command **/
void SearchCommand::parse_args (const std::vector<std::string>& args)
{
	std::cout << "Command: " << get_tag() << "\n";
	std::cout << "Args:    ";

	for (auto &a : args) {
		std::cout << a << ", ";
	}
	std::cout << std::endl;
}

/** "RANGE" Command **/
void RangeCommand::parse_args (const std::vector<std::string>& args)
{
	std::cout << "Command: " << get_tag() << "\n";
	std::cout << "Args:    NONE (" << args.size() << ")" << std::endl;
}
