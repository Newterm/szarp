
#include <iostream>
#include "commands.h"

/* "GET" Command */
const std::string GetCommand::tag = "GET";

void GetCommand::parse_args (const std::vector<std::string>& args)
{
	std::cout << "Command: " << tag << "\n";
	std::cout << "Args:    ";

	for (auto &a : args) {
		std::cout << a << ", ";
	}
	std::cout << std::endl;
}

/* "SEARCH" Command */
const std::string SearchCommand::tag = "SEARCH";

void SearchCommand::parse_args (const std::vector<std::string>& args)
{
	std::cout << "Command: " << tag << "\n";
	std::cout << "Args:    ";

	for (auto &a : args) {
		std::cout << a << ", ";
	}
	std::cout << std::endl;
}

/* "RANGE" Command */
const std::string RangeCommand::tag = "RANGE";

void RangeCommand::parse_args (const std::vector<std::string>& args)
{
	std::cout << "Command: " << tag << "\n";
	std::cout << "Args:    NONE (" << args.size() << ")" << std::endl;
}
