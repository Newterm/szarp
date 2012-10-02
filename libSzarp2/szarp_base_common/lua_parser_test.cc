#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "conversion.h"
#include "lua_syntax.h"

int main(int argc, char *argv[]) {
	std::cout << "\t\tLua parser \n\n";
	std::cout << "Give a file name to parse...or [q or Q] to quit\n\n";


	typedef std::string::const_iterator iterator_type;
	std::string str;
	while (std::getline(std::cin, str))
	{
		if (str.empty() || str[0] == 'q' || str[0] == 'Q')
			break;
		std::ifstream ff(str.c_str());
		std::stringstream ss;
		ss << ff.rdbuf();

		lua_grammar::chunk result;
		std::wstring wstr = SC::U2S((const unsigned char*)ss.str().c_str());
		std::wstring::const_iterator iter = wstr.begin();
		std::wstring::const_iterator end = wstr.end();
		std::cout << "Parsing: " << ss.str() << std::endl;
		bool r = lua_grammar::parse(iter, end, result);

		if (r && iter == end)
		{
			std::cout << "-------------------------\n";
			std::cout << "Parsing succeeded\n";
			std::cout << "Number of statements:" << result.stats.size() << std::endl;
			std::cout << "Is last statement:" << std::string(result.laststat_ ? "yes" : "no") << std::endl;

			std::cout << "-------------------------\n";
		}
		else
		{
			std::string rest(iter, end);
			std::cout << "-------------------------\n";
			std::cout << "Parsing failed\n";
			std::cout << "stopped at: \": " << rest << "\"\n";
			std::cout << "-------------------------\n";
		}
	}

	std::cout << "Bye... :-) \n\n";
	return 0;
}
