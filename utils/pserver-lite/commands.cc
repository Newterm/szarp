
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
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
		throw ParseError("too few tokens in line");
		// probably won't happen as "" gives one empty token
	}

	return tokens;
}


/*** Command implementations (CommandHandler derivatives) ***/

/** "GET" Command **/
GetCommand::GetCommand (void)
	: m_ready(false), m_start_time(0), m_end_time(0), m_param_path() { }

void GetCommand::load_args (const std::vector<std::string>& args)
{
	m_ready = false;

	if (args.size() != 3)
		throw ArgumentError("(GET) bad number of arguments");

	/* load arguments 1 and 2 (start_time, end_time) */
	try {
		m_start_time = boost::lexical_cast<time_t>(args[0]);
		m_end_time = boost::lexical_cast<time_t>(args[1]);
	}
	catch (const boost::bad_lexical_cast &) {
		throw ArgumentError("(GET) cannot cast argument 1 or 2 to time_t");
	}

	if (m_start_time > m_end_time)
		throw ArgumentError("(GET) bad argument 1 and 2, start > end");

	/* load argument 3 (param_path) */
	if (args[2].length() == 0)
		throw ArgumentError("(GET) bad argument 3, parameter path is zero length");

	m_param_path = args[2];

	/* arguments successfully loaded */
	m_ready = true;
}

void GetCommand::exec (void)
{
	if (! m_ready)
		throw ArgumentError("cannot exec(), arguments not loaded");

	/* TODO: read data from cache and return answer */
}

/** "SEARCH" Command **/
SearchCommand::SearchCommand (void)
	: m_ready(false), m_start_time(0), m_end_time(0), m_direction(0),
	  m_param_path() { }

void SearchCommand::load_args (const std::vector<std::string>& args)
{
	m_ready = false;

	if (args.size() != 4)
		throw ArgumentError("(SEARCH) bad number of arguments");

	/* load arguments 1 and 2 (start_time, end_time) */
	try {
		m_start_time = boost::lexical_cast<time_t>(args[0]);
		m_end_time = boost::lexical_cast<time_t>(args[1]);
	}
	catch (const boost::bad_lexical_cast &) {
		throw ArgumentError("(SEARCH) cannot cast argument 1 or 2 to time_t");
	}

	if (m_start_time > m_end_time)
		throw ArgumentError("(SEARCH) bad argument 1 and 2, start > end");

	/* load argument 3 (direction) */
	try {
		m_direction = boost::lexical_cast<int>(args[2]);
	}
	catch (const boost::bad_lexical_cast &) {
		throw ArgumentError("(SEARCH) cannot cast argument 1 or 2 to time_t");
	}

	if (m_direction < -1 || m_direction > 1)
		throw ArgumentError("(SEARCH) bad argument 3, direction not in {-1, 0, 1}.");

	/* load argument 4 (param_path) */
	if (args[3].length() == 0)
		throw ArgumentError("(SEARCH) bad argument 4, parameter path is zero length");

	m_param_path = args[3];

	/* arguments successfully loaded */
	m_ready = true;
}

void SearchCommand::exec (void)
{
	if (! m_ready)
		throw ArgumentError("cannot exec(), arguments not loaded");

	/* TODO: read data from cache and return answer */
}

/** "RANGE" Command **/
void RangeCommand::load_args (const std::vector<std::string>& args)
{
	if (args.size() != 0)
		throw ArgumentError("(RANGE) bad number of arguments");
}

void RangeCommand::exec (void)
{
	/* TODO: read data from cache and return answer */
}
