#define BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_DEBUG_PRINT_SOME 100
#include "lua_syntax.h"

#include <iostream>
#include <string>

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::namelist,
	(std::vector<std::string>, namelist_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::tableconstructor,
	(std::vector<lua_grammar::field>, tableconstructor_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::parlist1,
	(lua_grammar::namelist, namelist_)
	(boost::optional<lua_grammar::threedots>, threedots_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::nil,
	(std::string, nil_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::threedots,
	(std::string, threedots_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::block,
	(boost::recursive_wrapper<lua_grammar::chunk>, chunk_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::var_seq,
	(std::vector<lua_grammar::namearg>, nameargs)
	(lua_grammar::exp_identifier, exp_identifier_) 
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::varc,
	(lua_grammar::exp_identifier, exp_identifier_)
	(std::vector<lua_grammar::var_seq>, var_seqs)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::funcbody,
	(lua_grammar::parlist, parlist_)
	(lua_grammar::block, block_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::mul_exp,
	(lua_grammar::unop_exp, unop)
	(lua_grammar::mul_list, muls)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::add_exp,
	(lua_grammar::mul_exp, mul)
	(lua_grammar::add_list, adds)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::cmp_exp,
	(lua_grammar::concat_exp, concat)
	(lua_grammar::cmp_list, cmps)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::expression,
	(lua_grammar::or_exp, o)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::assignment,
	(std::vector<lua_grammar::var>, varlist)
	(std::vector<lua_grammar::expression>, explist)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::while_loop,
	(lua_grammar::expression, expression_)
	(lua_grammar::block, block_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::repeat_loop,
	(lua_grammar::block, block_)
	(lua_grammar::expression, expression_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::if_stat,
	(lua_grammar::expression, if_exp)
	(lua_grammar::block, block_)
	(lua_grammar::elseif, elseif_)
	(boost::optional<lua_grammar::block>, else_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::for_from_to_loop,
	(lua_grammar::identifier, identifier_)
	(lua_grammar::expression, from)
	(lua_grammar::expression, to)
	(boost::optional<lua_grammar::expression>, step)
	(lua_grammar::block, block_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::for_in_loop,
	(lua_grammar::namelist, namelist_)
	(std::vector<lua_grammar::expression>, expressions)
	(lua_grammar::block, block_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::funcname,
	(std::vector<lua_grammar::identifier>, identifiers)
	(boost::optional<lua_grammar::identifier>, method_name)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::function_declaration,
	(lua_grammar::funcname, funcname_)
	(lua_grammar::funcbody, funcbody_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::local_assignment,
	(std::vector<lua_grammar::var>, varlist)
	(std::vector<lua_grammar::expression>, explist)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::local_function_declaration,
	(lua_grammar::funcname, funcname_)
	(lua_grammar::funcbody, funcbody_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::return_,
	(std::vector<lua_grammar::expression>, expressions)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::chunk,
	(std::vector<lua_grammar::stat>, stats)
	(boost::optional<lua_grammar::laststat>, laststat_)
)

namespace lua_grammar {

expression& expression::operator=(const or_exp& o_) {
	o = o_;
}

namelist& namelist::operator=(const namelist& v) {
	namelist_ = v.namelist_;
	return *this;
}

namelist& namelist::operator=(const std::vector<std::string>& v) {
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

std::ostream& operator<< (std::ostream& os, const expression& e);

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
}

std::ostream& operator<< (std::ostream& out, const block& c ) {
	return out << c.chunk_.get();
}

std::ostream& operator<< (std::ostream& out, const parlist1& c ) {
	out << c.namelist_;
	if (c.threedots_)
		out << " " << *c.threedots_;
	out << " ";
	return out;
}

std::ostream& operator<< (std::ostream& os, const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &e) {
	os << e.get<0>();
	for (size_t i = 0; i < e.get<1>().size(); i++)
		os << " " << e.get<1>()[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const pow_exp& e) {
	if (e.size() > 1)
		os << "pow exp:";
	for (size_t i = 0; i < e.size(); i++)
		os << " " << e[i];
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
	os << e[0];
	for (size_t i = 1; i < e.size(); i++)
		os << " and " << e[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const or_exp& e) {
	os << e[0];
	for (size_t i = 1; i < e.size(); i++)
		os << " or " << e[i];
	return os;
}

std::ostream& operator<< (std::ostream& os, const expression& e) {
	return os << e.o;
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

template<typename Iterator> struct lua_skip_parser : qi::grammar<Iterator> {

	qi::rule<Iterator> skip;

	lua_skip_parser() : lua_skip_parser::base_type(skip) {

		using boost::spirit::ascii::char_;

		skip = qi::space
			| "--[[" >> *(char_ - "]]") >> "]]"
			| "--" >> *(char_ - qi::eol) >> qi::eol
			;
	}

};


template<typename Iterator> struct lua_parser : qi::grammar<Iterator, chunk(), lua_skip_parser<Iterator> > {

	typedef lua_skip_parser<Iterator> space;

	struct mulop_symbols : qi::symbols<char, mul_op> {
		mulop_symbols () {
			add("*", MUL)
				("/", DIV)
				("%", REM);
		}
	} mulop_symbols_;

	struct addop_symbols : qi::symbols<char, add_op> {
		addop_symbols () {
			add("+", PLUS)
				("-", MINUS);
		}
	} addop_symbols_;

	struct cmpop_symbols : qi::symbols<char, cmp_op> {
		cmpop_symbols () {
			add("<", LT)
				("<=", LTE)
				(">", GT)
				(">=", GTE)
				("==", EQ)
				("~=", NEQ);
		}
	} cmpop_symbols_;

	struct unop_symbols : qi::symbols<char, un_op> {
		unop_symbols () {
			add("-", NEG)
				("not", NOT)
				("#", LEN);
		}
	} unop_symbols_;

	struct escaped_symbol : qi::symbols<char, char> {
		escaped_symbol() {
			add("a", '\a')
				("b", '\b')
				("f", '\f')
				("n", '\n')
				("t", '\t')
				("r", '\r')
				("v", '\v')
				("\\", '\\')
				("\'", '\'')
				("\"", '\"');
		}
	} escaped_symbol_;

	qi::rule<Iterator, std::string(), space> keywords;
	qi::rule<Iterator, std::string(), space> identifier_;
	qi::rule<Iterator, std::string(), space> string;
	qi::rule<Iterator, bool(), space> boolean;
	qi::rule<Iterator, nil(), space> nil_;
	qi::rule<Iterator, char(), space> octal_char;
	qi::rule<Iterator, char(), space> escaped_character;
	qi::rule<Iterator, threedots(), space> threedots_;
	qi::rule<Iterator, namelist(), space> namelist_;
	qi::rule<Iterator, parlist1(), space> parlist1_;
	qi::rule<Iterator, parlist(), space> parlist_;
	qi::rule<Iterator, block(), space> block_;
	qi::rule<Iterator, boost::tuple<identifier, expression>(), space> tuple_identifier_expression_;
	qi::rule<Iterator, boost::tuple<expression, expression>(), space> tuple_expression_expression_;
	qi::rule<Iterator, field(), space> field_;
	qi::rule<Iterator, args(), space> args_;
	qi::rule<Iterator, exp_identifier(), space> exp_identifier_;
	qi::rule<Iterator, exp_identifier(), space> exp_identifier_square;
	qi::rule<Iterator, boost::tuple<identifier, args>(), space> identifier_args_;
	qi::rule<Iterator, namearg(), space> namearg_;
	qi::rule<Iterator, var_seq(), space> var_seq_;
	qi::rule<Iterator, varc(), space> varc_;
	qi::rule<Iterator, var(), space> var_;
	qi::rule<Iterator, boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> >(), space> exp_ident_arg_namearg_;
	qi::rule<Iterator, postfixexp(), space> postfixexp_;
	qi::rule<Iterator, funcbody(), space> funcbody_;
	qi::rule<Iterator, term(), space> term_;
	qi::rule<Iterator, expression(), space> expression_;
	qi::rule<Iterator, or_exp(), space> or_;
	qi::rule<Iterator, and_exp(), space> and_;
	qi::rule<Iterator, cmp_exp(), space> cmp_;
	qi::rule<Iterator, concat_exp(), space> concat_;
	qi::rule<Iterator, add_exp(), space> add_;
	qi::rule<Iterator, mul_exp(), space> mul_;
	qi::rule<Iterator, unop_exp(), space> unop_;
	qi::rule<Iterator, pow_exp(), space> pow_;
	qi::rule<Iterator, assignment(), space> assignment_;
	qi::rule<Iterator, block(), space> do_block;
	qi::rule<Iterator, while_loop(), space> while_loop_;
	qi::rule<Iterator, repeat_loop(), space> repeat_loop_;
	qi::rule<Iterator, if_stat(), space> if_stat_;
	qi::rule<Iterator, for_from_to_loop(), space> for_from_to_loop_;
	qi::rule<Iterator, for_in_loop(), space> for_in_loop_;
	qi::rule<Iterator, funcname(), space> funcname_;
	qi::rule<Iterator, funcbody(), space> function_;
	qi::rule<Iterator, function_declaration(), space> function_declaration_;
	qi::rule<Iterator, local_assignment(), space> local_assignment_;
	qi::rule<Iterator, local_function_declaration(), space> local_function_declaration_;
	qi::rule<Iterator, break_(), space> break__;
	qi::rule<Iterator, return_(), space> return__;
	qi::rule<Iterator, stat(), space> stat_;
	qi::rule<Iterator, laststat(), space> laststat_;
	qi::rule<Iterator, chunk(), space> chunk_;
	qi::rule<Iterator, tableconstructor(), space> tableconstructor_;
	qi::rule<Iterator, std::vector<var>(), space> varlist_;
	qi::rule<Iterator, std::vector<field>(), space> fieldlist_;
	qi::rule<Iterator, std::vector<expression>(), space> explist_;
	qi::rule<Iterator, space> fieldsep_;

public:
	lua_parser() : lua_parser::base_type(chunk_) {
		using qi::lit;
		using qi::lexeme;
		using qi::on_error;
		using qi::fail;
		using qi::_val;
		using qi::_1;
		using qi::eps;
		using boost::spirit::ascii::char_;

		keywords = lit("and")
			| "break"
			| "do"
			| "else"
			| "elseif"
			| "end"   
			| "false"
			| "for"
			| "function"
			| "if"
			| "in"
			| "local"
			| "nil"
			| "not"
			| "or"
			| "repeat"
			| "return"
			| "then"
			| "true"
			| "until"
			| "while";

		identifier_ = lexeme[ char_("a-zA-Z_") >> *(char_("a-zA-Z0-9_")) ] - keywords;

		octal_char = eps [_val = 0] >> 
			char_("0-7") [ _val = (_1 - '0') * 64 ] >>
			char_("0-7") [ _val += (_1 - '0') * 8] >>
			char_("0-7") [ _val += (_1 - '0')];

		escaped_character = ( lit('\\') >> (escaped_symbol_ | octal_char) ) | char_;

		threedots_ = lit("...") [_val = threedots() ];

		string = ("'" >> *(escaped_character - "'") >> "'")
			| ("\"" >> *(escaped_character - "\"") >> "\"")
			| ("[[" >> *(char_ - "]]") >> "]]"); 

		break__ = lit("break") [_val = break_()];

		boolean = lit("true")[_val = true]
			| lit("false")[_val = false];

		nil_ = lit("nil")[_val = nil()];

		return__ = ("return" >> explist_) [_val = _1];

		chunk_ = *(stat_ >> -lit(";")) >> -(laststat_ >> -lit(";"));

		block_ = chunk_[_val =  _1];

		explist_ = expression_ % ',' | eps;

		varlist_ = var_ % ',' | eps;

		fieldlist_ = field_  % fieldsep_ | eps;

		tableconstructor_ = ("{" >> fieldlist_ >> '}') [ _val = _1];

		args_ = lit('(') >> explist_ >> lit(')')
			| tableconstructor_
			| string;

		assignment_ = varlist_ >> "=" >> explist_;

		while_loop_ = "while" >> expression_ >> "do" >> block_ >> "end";

		do_block = "do" >> block_ >> "end";

		repeat_loop_ = "repeat" >> block_ >> "until" >> expression_;

		if_stat_ = "if" >> expression_ >> "then" >> block_ >> *("elseif" >> expression_ >> "then" >> block_) >> -("else" >> block_) >> "end";

		for_from_to_loop_ = "for" >> identifier_ >> "=" >> expression_ >> "," >> expression_ >> -("," >> expression_) >> "do" >> block_ >> "end";

		for_in_loop_ = "for" >> namelist_ >> "in" >> explist_ >> "do" >> block_ >> "end";

		funcname_ = "function" >> (identifier_ % ".") >> -(":" >> identifier_);

		function_declaration_ = "function" >> funcname_ >> funcbody_;

		local_assignment_ = "local" >> varlist_ >> "=" >> explist_;

		local_function_declaration_ = lit("local") >> "function" >> funcname_ >> funcbody_;

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

		laststat_ = return__ | break__;

		namelist_ = (identifier_ % ',' )[_val = _1] | eps;
		
		explist_ = expression_ % ',' | eps;
		
		function_ = "function" >> funcbody_;

		term_ = nil_
			| qi::double_
			| boolean
			| string
			| threedots_
			| function_
			| postfixexp_
			| tableconstructor_
			| '(' >> expression_ >> ')';

		expression_ = or_ [_val = _1];
		or_ = and_ % "or";
		and_ =  cmp_ % "and";
		cmp_ = concat_ >> *(cmpop_symbols_ >> concat_);
		concat_ = add_ % "..";
		add_ = mul_ >> *(addop_symbols_ >> mul_);
		mul_ = unop_ >> *(mulop_symbols_ >> unop_);
		unop_ = *unop_symbols_ >> pow_;
		pow_ = term_ % "^";

		exp_identifier_ = '(' >> expression_ >> ')' | identifier_;
		exp_identifier_square = "[" >> expression_ >> "]" | "." >> identifier_;

		identifier_args_ = ":" >> identifier_ >> args_;

		namearg_ = args_ | identifier_args_; 

		exp_ident_arg_namearg_ = exp_identifier_ >> +(exp_identifier_square | namearg_);

		postfixexp_ = exp_ident_arg_namearg_ | identifier_;

		var_seq_ = *namearg_ >> exp_identifier_square;

		varc_ = exp_identifier_ >> +var_seq_;

		var_ = varc_ | identifier_;

		funcbody_ = '(' >> -parlist_ >> ')' >> block_ >> "end";
		
		parlist1_ = namelist_ >> -("," >> threedots_);

		parlist_ = parlist1_ | threedots_;

		tuple_expression_expression_ = '[' >> expression_ >> ']' >> '=' >> expression_;

		tuple_identifier_expression_ = identifier_ >> '=' >> expression_;

		field_ = tuple_expression_expression_
			| tuple_identifier_expression_
			| expression_;

		fieldsep_ = lit(",") | lit(";");

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
		//BOOST_SPIRIT_DEBUG_NODE(qi::double_);
	}

};

bool parse(std::string::const_iterator& iter, std::string::const_iterator &end, chunk& chunk_) {
	typedef lua_parser<std::string::const_iterator> parser;
	typedef lua_skip_parser<std::string::const_iterator> skip;
	return qi::phrase_parse(iter, end, parser(), skip(), chunk_);
}

block& block::operator=(const boost::recursive_wrapper<chunk>& c) { chunk_ = c.get(); return *this; }

}


