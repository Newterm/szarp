/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include "config.h"

#ifdef LUA_PARAM_OPTIMISE

/*
  This is a parser for lua grammar as defined in http://www.lua.org/manual/5.1/manual.html#8
  The parser takes into account operator precedence. Parser in split into three files:
  ../include/szbase/lua_syntax.h: defines a data structers used by parser
  lua_prser.cc: this file, contains majority of parsing code
  lua_parer_extra.cc: addition file, containing part of a parser, this file exists
    because version of mingw present in debian unstable (as of this writing) was not able to compile
    parser contained in one file (compiler died on symbol table overflow)
*/
	
//uncomment to enable parser debugging
//#define BOOST_SPIRIT_DEBUG
#include <iostream>
#include <fstream>

#ifdef BOOST_SPIRIT_DEBUG
std::ofstream ofs("/tmp/boost_spirit_debug_out");
#define BOOST_SPIRIT_DEBUG_OUT ofs
#endif

#include <string>
namespace std {
ostream& operator<< (ostream& os, const wstring& s);
}
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include "szarp_base_common/lua_syntax.h"
#include "lua_parser_extra.h"
#include "szarp_base_common/lua_syntax_fusion_adapt.h"
#include "conversion.h"

namespace std {
ostream& operator<< (ostream& os, const wstring& s);
}

namespace lua_grammar {

namespace qi = boost::spirit::qi;

std::ostream& operator<< (std::ostream& os, const expression& e);

std::ostream& operator<< (std::ostream& os, const boost::recursive_wrapper<expression>& e);

std::ostream& operator<< (std::ostream& os, const wchar_t* s);

std::ostream& operator<< (std::ostream& os, const boost::recursive_wrapper<std::wstring>& s);

std::ostream& operator<<(std::ostream& os, const namelist& c );

std::ostream& operator<< (std::ostream& out, const nil& c );

std::ostream& operator<< (std::ostream& out, const threedots& c );

std::ostream& operator<< (std::ostream& out, const boost::recursive_wrapper<chunk>& c );

std::ostream& operator<< (std::ostream& out, const boost::recursive_wrapper<block>& c );

std::ostream& operator<< (std::ostream& out, const block& c );

std::ostream& operator<< (std::ostream& out, const parlist1& c );

std::ostream& operator<< (std::ostream& os, const pow_exp& e);

std::ostream& operator<< (std::ostream& os, const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &e);

std::ostream& operator<< (std::ostream& os, const unop_exp& e);

std::ostream& operator<< (std::ostream& os, const mul_exp& e);

std::ostream& operator<< (std::ostream& os, const add_exp & e);

std::ostream& operator<< (std::ostream& os, const concat_exp& e);

std::ostream& operator<< (std::ostream& os, const cmp_exp& e);

std::ostream& operator<< (std::ostream& os, const and_exp& e);

std::ostream& operator<< (std::ostream& os, const or_exp& e);

std::ostream& operator<< (std::ostream& os, const expression& e);

std::ostream& operator<< (std::ostream& os, const boost::tuple<expression, expression> & t);

std::ostream& operator<< (std::ostream& os, const boost::tuple<identifier, expression> & t);

std::ostream& operator<<(std::ostream& os, const boost::recursive_wrapper<std::vector<expression> >& v);

std::ostream& operator<<(std::ostream& os, const std::vector<field>& v);

std::ostream& operator<< (std::ostream& os, const tableconstructor& e);

std::ostream& operator<< (std::ostream& os, const chunk& e);

std::ostream& operator<< (std::ostream& os, const boost::tuple<identifier, args> & t);

std::ostream& operator<< (std::ostream& os, const boost::tuple<expression, namearg> & t);

std::ostream& operator<< (std::ostream& os, const funcbody& e);

std::ostream& operator<< (std::ostream& os, const var_seq& v);

std::ostream& operator<< (std::ostream& os, const varc& v);

std::ostream& operator<< (std::ostream& os, const assignment& a);

std::ostream& operator<< (std::ostream& os, const while_loop& l);

std::ostream& operator<< (std::ostream& os, const repeat_loop& l);

std::ostream& operator<< (std::ostream& os, const if_stat& if_);

std::ostream& operator<< (std::ostream& os, const for_from_to_loop& for_);

std::ostream& operator<< (std::ostream& os, const for_in_loop& for_);

std::ostream& operator<< (std::ostream& os, const funcname& f);

std::ostream& operator<< (std::ostream& os, const function_declaration& f);

std::ostream& operator<< (std::ostream& os, const return_& r);

std::ostream& operator<< (std::ostream& os, const break_& b);

std::ostream& operator<< (std::ostream& os, const std::vector<var>& v);

std::ostream& operator<< (std::ostream& os, const std::vector<expression>& v);

struct lua_parser : qi::grammar<std::wstring::const_iterator, chunk(), lua_skip_parser> {

