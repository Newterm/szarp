#ifndef LUA_PARSER_EXTRA_H
#define LUA_PARSER_EXTRA_H

#include "config.h"

namespace qi = boost::spirit::qi;

qi::rule<std::wstring::const_iterator, std::wstring()> get_keywords_rule();

namespace lua_grammar {

struct lua_skip_parser : qi::grammar<std::wstring::const_iterator> {
	qi::rule<std::wstring::const_iterator> skip;
	lua_skip_parser();
};

}

struct mulop_symbols : qi::symbols<wchar_t, lua_grammar::mul_op> {
	mulop_symbols ();
};

struct addop_symbols : qi::symbols<wchar_t, lua_grammar::add_op> {
	addop_symbols();
};

struct cmpop_symbols : qi::symbols<wchar_t, lua_grammar::cmp_op> {
	cmpop_symbols();
};

struct unop_symbols : qi::symbols<wchar_t, lua_grammar::un_op> {
	unop_symbols ();
};

struct escaped_symbol : qi::symbols<wchar_t, wchar_t> {
	escaped_symbol();
};

#endif
