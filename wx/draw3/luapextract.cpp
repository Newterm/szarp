
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

#include <boost/variant.hpp>

#include "szbase/lua_syntax.h"
#include "luapextract.h"

using namespace lua_grammar;

class field_extractor : public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	field_extractor(std::vector<std::wstring>& strings) : strings_(strings) {}

	void operator()(const boost::tuple<expression, expression>& e) const;

	void operator()(const boost::tuple<identifier, expression>& e) const;

	void operator()(const expression& e) const;
};

class term_extractor : public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	term_extractor(std::vector<std::wstring>& strings) : strings_(strings) {}

	template <typename T> void operator()(const T& op) const;

	void operator()(const std::wstring& string) const;

	void operator()(const funcbody& funcbody_) const;

	void operator()(const tableconstructor& tableconstructor_) const;

	void operator()(const postfixexp& postfixexp_) const;

	void operator()(const expression& expression_) const;
};

class args_extractor : public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	args_extractor(std::vector<std::wstring> &strings) : strings_(strings) {}

	void operator()(const std::vector<expression>& exps_) const;

	void operator()(const tableconstructor& tc) const;

	void operator()(const std::wstring& str) const;

};

class name_arg_extractor: public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	name_arg_extractor(std::vector<std::wstring> &strings) : strings_(strings) {}

	void operator()(const args& args_) const;

	void operator()(const boost::tuple<identifier, args>& namearg) const;

};

class exp_identifier_extractor : public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	exp_identifier_extractor(std::vector<std::wstring> &strings) : strings_(strings) {}

	void operator()(const expression& e) const;

	void operator()(const identifier& i) const {}
};

class exp_identifier_name_arg_extractor : public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	exp_identifier_name_arg_extractor(std::vector<std::wstring> &strings) : strings_(strings) {}

	void operator()(const exp_identifier& exp_identifier_) const;

	void operator()(const namearg& namearg_) const;
};

class postfix_extractor : public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	postfix_extractor(std::vector<std::wstring> &strings) : strings_(strings) {}

	void operator()(const identifier& identifier_) const {}

	void operator()(const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &e) const;
};

class var_extractor : public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	var_extractor(std::vector<std::wstring> &strings) : strings_(strings) {}

	void operator()(const identifier& identifier_) const {}

	void operator()(const varc& varc) const;

};

class statement_extractor : public boost::static_visitor<> {
	mutable std::vector<std::wstring>& strings_;
public:
	statement_extractor(std::vector<std::wstring> &strings) : strings_(strings) {}

	void operator() (const assignment &a) const;

	void operator() (const block &b) const;

	void operator() (const while_loop &w) const;

	void operator() (const repeat_loop &r) const;

	void operator() (const if_stat &if_) const;

	void operator() (const for_in_loop &a) const;

	void operator() (const for_from_to_loop &for_) const;

	void operator() (const postfixexp &a) const;

	void operator() (const function_declaration &a) const;
};

class block_extractor {
	mutable std::vector<std::wstring>& strings_;
public:
	block_extractor(std::vector<std::wstring>& strings) : strings_(strings) {}

	void operator()(const block& block_) const;
};

class chunk_extractor {
	mutable std::vector<std::wstring>& strings_;
public:
	chunk_extractor(std::vector<std::wstring>& strings) : strings_(strings) {}
	void operator()(const chunk& chunk_) const;
};

class expression_extractor {
	mutable std::vector<std::wstring>& strings_;
	void extract_unop(const unop_exp& e) const;
	void extract_mul(const mul_exp &e) const;
	void extract_concat(const concat_exp &e) const;
public:
	expression_extractor(std::vector<std::wstring>& strings) : strings_(strings) {}
	void operator() (const expression& e) const;
};

bool extract_strings_from_formula(const std::wstring& formula, std::vector<std::wstring> &strings) {
	chunk c;
	std::wstring::const_iterator s = formula.begin();
	std::wstring::const_iterator e = formula.end();
	if (parse(s, e, c)) {
		(chunk_extractor(strings))(c);
		return true;
	} else 
		return false; 
}

void field_extractor::operator()(const boost::tuple<expression, expression>& e) const {
	(expression_extractor(strings_))(e.get<0>());
	(expression_extractor(strings_))(e.get<1>());
}

void field_extractor::operator()(const boost::tuple<identifier, expression>& e) const {
	(expression_extractor(strings_))(e.get<1>());
}

void field_extractor::operator()(const expression& e) const {
	(expression_extractor(strings_))(e);
}

template <typename T> void term_extractor::operator()(const T& op) const {}

void term_extractor::operator()(const std::wstring& string) const {
	strings_.push_back(string);
}

void term_extractor::operator()(const funcbody& funcbody_) const {
	(block_extractor(strings_))(funcbody_.block_);
}

void term_extractor::operator()(const tableconstructor& t) const {
	for (size_t i = 0; i < t.tableconstructor_.size(); i++) 
		boost::apply_visitor(field_extractor(strings_), t.tableconstructor_[i]);
}

void term_extractor::operator()(const postfixexp& postfixexp_) const {
	boost::apply_visitor(postfix_extractor(strings_), postfixexp_);
}

void term_extractor::operator()(const expression& expression_) const {
	(expression_extractor(strings_))(expression_);
}

void args_extractor::operator()(const std::vector<expression>& exps_) const {
	for (size_t i = 0; i < exps_.size(); i++)
		(expression_extractor(strings_))(exps_[i]);
}

void args_extractor::operator()(const tableconstructor& t) const {
	for (size_t i = 0; i < t.tableconstructor_.size(); i++) 
		boost::apply_visitor(field_extractor(strings_), t.tableconstructor_[i]);
}

void args_extractor::operator()(const std::wstring& str) const {
	strings_.push_back(str);
}