	typedef lua_skip_parser space;

	escaped_symbol escaped_symbol_;

	qi::rule<std::wstring::const_iterator, std::wstring()> keywords;
	qi::rule<std::wstring::const_iterator, std::wstring()> keywordse;
	qi::rule<std::wstring::const_iterator, std::wstring(), space> identifier_;
	qi::rule<std::wstring::const_iterator, std::wstring(), space> string;
	qi::rule<std::wstring::const_iterator, bool(), space> boolean;
	qi::rule<std::wstring::const_iterator, nil(), space> nil_;
	qi::rule<std::wstring::const_iterator, wchar_t()> octal_char;
	qi::rule<std::wstring::const_iterator, wchar_t()> escaped_character;
	qi::rule<std::wstring::const_iterator, threedots(), space> threedots_;
	qi::rule<std::wstring::const_iterator, namelist(), space> namelist_;
	qi::rule<std::wstring::const_iterator, parlist1(), space> parlist1_;
	qi::rule<std::wstring::const_iterator, parlist(), space> parlist_;
	qi::rule<std::wstring::const_iterator, block(), space> block_;
	qi::rule<std::wstring::const_iterator, boost::tuple<identifier, expression>(), space> tuple_identifier_expression_;
	qi::rule<std::wstring::const_iterator, boost::tuple<expression, expression>(), space> tuple_expression_expression_;
	qi::rule<std::wstring::const_iterator, field(), space> field_;
	qi::rule<std::wstring::const_iterator, args(), space> args_;
	qi::rule<std::wstring::const_iterator, exp_identifier(), space> exp_identifier_;
	qi::rule<std::wstring::const_iterator, exp_identifier(), space> exp_identifier_square;
	qi::rule<std::wstring::const_iterator, boost::tuple<identifier, args>(), space> identifier_args_;
	qi::rule<std::wstring::const_iterator, namearg(), space> namearg_;
	qi::rule<std::wstring::const_iterator, var_seq(), space> var_seq_;
	qi::rule<std::wstring::const_iterator, varc(), space> varc_;
	qi::rule<std::wstring::const_iterator, var(), space> var_;
	qi::rule<std::wstring::const_iterator, boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> >(), space> exp_ident_arg_namearg_;
	qi::rule<std::wstring::const_iterator, postfixexp(), space> postfixexp_;
	qi::rule<std::wstring::const_iterator, funcbody(), space> funcbody_;
	qi::rule<std::wstring::const_iterator, term(), space> term_;
	qi::rule<std::wstring::const_iterator, expression(), space> expression_;
	qi::rule<std::wstring::const_iterator, or_exp(), space> or_;
	qi::rule<std::wstring::const_iterator, and_exp(), space> and_;
	qi::rule<std::wstring::const_iterator, cmp_exp(), space> cmp_;
	qi::rule<std::wstring::const_iterator, concat_exp(), space> concat_;
	qi::rule<std::wstring::const_iterator, add_exp(), space> add_;
	qi::rule<std::wstring::const_iterator, mul_exp(), space> mul_;
	qi::rule<std::wstring::const_iterator, unop_exp(), space> unop_;
	qi::rule<std::wstring::const_iterator, pow_exp(), space> pow_;
	qi::rule<std::wstring::const_iterator, assignment(), space> assignment_;
	qi::rule<std::wstring::const_iterator, block(), space> do_block;
	qi::rule<std::wstring::const_iterator, while_loop(), space> while_loop_;
	qi::rule<std::wstring::const_iterator, repeat_loop(), space> repeat_loop_;
	qi::rule<std::wstring::const_iterator, if_stat(), space> if_stat_;
	qi::rule<std::wstring::const_iterator, for_from_to_loop(), space> for_from_to_loop_;
	qi::rule<std::wstring::const_iterator, for_in_loop(), space> for_in_loop_;
	qi::rule<std::wstring::const_iterator, funcname(), space> funcname_;
	qi::rule<std::wstring::const_iterator, funcbody(), space> function_;
	qi::rule<std::wstring::const_iterator, function_declaration(), space> function_declaration_;
	qi::rule<std::wstring::const_iterator, local_assignment(), space> local_assignment_;
	qi::rule<std::wstring::const_iterator, local_function_declaration(), space> local_function_declaration_;
	qi::rule<std::wstring::const_iterator, break_(), space> break__;
	qi::rule<std::wstring::const_iterator, return_(), space> return__;
	qi::rule<std::wstring::const_iterator, stat(), space> stat_;
	qi::rule<std::wstring::const_iterator, laststat(), space> laststat_;
	qi::rule<std::wstring::const_iterator, chunk(), space> chunk_;
	qi::rule<std::wstring::const_iterator, tableconstructor(), space> tableconstructor_;
	qi::rule<std::wstring::const_iterator, std::vector<var>(), space> varlist_;
	qi::rule<std::wstring::const_iterator, std::vector<field>(), space> fieldlist_;
	qi::rule<std::wstring::const_iterator, std::vector<expression>(), space> explist_;
	qi::rule<std::wstring::const_iterator, std::vector<expression>(), space> eexplist_;
	qi::rule<std::wstring::const_iterator, space> fieldsep_;

