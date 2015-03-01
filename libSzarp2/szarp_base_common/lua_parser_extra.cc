#include "config.h"

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

namespace lua_grammar {


qi::rule<std::wstring::const_iterator, std::wstring()> keywords_rule() {
	qi::rule<std::wstring::const_iterator, std::wstring()> rule = 
		qi::lit(L"and")
			| L"break"
			| L"do"
			| L"elseif"
			| L"else"
			| L"end"   
			| L"false"
			| L"for"
			| L"function"
			| L"if"
			| L"in"
			| L"local"
			| L"nil"
			| L"not"
			| L"or"
			| L"repeat"
			| L"return"
			| L"then"
			| L"true"
			| L"until"
			| L"while";

	return rule;
}

qi::rule<std::wstring::const_iterator, bool(), lua_skip_parser> boolean_rule() {
	namespace qi = boost::spirit::qi;
	using qi::lit;
	using qi::_val;

	return lit(L"true")[_val = true] | lit(L"false")[_val = false];
}

qi::rule<std::wstring::const_iterator, std::wstring(), lua_skip_parser> string_rule(
	qi::rule<std::wstring::const_iterator, wchar_t()>& escaped_character
	)
{
	using boost::spirit::standard_wide::char_;
	using qi::lexeme;
	return lexeme[L"'" >> *(escaped_character - L"'") >> L"'"]
		| lexeme[L"\"" >> *(escaped_character - L"\"") >> L"\""]
		| lexeme[L"[[" >> *(char_ - L"]]") >> L"]]"]; 

}

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
)
{
	using qi::_1;
	using qi::_val;

	*addop_symbols_ = new addop_symbols();
	*cmpop_symbols_ = new cmpop_symbols();
	*mulop_symbols_ = new mulop_symbols();
	*unop_symbols_ = new unop_symbols();

	expression_ = or_ [_val = _1];
	or_ = and_ % L"or";
	and_ =  cmp_ % L"and";
	cmp_ = concat_ >> *((**cmpop_symbols_) >> concat_);
	concat_ = add_ % L"..";
	add_ = mul_ >> *((**addop_symbols_) >> mul_);
	mul_ = unop_ >> *((**mulop_symbols_) >> unop_);
	unop_ = *(**unop_symbols_) >> pow_;
	pow_ = term_ % L"^";
}

lua_skip_parser::lua_skip_parser() : lua_skip_parser::base_type(skip) {

		using boost::spirit::standard_wide::char_;
		using boost::spirit::standard_wide::space;

		skip = qi::space
			| L"--[[" >> *(char_ - L"]]") >> L"]]"
			| L"--" >> *(char_ - (qi::eol | qi::eoi)) >> (qi::eol | qi::eoi);
			;

	}	


mulop_symbols::mulop_symbols() {
	add(L"*", lua_grammar::MUL)
		(L"/", lua_grammar::DIV)
		(L"%", lua_grammar::REM);
}

addop_symbols::addop_symbols() {
	add(L"+", lua_grammar::PLUS)
		(L"-", lua_grammar::MINUS);
}

cmpop_symbols::cmpop_symbols() {
	add(L"<", lua_grammar::LT)
		(L"<=", lua_grammar::LTE)
		(L">", lua_grammar::GT)
		(L">=", lua_grammar::GTE)
		(L"==", lua_grammar::EQ)
		(L"~=", lua_grammar::NEQ);
}

unop_symbols::unop_symbols () {
	add(L"-", lua_grammar::NEG)
		(L"not", lua_grammar::NOT)
		(L"#", lua_grammar::LEN);
}

escaped_symbol::escaped_symbol() {
	add(L"a", L'\a')
		(L"b", L'\b')
		(L"f", L'\f')
		(L"n", L'\n')
		(L"t", L'\t')
		(L"r", L'\r')
		(L"v", L'\v')
		(L"\\", L'\\')
		(L"\'", L'\'')
		(L"\"", L'\"');
}

}

