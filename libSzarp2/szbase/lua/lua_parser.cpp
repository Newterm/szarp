#define BOOST_SPIRIT_DEBUG

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
	lua_grammar::functioncall_seq,
	(std::vector<lua_grammar::exp_identifier>, exp_identifiers)
	(lua_grammar::namearg, namearg_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::functioncall,
	(lua_grammar::exp_identifier, exp_identifier_)
	(std::vector<lua_grammar::functioncall_seq>, functioncall_seqs)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::funcbody,
	(lua_grammar::parlist, parlist_)
	(lua_grammar::block, block_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::expression,
	(lua_grammar::exp_left, exp_left_)
	(boost::optional<lua_grammar::exp_rest>, exp_rest_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::unop_expression,
	(lua_grammar::unop, op)
	(lua_grammar::expression, expression_)
)

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::binop_expression,
	(lua_grammar::binop, op)
	(lua_grammar::expression, expression_)
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


std::ostream& operator<< (std::ostream& os, const expression& e);

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
	return os << t.get<0>() << " " << t.get<1>();
}

std::ostream& operator<< (std::ostream& os, const unop_expression & e) {
	os << "unop operator(unprinted) ";
	return os << e.expression_;
}

std::ostream& operator<< (std::ostream& os, const funcbody& e) {
	os << e.parlist_;
	os << e.block_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const binop_expression & e) {
	os << "binop operator(unprinted) ";
	os << e.expression_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const expression& e) {
	os << e.exp_left_;
	if (e.exp_rest_)
		os << " " << (*(e.exp_rest_)).get();
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

std::ostream& operator<< (std::ostream& os, const functioncall_seq& f) {
	for (size_t i = 0; i < f.exp_identifiers.size(); i++)
		os << " " << f.exp_identifiers[i];
	os << " " << f.namearg_;
	return os;
}

std::ostream& operator<< (std::ostream& os, const functioncall& f) {
	os << f.exp_identifier_;
	for (size_t i = 0; i < f.functioncall_seqs.size(); i++)
		os << " " << f.functioncall_seqs[i];
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

	struct binop_symbols : qi::symbols<char, binop> {
		binop_symbols () {
			add("+", PLUS)
				("-", MINUS)
				("<", LT)
				("<=", LTE)
				(">", GT)
				(">=", GTE)
				("==", EQ)
				("~=", NEQ)
				("..", CONCAT)
				("*", MUL)
				("/", DIV)
				("%", REM)
				("^", POW);
		}
	} binop_symbols_;

	struct unop_symbols : qi::symbols<char, unop> {
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
	qi::rule<Iterator, std::string(), space> identifier_, comment;
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
	qi::rule<Iterator, functioncall_seq(), space> functioncall_seq_;
	qi::rule<Iterator, functioncall(), space> functioncall_;
	qi::rule<Iterator, funcbody(), space> funcbody_;
	qi::rule<Iterator, exp_left(), space> exp_left_;
	qi::rule<Iterator, boost::optional<exp_rest>(), space> exp_rest_;
	qi::rule<Iterator, expression(), space> expression_;
	qi::rule<Iterator, unop_expression(), space> unop_exp;
	qi::rule<Iterator, binop_expression(), space> binop_exp;
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
			| functioncall_
			| var_;

		laststat_ = return__ | break__;

		namelist_ = (identifier_ % ',' )[_val = _1] | eps;
		
		explist_ = expression_ % ',' | eps;
		
		unop_exp = unop_symbols_ >> expression_;

		function_ = "function" >> funcbody_;

		exp_left_ = nil_
			| qi::double_
			| boolean
			| string
			| threedots_
			| function_
			| functioncall_
			| var_
			| tableconstructor_
			| unop_exp;

		binop_exp = binop_symbols_ >> expression_;

		exp_rest_ = binop_exp | eps;

		expression_ = exp_left_ >> exp_rest_;

		exp_identifier_ = '(' >> expression_ >> ')' | identifier_;

		exp_identifier_square = "[" >> expression_ >> "]" | "." >> identifier_;

		identifier_args_ = ":" >> identifier_ >> args_;

		namearg_ = args_ | identifier_args_; 

		var_seq_ = *namearg_ >> exp_identifier_square;

		varc_ = exp_identifier_ >> +var_seq_;

		var_ = varc_ | identifier_;

		functioncall_seq_ = *exp_identifier_square >> namearg_;

		functioncall_ = exp_identifier_ >> +functioncall_seq_;

		funcbody_ = '(' >> -parlist_ >> ')' >> block_ >> "end";
		
		parlist1_ = namelist_ >> -("," >> threedots_);

		parlist_ = parlist1_ | threedots_;

		tuple_expression_expression_ = '[' >> expression_ >> ']' >> '=' >> expression_;

		tuple_identifier_expression_ = identifier_ >> '=' >> expression_;

		field_ = tuple_expression_expression_
			| tuple_identifier_expression_
			| expression_;

		fieldsep_ = lit(",") | lit(";");

		identifier_.name("identifier_");
		comment.name("comment");
		string.name("string");
		boolean.name("boolean");
		nil_.name("nil_");
		octal_char.name("octal_char");
		escaped_character.name("escaped_character");
		threedots_.name("threedots_");
		namelist_.name("namelist_");
		parlist1_.name("parlist1_");
		parlist_.name("parlist_");
		block_.name("block_");
		tuple_identifier_expression_.name("tuple_identifier_expression_");
		tuple_expression_expression_.name("tuple_expression_expression_");
		field_.name("field_");
		args_.name("args_");
		exp_identifier_.name("exp_identifier_");
		exp_identifier_square.name("exp_identifier_square");
		identifier_args_.name("identifier_args_");
		namearg_.name("namearg_");
		var_seq_.name("var_seq_");
		var_.name("var_");
		varc_.name("varc_");
		functioncall_seq_.name("functioncall_seq_");
		functioncall_.name("functioncall_");
		funcbody_.name("funcbody_");
		exp_left_.name("exp_left_");
		exp_rest_.name("exp_rest_");
		expression_.name("expression_");
		unop_exp.name("unop_exp");
		binop_exp.name("binop_exp");
		assignment_.name("assignment_");
		do_block.name("do_block");
		while_loop_.name("while_loop_");
		repeat_loop_.name("repeat_loop_");
		if_stat_.name("if_stat_");
		for_from_to_loop_.name("for_from_to_loop_");
		for_in_loop_.name("for_in_loop_");
		funcname_.name("funcname_");
		function_.name("function_");
		function_declaration_.name("function_declaration_");
		local_assignment_.name("local_assignment_");
		local_function_declaration_.name("local_function_declaration_");
		break__.name("break__");
		return__.name("return__");
		stat_.name("stat_");
		laststat_.name("laststat_");
		chunk_.name("chunk_");
		tableconstructor_.name("tableconstructor_");
		varlist_.name("varlist_");
		varlist_.name("fieldlist_");
		explist_.name("explist_");
		fieldsep_.name("fieldsep_");

		qi::debug(identifier_);
		qi::debug(comment);
		qi::debug(string);
		qi::debug(boolean);
		qi::debug(nil_);
		qi::debug(octal_char);
		qi::debug(escaped_character);
		qi::debug(threedots_);
		qi::debug(namelist_);
		qi::debug(parlist1_);
		qi::debug(parlist_);
		qi::debug(block_);
		qi::debug(tuple_identifier_expression_);
		qi::debug(tuple_expression_expression_);
		qi::debug(field_);
		qi::debug(args_);
		qi::debug(exp_identifier_);
		qi::debug(identifier_args_);
		qi::debug(namearg_);
		qi::debug(var_seq_);
		qi::debug(varc_);
		qi::debug(var_);
		qi::debug(functioncall_seq_);
		qi::debug(functioncall_);
		qi::debug(funcbody_);
		qi::debug(exp_left_);
		qi::debug(exp_rest_);
		qi::debug(expression_);
		qi::debug(unop_exp);
		qi::debug(binop_exp);
		qi::debug(assignment_);
		qi::debug(do_block);
		qi::debug(while_loop_);
		qi::debug(repeat_loop_);
		qi::debug(if_stat_);
		qi::debug(for_from_to_loop_);
		qi::debug(for_in_loop_);
		qi::debug(funcname_);
		qi::debug(function_);
		qi::debug(function_declaration_);
		qi::debug(local_assignment_);
		qi::debug(local_function_declaration_);
		qi::debug(break__);
		qi::debug(return__);
		qi::debug(stat_);
		qi::debug(laststat_);
		qi::debug(chunk_);
		qi::debug(tableconstructor_);
		qi::debug(varlist_);
		qi::debug(fieldlist_);
		qi::debug(explist_);
		qi::debug(fieldsep_);

	}

};

bool parse(std::string::const_iterator& iter, std::string::const_iterator &end, chunk& chunk_) {
	typedef lua_parser<std::string::const_iterator> parser;
	typedef lua_skip_parser<std::string::const_iterator> skip;
	return qi::phrase_parse(iter, end, parser(), skip(), chunk_);
}

block& block::operator=(const boost::recursive_wrapper<chunk>& c) { chunk_ = c.get(); return *this; }

}