	addop_symbols* addop_symbols_;
	cmpop_symbols* cmpop_symbols_;
	mulop_symbols* mulop_symbols_;
	unop_symbols* unop_symbols_;
public:
	lua_parser() : lua_parser::base_type(chunk_) {
		using qi::lit;
		using qi::lexeme;
		using qi::_val;
		using qi::_1;
		using qi::eps;
		using boost::spirit::standard_wide::char_;

		//actual grammar definition

		//keywords rule sucked from lua_parser_extra.cc
		keywords = keywords_rule();

		//identifier is a string that is not a keyword
		keywordse = keywords >> !char_(L"a-zA-Z0-9_");
		identifier_ = lexeme[ char_(L"a-zA-Z_") >> *(char_(L"a-zA-Z0-9_")) ] - keywordse;

		//pretty strightforward octal char definition
		octal_char = eps [_val = 0] >> 
			char_(L"0-7") [ _val = (_1 - L'0') * 64 ] >>
			char_(L"0-7") [ _val += (_1 - L'0') * 8] >>
			char_(L"0-7") [ _val += (_1 - L'0')];

		//esacped characters
		escaped_character = ( lit(L'\\') >> (escaped_symbol_ | octal_char) ) | char_;

		//three dots term
		threedots_ = lit(L"...") [_val = threedots() ];

		//string handling
		string = string_rule(escaped_character);

		//break term
		break__ = lit(L"break") [_val = break_()];

		//boolean constant
		boolean = boolean_rule();

		//nil constant
		nil_ = lit(L"nil")[_val = nil()];

		//return expression parser
		return__ = (L"return" >> eexplist_) [_val = _1];

		//chunk ::= {stat [`;´]} [laststat [`;´]]
		chunk_ = *(stat_ >> -lit(L";")) >> -(laststat_ >> -lit(L";"));

		//block ::= chunk
		block_ = chunk_[_val =  _1];

		//explist ::= {exp `,´} exp
		explist_ = expression_ % L',';

		//potentially empty expresion list
		eexplist_ = expression_ % L',' | eps;

		//potentially empty variables list
		varlist_ = var_ % L',' | eps;

		//fieldlist ::= field {fieldsep field} [fieldsep]
		//tableconstructor ::= `{´ [fieldlist] `}´
		fieldlist_ = field_  % fieldsep_ | eps;
		tableconstructor_ = (L"{" >> fieldlist_ >> L'}') [ _val = _1];

		//args ::=  `(´ [explist] `)´ | tableconstructor | String 
		args_ = lit(L'(') >> eexplist_ >> lit(L')')
			| tableconstructor_
			| string;

/*	
		stat ::=  varlist `=´ explist | 
			functioncall | 
			do block end | 
			while exp do block end | 
			repeat block until exp | 
			if exp then block {elseif exp then block} [else block] end | 
			for Name `=´ exp `,´ exp [`,´ exp] do block end | 
			for namelist in explist do block end | 
			function funcname funcbody | 
			local function Name funcbody | 
			local namelist [`=´ explist] 
*/

		assignment_ = varlist_ >> L"=" >> explist_;

		while_loop_ = L"while" >> expression_ >> L"do" >> block_ >> L"end";

		do_block = L"do" >> block_ >> L"end";

		repeat_loop_ = L"repeat" >> block_ >> L"until" >> expression_;

		if_stat_ = L"if" >> expression_ >> L"then" >> block_ >> *(L"elseif" >> expression_ >> L"then" >> block_) >> -(L"else" >> block_) >> L"end";

		for_from_to_loop_ = L"for" >> identifier_ >> L"=" >> expression_ >> L"," >> expression_ >> -(L"," >> expression_) >> L"do" >> block_ >> L"end";

		for_in_loop_ = L"for" >> namelist_ >> L"in" >> explist_ >> L"do" >> block_ >> L"end";

		funcname_ = L"function" >> (identifier_ % L".") >> -(L":" >> identifier_);

		function_declaration_ = L"function" >> funcname_ >> funcbody_;

		local_assignment_ = L"local" >> varlist_ >> -(L"=" >> explist_);

		local_function_declaration_ = lit(L"local") >> L"function" >> funcname_ >> funcbody_;

		stat_ = do_block
			| while_loop_
			| repeat_loop_ 
			| if_stat_
			| for_in_loop_ 
			| for_from_to_loop_
			| local_assignment_
			| local_function_declaration_
			| assignment_ 
			| function_declaration_
			| postfixexp_;

		//laststat ::= return [explist] | break 
		laststat_ = return__ | break__;

		//namelist ::= Name {`,´ Name}
		namelist_ = (identifier_ % L',' )[_val = _1] | eps;
		
		function_ = L"function" >> funcbody_;

		/* exp ::=  nil | false | true | Number | String | `...´ | function | 
			prefixexp | tableconstructor | exp binop exp | unop exp 
		  we remove left recursion and add operator precedence
		*/

		term_ = nil_
			| qi::double_
			| boolean
			| string
			| threedots_
			| function_
			| postfixexp_
			| tableconstructor_
			| L'(' >> expression_ >> L')';

		//operator precedence
		operators_precedence(
			expression_,
			or_,
			and_,
			cmp_,
			concat_,
			add_,
			mul_,
			unop_,
			pow_,
			term_,
			&addop_symbols_,
			&cmpop_symbols_,
			&mulop_symbols_,
			&unop_symbols_
			);
		/*
		 We have following rules that cannot be directly fed to spirit
  		 var ::=  Name | prefixexp `[´ exp `]´ | prefixexp `.´ Name 
 		 prefixexp ::= var | functioncall | `(´ exp `)´
		 functioncall ::=  prefixexp args | prefixexp `:´ Name args 
		 We subsitute them with the following:
		*/

		exp_identifier_ = L'(' >> expression_ >> L')' | identifier_;

		exp_identifier_square = L"[" >> expression_ >> L"]" | L"." >> identifier_;

		identifier_args_ = L":" >> identifier_ >> args_;

		namearg_ = args_ | identifier_args_; 

		exp_ident_arg_namearg_ = exp_identifier_ >> +(exp_identifier_square | namearg_);

		postfixexp_ = exp_ident_arg_namearg_ | identifier_;

		var_seq_ = *namearg_ >> exp_identifier_square;

		varc_ = exp_identifier_ >> +var_seq_;

		var_ = varc_ | identifier_;

		/*
		 so our parser does not distinguish directly between functioncall, variable and array's
		 element access, expressions: something[], something(), and something1.something2
		 are all recognized as postfixexp_.
		 var_ rule is very similar to postfixexp_ but refers only to expressions
		 that can be on the left hand side of assignements: variables and array elements
		*/
		

		funcbody_ = L'(' >> -parlist_ >> L')' >> block_ >> L"end";
		
		parlist1_ = namelist_ >> -(L"," >> threedots_);

		parlist_ = parlist1_ | threedots_;

		tuple_expression_expression_ = L'[' >> expression_ >> L']' >> L'=' >> expression_;

		tuple_identifier_expression_ = identifier_ >> L'=' >> expression_;

		field_ = tuple_expression_expression_
			| tuple_identifier_expression_
			| expression_;

		fieldsep_ = lit(L",") | lit(L";");

		using qi::debug;

		BOOST_SPIRIT_DEBUG_NODE(identifier_);
		BOOST_SPIRIT_DEBUG_NODE(string);
		BOOST_SPIRIT_DEBUG_NODE(boolean);
		BOOST_SPIRIT_DEBUG_NODE(nil_);
		BOOST_SPIRIT_DEBUG_NODE(octal_char);
		BOOST_SPIRIT_DEBUG_NODE(escaped_character);
		BOOST_SPIRIT_DEBUG_NODE(threedots_);
		BOOST_SPIRIT_DEBUG_NODE(namelist_);
		BOOST_SPIRIT_DEBUG_NODE(parlist1_);
		BOOST_SPIRIT_DEBUG_NODE(parlist_);
		BOOST_SPIRIT_DEBUG_NODE(block_);
		BOOST_SPIRIT_DEBUG_NODE(tuple_identifier_expression_);
		BOOST_SPIRIT_DEBUG_NODE(tuple_expression_expression_);
		BOOST_SPIRIT_DEBUG_NODE(field_);
		BOOST_SPIRIT_DEBUG_NODE(args_);
		BOOST_SPIRIT_DEBUG_NODE(exp_identifier_);
		BOOST_SPIRIT_DEBUG_NODE(identifier_args_);
		BOOST_SPIRIT_DEBUG_NODE(namearg_);
		BOOST_SPIRIT_DEBUG_NODE(var_seq_);
		BOOST_SPIRIT_DEBUG_NODE(varc_);
		BOOST_SPIRIT_DEBUG_NODE(var_);
		BOOST_SPIRIT_DEBUG_NODE(funcbody_);
		BOOST_SPIRIT_DEBUG_NODE(expression_);
		BOOST_SPIRIT_DEBUG_NODE(assignment_);
		BOOST_SPIRIT_DEBUG_NODE(do_block);
		BOOST_SPIRIT_DEBUG_NODE(while_loop_);
		BOOST_SPIRIT_DEBUG_NODE(repeat_loop_);
		BOOST_SPIRIT_DEBUG_NODE(if_stat_);
		BOOST_SPIRIT_DEBUG_NODE(for_from_to_loop_);
		BOOST_SPIRIT_DEBUG_NODE(for_in_loop_);
		BOOST_SPIRIT_DEBUG_NODE(funcname_);
		BOOST_SPIRIT_DEBUG_NODE(function_);
		BOOST_SPIRIT_DEBUG_NODE(function_declaration_);
		BOOST_SPIRIT_DEBUG_NODE(local_assignment_);
		BOOST_SPIRIT_DEBUG_NODE(local_function_declaration_);
		BOOST_SPIRIT_DEBUG_NODE(break__);
		BOOST_SPIRIT_DEBUG_NODE(return__);
		BOOST_SPIRIT_DEBUG_NODE(stat_);
		BOOST_SPIRIT_DEBUG_NODE(laststat_);
		BOOST_SPIRIT_DEBUG_NODE(chunk_);
		BOOST_SPIRIT_DEBUG_NODE(tableconstructor_);
		BOOST_SPIRIT_DEBUG_NODE(varlist_);
		BOOST_SPIRIT_DEBUG_NODE(fieldlist_);
		BOOST_SPIRIT_DEBUG_NODE(explist_);
		BOOST_SPIRIT_DEBUG_NODE(eexplist_);
		BOOST_SPIRIT_DEBUG_NODE(fieldsep_);
		BOOST_SPIRIT_DEBUG_NODE(expression_);
		BOOST_SPIRIT_DEBUG_NODE(or_);
		BOOST_SPIRIT_DEBUG_NODE(and_);
		BOOST_SPIRIT_DEBUG_NODE(cmp_);
		BOOST_SPIRIT_DEBUG_NODE(concat_);
		BOOST_SPIRIT_DEBUG_NODE(add_);
		BOOST_SPIRIT_DEBUG_NODE(mul_);
		BOOST_SPIRIT_DEBUG_NODE(unop_);
		BOOST_SPIRIT_DEBUG_NODE(pow_);
		BOOST_SPIRIT_DEBUG_NODE(term_);
		BOOST_SPIRIT_DEBUG_NODE(postfixexp_);
		BOOST_SPIRIT_DEBUG_NODE(keywords);
		BOOST_SPIRIT_DEBUG_NODE(keywordse);
		//BOOST_SPIRIT_DEBUG_NODE(qi::double_);
	}

	~lua_parser() {
		delete addop_symbols_;
		delete cmpop_symbols_;
		delete mulop_symbols_;
		delete unop_symbols_;
	}
};

bool parse(std::wstring::const_iterator& iter, std::wstring::const_iterator &end, chunk& chunk_) {
	lua_parser _lua_parser;
	lua_skip_parser _lua_skip_parser;
	return qi::phrase_parse(iter, end, _lua_parser, _lua_skip_parser, chunk_);
}

block& block::operator=(const boost::recursive_wrapper<chunk>& c) { chunk_ = c.get(); return *this; }

}

#endif