namespace std {
ostream& operator<< (ostream& os, const wstring& s) {
	return os << SC::S2A(s);
}

}

namespace lua_grammar {

chunk::chunk() {}

chunk::chunk(const block& block_) : stats(block_.chunk_.get().stats), laststat_(block_.chunk_.get().laststat_) {}

namelist& namelist::operator=(const namelist& v) {
	namelist_ = v.namelist_;
	return *this;
}


namelist& namelist::operator=(const std::vector<std::wstring>& v) {
	namelist_ = v;
	return *this;
}

tableconstructor& tableconstructor::operator=(const tableconstructor& t) {
	tableconstructor_ = t.tableconstructor_;
	return *this;
}

tableconstructor& tableconstructor::operator=(const std::vector<field>& v) {
	tableconstructor_ = v;
	return *this;
}



std::ostream& operator<< (std::ostream& os, const boost::recursive_wrapper<expression>& e) {
	return os << e.get();
}

std::ostream& operator<< (std::ostream& os, const wchar_t* s) {
	return os << SC::S2A(s);
}

std::ostream& operator<< (std::ostream& os, const boost::recursive_wrapper<std::wstring>& s) {
	return os << SC::S2A(s.get());
}

std::ostream& operator<<(std::ostream& os, const namelist& c ) {
	for (size_t i = 0; i < c.namelist_.size(); i++)
		os << c.namelist_[i] << " ";
	return os;
}

std::ostream& operator<< (std::ostream& out, const nil& c ) {
	return out << "nil";
}

std::ostream& operator<< (std::ostream& out, const threedots& c ) {
	return out << "threedots";
}

std::ostream& operator<< (std::ostream& out, const chunk& c ) {
	for (size_t i = 0; i < c.stats.size(); i++)
		out << " " << c.stats[i];	
	if (c.laststat_)
		out << "last stat " << *c.laststat_;
	return out;
}

std::ostream& operator<< (std::ostream& out, const block& c ) {
	return out << c.chunk_.get();
}

std::ostream& operator<< (std::ostream& out, const boost::recursive_wrapper<chunk>& c ) {
	return out << c.get();
}

std::ostream& operator<< (std::ostream& out, const boost::recursive_wrapper<block>& c ) {
	return out << c.get();
}


std::ostream& operator<< (std::ostream& out, const parlist1& c ) {
	out << c.namelist_;
	if (c.threedots_)
		out << " " << *c.threedots_;
	out << " ";
	return out;
}

std::ostream& operator<< (std::ostream& os, const pow_exp& e) {
	if (e.size() > 1)
		os << "pow exp:";
	for (size_t i = 0; i < e.size(); i++)
		os << " " << e[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &e) {
	os << e.get<0>();
	for (size_t i = 0; i < e.get<1>().size(); i++)
		os << " " << e.get<1>()[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const unop_exp& e) {
	if (e.get<0>().size()) {
		os << "unop exp: ";
		for (size_t i = 0; i < e.get<0>().size(); i++)
			//os << e.get<0>()[i] << " ";
			os << "unop op ";
	}
	return os << e.get<1>();
}

std::ostream& operator<< (std::ostream& os, const mul_exp& e) {
	os << e.unop;
	for (size_t i = 0; i < e.muls.size(); i++)
		os << " mul op " << e.muls[i].get<1>();
	return os;
}

std::ostream& operator<< (std::ostream& os, const add_exp & e) {
	os << e.mul;
	for (size_t i = 0; i < e.adds.size(); i++)
		os << " add op " << e.adds[i].get<1>();
	return os;
}

std::ostream& operator<< (std::ostream& os, const concat_exp& e) {
	os << e[0];
	for (size_t i = 1; i < e.size(); i++)
		os << " .. " << e[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const cmp_exp& e) {
	os << e.concat;
	for (size_t i = 0; i < e.cmps.size(); i++)
		os << " cmp op " << e.cmps[i].get<1>();
	return os;
}

std::ostream& operator<< (std::ostream& os, const and_exp& e) {
	os << e[0].get();
	for (size_t i = 1; i < e.size(); i++)
		os << " and " << e[i].get();
	return os;
}

std::ostream& operator<< (std::ostream& os, const or_exp& e) {
	os << e[0];
	for (size_t i = 1; i < e.size(); i++)
		os << " or " << e[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const boost::tuple<expression, expression> & t) {
	return os << t.get<0>() << " " << t.get<1>();
}

std::ostream& operator<< (std::ostream& os, const boost::tuple<identifier, expression> & t) {
	return os << t.get<0>() << " " << t.get<1>();
}

std::ostream& operator<<(std::ostream& os, const boost::recursive_wrapper<std::vector<expression> >& v) {
	for (size_t i = 0; i < v.get().size(); i++)
		os << " " << v.get()[i];
	return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<field>& v) {
	for (size_t i = 0; i < v.size(); i++)
		os << " " << v[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const tableconstructor& e) {
	for (size_t i = 0; i < e.tableconstructor_.size(); i++)
		os << e.tableconstructor_[i] << " ";
	return os;
}

std::ostream& operator<< (std::ostream& os, const boost::tuple<identifier, args> & t) {
	return os << t.get<0>() << " " << t.get<1>();
}

std::ostream& operator<< (std::ostream& os, const boost::tuple<expression, namearg> & t) {
	os << t.get<0>();
	return os << " " << t.get<1>();
}

std::ostream& operator<< (std::ostream& os, const funcbody& e) {
	os << e.parlist_;
	os << e.block_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const var_seq& v) {
	for (size_t i = 0; i < v.nameargs.size(); i++) {
		os << " " << v.nameargs[i];
	}
	os << " " << v.exp_identifier_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const varc& v) {
	os << v.exp_identifier_;
	for (size_t i = 0; i < v.var_seqs.size(); i++)
		os << " " << v.var_seqs[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const assignment& a) {
	for (size_t i = 0; i < a.varlist.size(); i++)
		os << " " << a.varlist[i];
	for (size_t i = 0; i < a.explist.size(); i++) 
		os << " " << a.explist[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const while_loop& l) {
	os << l.expression_;
	os << " " << l.block_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const repeat_loop& l) {
	os << l.expression_;
	os << " " << l.block_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const if_stat& if_) {
	os << if_.if_exp;
	os << " " << if_.block_;
	for (size_t i = 0; i < if_.elseif_.size(); i++) {
		os << " elseif " << if_.elseif_[i].get<0>();
		os << " " << if_.elseif_[i].get<1>();

	}
	if (if_.else_)
		os << " else " << *if_.else_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const for_from_to_loop& for_) {
	os << for_.identifier_;
	os << " " << for_.from;
	os << " " << for_.to;
	if (for_.step)
		os << " " << for_.step;
	os << " " << for_.block_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const for_in_loop& for_) {
	os << for_.namelist_;
	for (size_t i = 0; i < for_.expressions.size(); i++)
		os << " " << for_.expressions[i];
	os << " " << for_.block_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const funcname& f) {
	for (size_t i = 0; i < f.identifiers.size(); i++)
		os << " " << f.identifiers[i];
	if (f.method_name)
		os << " " << *f.method_name;
	return os;
}

std::ostream& operator<< (std::ostream& os, const function_declaration& f) {
	os << f.funcname_;
	os << " " << f.funcbody_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const return_& r) {
	os << "return";
	for (size_t i = 0; i < r.expressions.size(); i++)
		os << " " << r.expressions[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const break_& b) {
	os << "break";
	return os;
}

std::ostream& operator<< (std::ostream& os, const std::vector<var>& v) {
	for (size_t i = 0; i < v.size(); i++)
		os << " " << v[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const std::vector<expression>& v) {
	for (size_t i = 0; i < v.size(); i++)
		os << " " << v[i];
	return os;
}

}

