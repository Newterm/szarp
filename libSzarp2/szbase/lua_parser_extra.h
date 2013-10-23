#ifndef LUA_PARSER_EXTRA_H
#define LUA_PARSER_EXTRA_H

#include "config.h"

namespace qi = boost::spirit::qi;

namespace lua_grammar {

qi::rule<std::wstring::const_iterator, std::wstring()> keywords_rule();

struct lua_skip_parser : qi::grammar<std::wstring::const_iterator> {
	qi::rule<std::wstring::const_iterator> skip;
	lua_skip_parser();
};

qi::rule<std::wstring::const_iterator, bool(), lua_skip_parser> boolean_rule();

qi::rule<std::wstring::const_iterator, std::wstring(), lua_skip_parser> string_rule(
	qi::rule<std::wstring::const_iterator, wchar_t()>& escaped_character
	);

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

void operators_precedence(
	qi::rule<std::wstring::const_iterator, expression(), lua_skip_parser>& expression_,
	qi::rule<std::wstring::const_iterator, or_exp(), lua_skip_parser>& or_,
	qi::rule<std::wstring::const_iterator, and_exp(), lua_skip_parser>& and_,
	qi::rule<std::wstring::const_iterator, cmp_exp(), lua_skip_parser>& cmp_,
	qi::rule<std::wstring::const_iterator, concat_exp(), lua_skip_parser>& concat_,
	qi::rule<std::wstring::const_iterator, add_exp(), lua_skip_parser>& add_,
	qi::rule<std::wstring::const_iterator, mul_exp(), lua_skip_parser>& mul_,
	qi::rule<std::wstring::const_iterator, unop_exp(), lua_skip_parser>& unop_,
	qi::rule<std::wstring::const_iterator, pow_exp(), lua_skip_parser>& pow_,
	qi::rule<std::wstring::const_iterator, term(), lua_skip_parser>& term_,
	addop_symbols** addop_symbols_,
	cmpop_symbols** cmpop_symbols_,
	mulop_symbols** mulop_symbols_,
	unop_symbols** unop_symbols_
);


}

#endif