void name_arg_extractor::operator()(const args& args_) const {
	boost::apply_visitor(args_extractor(strings_), args_);	
}

void name_arg_extractor::operator()(const boost::tuple<identifier, args>& namearg) const {
	boost::apply_visitor(args_extractor(strings_), namearg.get<1>());	
}

void exp_identifier_extractor::operator()(const expression& e) const {
	(expression_extractor(strings_))(e);
}

void exp_identifier_name_arg_extractor::operator()(const exp_identifier& exp_identifier_) const {
	boost::apply_visitor(exp_identifier_extractor(strings_), exp_identifier_);
}

void exp_identifier_name_arg_extractor::operator()(const namearg& namearg_) const {
	boost::apply_visitor(name_arg_extractor(strings_), namearg_);
}

void postfix_extractor::operator()(const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &e) const {
	boost::apply_visitor(exp_identifier_extractor(strings_), e.get<0>());
	for (std::vector<exp_ident_arg_namearg>::const_iterator i = e.get<1>().begin(); i != e.get<1>().end(); i++)
		boost::apply_visitor(exp_identifier_name_arg_extractor(strings_), *i);
}

void var_extractor::operator()(const varc& varc_) const {
	boost::apply_visitor(exp_identifier_extractor(strings_), varc_.exp_identifier_);
	for (size_t i = 0; i < varc_.var_seqs.size(); i++) {
		boost::apply_visitor(exp_identifier_extractor(strings_), varc_.var_seqs[i].exp_identifier_);
		const std::vector<namearg>& nameargs = varc_.var_seqs[i].nameargs;
		for (size_t j = 0; j < nameargs.size(); j++)
			boost::apply_visitor(name_arg_extractor(strings_), nameargs[j]);
	}
}

void statement_extractor::operator() (const assignment &a) const {
	for (size_t i = 0; i < a.varlist.size(); i++)
		boost::apply_visitor(var_extractor(strings_), a.varlist[i]);

	for (size_t i = 0; i < a.explist.size(); i++)
		(expression_extractor(strings_))(a.explist[i]);
}

void statement_extractor::operator() (const block &b) const {
	(block_extractor(strings_))(b);
}

void statement_extractor::operator() (const while_loop &w) const {
	(expression_extractor(strings_))(w.expression_);
	(block_extractor(strings_))(w.block_);
}

void statement_extractor::operator() (const repeat_loop &r) const {
	(expression_extractor(strings_))(r.expression_);
	(block_extractor(strings_))(r.block_);
}

void statement_extractor::operator() (const if_stat &if_) const {
	(expression_extractor(strings_))(if_.if_exp);
	(block_extractor(strings_))(if_.block_);
	for (size_t i = 0; i < if_.elseif_.size(); i++) {
		(expression_extractor(strings_))(if_.elseif_[i].get<0>());
		(block_extractor(strings_))(if_.elseif_[i].get<1>());
	}
	if (if_.else_)
		(block_extractor(strings_))(*if_.else_);
}

void statement_extractor::operator() (const for_in_loop &a) const {
	for (size_t i = 0; i < a.expressions.size(); i++) 
		(expression_extractor(strings_))(a.expressions[i]);
	(block_extractor(strings_))(a.block_);
}

void statement_extractor::operator() (const for_from_to_loop &for_) const {
	(expression_extractor(strings_))(for_.from);
	(expression_extractor(strings_))(for_.to);
	if (for_.step)
		(expression_extractor(strings_))(*for_.step);
	(block_extractor(strings_))(for_.block_);
}

void statement_extractor::operator() (const postfixexp &a) const {
	boost::apply_visitor(postfix_extractor(strings_), a);
}

void statement_extractor::operator() (const function_declaration &f) const {
	(block_extractor(strings_))(f.funcbody_.block_);
}


void block_extractor::operator()(const block& block_) const {
	(chunk_extractor(strings_))(block_.chunk_.get());
}

void chunk_extractor::operator()(const chunk& chunk_) const {
	for (size_t i = 0; i < chunk_.stats.size(); i++)
		boost::apply_visitor(statement_extractor(strings_), chunk_.stats[i]);

	if (chunk_.laststat_)
		try {
			const return_& ret = boost::get<return_>(*chunk_.laststat_);
			for (size_t i = 0; i < ret.expressions.size(); i++)
				(expression_extractor(strings_))(ret.expressions[i]);
		} catch (boost::bad_get&) { }
}

void expression_extractor::extract_unop(const unop_exp& e) const {
	const pow_exp& pe = e.get<1>();
	for (pow_exp::const_iterator i = pe.begin(); i != pe.end(); i++)
		boost::apply_visitor(term_extractor(strings_), *i);				
}

void expression_extractor::extract_mul(const mul_exp &e) const {
	extract_unop(e.unop);
	for (mul_list::const_iterator i = e.muls.begin(); i != e.muls.end(); i++)
		extract_unop(i->get<1>());
}

void expression_extractor::extract_concat(const concat_exp &e) const {
	for (concat_exp::const_iterator i = e.begin(); i != e.end(); i++) {
		extract_mul(i->mul);
		for (add_list::const_iterator j = i->adds.begin(); j != i->adds.end(); j++)
			extract_mul(j->get<1>());
	}
}

void expression_extractor::operator() (const expression& e) const {
	for (or_exp::const_iterator i = e.o.begin(); i != e.o.end(); i++)
		for (and_exp::const_iterator j = i->begin(); j != i->end(); j++) {
			extract_concat(j->concat);
			for (cmp_list::const_iterator k = j->cmps.begin(); k != j->cmps.end(); k++)
				extract_concat(k->get<1>());
		}
}

#endif /* ifdef LUA_PARAM_OPTIMISE */

