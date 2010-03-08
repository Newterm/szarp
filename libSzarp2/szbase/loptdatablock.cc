#include "config.h"

#include <cmath>
#include <cfloat>
#include <functional>
#include <boost/variant.hpp>

#include "conversion.h"
#include "liblog.h"
#include "lua_syntax.h"
#include "szbbase.h"
#include "loptdatablock.h"

#ifdef LUA_PARAM_OPTIMISE

#include <boost/make_shared.hpp>

namespace LuaExec {

using namespace lua_grammar;


class ExecutionEngine;

class NilExpression : public Expression {
public:
	virtual Val Value() { return nan(""); }
};

class NumberExpression : public Expression {
	Val m_val;
public:
	NumberExpression(Val val) : m_val(val) {}

	virtual Val Value() {
		return m_val;
	}
};

class VarExpression : public Expression {
	Var* m_var;
public:
	VarExpression(Var *var) : m_var(var) {}

	virtual Val Value() {
		return (*m_var)();
	}
};

template<class unop> class UnExpression : public Expression {
	PExpression m_e;
	unop m_op;
public:
	UnExpression(PExpression e) : m_e(e) {}

	virtual Val Value() {
		return m_op(m_e->Value());
	}
};

template<class op> class BinExpression : public Expression {
	PExpression m_e1, m_e2; 
	op m_op;
public:
	BinExpression(const PExpression& e1, const PExpression& e2) : m_e1(e1), m_e2(e2) {}

	virtual Val Value() {
		return m_op(m_e1->Value(), m_e2->Value());
	}
};

template<> Val BinExpression<std::logical_or<Val> >::Value() {
	return m_e1->Value() || m_e2->Value();
}

template<> Val BinExpression<std::logical_and<Val> >::Value() {
	return m_e1->Value() && m_e2->Value();
}

template<> Val BinExpression<std::modulus<Val> >::Value() {
	return int(m_e1->Value()) % int(m_e2->Value());
}

class AssignmentExpression : public Expression {
	Var* m_var;
	PExpression m_exp;
public:
	AssignmentExpression(Var* var, PExpression exp) : m_var(var), m_exp(exp) {}

	virtual Val Value() {
		*m_var = m_exp->Value();
		return (*m_var)();
	}
};

class IsNanExpression : public Expression {
	PExpression m_exp;
public:
	IsNanExpression(PExpression exp) : m_exp(exp) {}

	virtual Val Value() {
		return std::isnan(m_exp->Value());
	}
};

class ParamValue : public Expression {
	ParamRef* m_param_ref;
	PExpression m_time;
	PExpression m_avg_type;
public:
	ParamValue(ParamRef* param_ref, PExpression time, const PExpression avg_type)
		: m_param_ref(param_ref), m_time(time), m_avg_type(avg_type) {}

	virtual Val Value() {
		return m_param_ref->Value(m_time->Value(), m_avg_type->Value());
	}
};

class SzbMoveTimeExpression : public Expression {
	PExpression m_start_time;
	PExpression m_displacement;
	PExpression m_period_type;
public:
	SzbMoveTimeExpression(PExpression start_time, PExpression displacement, PExpression period_type) : 
		m_start_time(start_time), m_displacement(displacement), m_period_type(period_type) {}

	virtual Val Value() {
		return szb_move_time(m_start_time->Value(), m_displacement->Value(), SZARP_PROBE_TYPE(m_period_type->Value()), 0);
	}

};


class IfExpression : public Expression {
	PExpression m_cond;
	PExpression m_consequent;
	std::vector<std::pair<PExpression, PExpression> > m_elseif;
	PExpression m_alternative;
public:
	IfExpression(const PExpression cond,
			const PExpression conseqeunt,
			const std::vector<std::pair<PExpression, PExpression> >& elseif,
			const PExpression alternative)
		: m_cond(cond), m_consequent(conseqeunt), m_elseif(elseif), m_alternative(alternative) {}

	virtual Val Value() {
		if (m_cond->Value())
			return m_consequent->Value();
		else for (std::vector<std::pair<PExpression, PExpression> >::iterator i = m_elseif.begin();
				i != m_elseif.end();	
				i++) 
			if (i->first->Value())
				return i->second->Value();
		return m_alternative->Value();
	}

};

class ForLoopExpression : public Expression { 
	Var* m_var;
	PExpression m_start;
	PExpression m_limit;
	PExpression m_step;
	PExpression m_exp;
public:
	ForLoopExpression(Var *var, PExpression start, PExpression limit, PExpression step, PExpression exp) :
		m_var(var), m_start(start), m_limit(limit), m_step(step), m_exp(exp) {}

	virtual Val Value() {
		*m_var = m_start->Value();
		double limit = m_limit->Value();
		double step = m_step->Value();
		while ((step > 0 && (*m_var)() <= limit)
				|| (step <= 0 && (*m_var)() >= limit)) {
			m_exp->Value();
			*m_var = (*m_var)() + step;
		}
		return nan("");
	}

};

class WhileExpression : public Expression {
	PExpression m_cond;
	PExpression m_exp;
public:
	WhileExpression(PExpression cond, PExpression exp) : m_cond(cond), m_exp(exp) {}

	virtual Val Value() {
		while (m_cond->Value())
			m_exp->Value();
		return nan("");
	}

};

class RepeatExpression : public Expression {
	PExpression m_cond;
	PExpression m_exp;
public:
	RepeatExpression(PExpression cond, PExpression exp) : m_cond(cond), m_exp(exp) {}
	virtual Val Value() {
		do
			m_exp->Value();
		while (m_cond->Value());
		return nan("");
	}
};


class ParamConversionException {
	std::wstring m_error;
public:
	ParamConversionException(std::wstring error) : m_error(error) {}
	const std::wstring& what() const ;
};

class ParamConverter;

class StatementConverter : public boost::static_visitor<PExpression> {
	ParamConverter* m_param_converter;
public:
	StatementConverter(ParamConverter *param_converter) : m_param_converter(param_converter) {}

	PExpression operator() (const assignment &a);

	PExpression operator() (const block &b);

	PExpression operator() (const while_loop &w);

	PExpression operator() (const repeat_loop &r);

	PExpression operator() (const if_stat &if_);

	PExpression operator() (const for_in_loop &a);

	PExpression operator() (const postfixexp &a);

	PExpression operator() (const for_from_to_loop &for_);

	PExpression operator() (const function_declaration &a);

	PExpression operator() (const local_assignment &a);

	PExpression operator() (const local_function_declaration &f);

};

class ExpressionConverter {
	ParamConverter* m_param_converter;

	PExpression ConvertTerm(const term& term_);

	PExpression ConvertPow(const pow_exp& exp);

	PExpression ConvertUnOp(const unop_exp& unop);

	PExpression ConvertMul(const mul_exp& mul_);

	PExpression ConvertAdd(const add_exp& add_);

	PExpression ConvertConcat(const concat_exp &concat);

	PExpression ConvertCmp(const cmp_exp& cmp_exp_);

	PExpression ConvertAnd(const and_exp& and_exp_);

	PExpression ConvertOr(const or_exp& or_exp_);

public:
	ExpressionConverter(ParamConverter *param_converter) : m_param_converter(param_converter) {}
	PExpression ConvertExpression(const expression& expression_);
};

class TermConverter : public boost::static_visitor<PExpression> {
	ParamConverter* m_param_converter;
public:
	TermConverter(ParamConverter *param_converter) : m_param_converter(param_converter) { }
	PExpression operator()(const nil& nil_);

	PExpression operator()(const bool& bool_);

	PExpression operator()(const double& double_);

	PExpression operator()(const std::wstring& string);

	PExpression operator()(const threedots& threedots_);

	PExpression operator()(const funcbody& funcbody_);

	PExpression operator()(const tableconstructor& tableconstrutor_);

	PExpression operator()(const postfixexp& postfixexp_);

	PExpression operator()(const expression& expression_);
};

class PostfixConverter : public boost::static_visitor<PExpression> {
	ParamConverter* m_param_converter;
public:
	PostfixConverter(ParamConverter* param_converter) : m_param_converter(param_converter) {}

	PExpression operator()(const identifier& identifier_);

	const std::vector<expression>& GetArgs(const std::vector<exp_ident_arg_namearg>& e);

	PExpression operator()(const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &exp);
};

class FunctionConverter { 
protected:
	ParamConverter* m_param_converter;
public:
	FunctionConverter(ParamConverter *param_converter) : m_param_converter(param_converter) {}	

	virtual PExpression Convert(const std::vector<expression>& expressions) = 0;
};

class SzbMoveTimeConverter : public FunctionConverter {
public:
	SzbMoveTimeConverter(ParamConverter *param_converter) : FunctionConverter(param_converter) {}	

	virtual PExpression Convert(const std::vector<expression>& expressions);
};

class IsNanConverter : public FunctionConverter {
public:
	IsNanConverter(ParamConverter *param_converter) : FunctionConverter(param_converter) {}	

	virtual PExpression Convert(const std::vector<expression>& expressions);
};

class NanConverter : public FunctionConverter {
public:
	NanConverter(ParamConverter *param_converter) : FunctionConverter(param_converter) {}	

	PExpression Convert(const std::vector<expression>& expressions);
};

class ParamValueConverter : public FunctionConverter {
	std::wstring GetParamName(const expression& e);
public:
	ParamValueConverter(ParamConverter *param_converter) : FunctionConverter(param_converter) {}	

	PExpression Convert(const std::vector<expression>& expressions);
};

class ParamConverter {
	Szbase* m_szbase;
	Param* m_param;
	typedef std::map<std::wstring, Var*> frame;
	std::vector<frame> m_vars_map;
	std::vector<std::map<std::wstring, Var*> >::iterator m_current_frame;
	std::map<std::wstring, boost::shared_ptr<FunctionConverter> > m_function_converters;
public:
	ParamConverter(Szbase *szbase);

	Var* GetGlobalVar(std::wstring identifier);

	Var* GetLocalVar(const std::wstring& identifier);

	Var* FindVar(const std::wstring& identifier);

	ParamRef* GetParamRef(const std::wstring param_name);

	PExpression ConvertBlock(const block& block);

	PExpression ConvertFunction(const identifier& identifier_, const std::vector<expression>& args);

	PExpression ConvertStatement(const stat& stat_);

	PExpression ConvertChunk(const chunk& chunk_);

	PExpression ConvertTerm(const term& term_);
	
	PExpression ConvertExpression(const expression& expression_);

	PExpression ConvertPostfixExp(const postfixexp& postfixexp_);

	void ConvertParam(const chunk& chunk_, Param* param);

	void AddVariable(std::wstring name);

	void InitalizeVars();

};

class ExecutionEngine {
	time_t m_start_time;
	szb_buffer_t* m_buffer;
	std::vector<szb_datablock_t*> m_blocks;
	std::vector<double> m_vals;
	Param* m_param;
	bool m_fixed;
public:
	ExecutionEngine(LuaOptDatablock *block);

	void CalculateValue(time_t t, double &val, bool &fixed);

	void Refresh();

	double Value(size_t param_index, double time, double period_type);

	std::vector<double>& Vars();

	~ExecutionEngine();
};

Val& Var::operator()() {
	return m_ee->Vars()[m_var_no];
}

Var& Var::operator=(const Val& val) {
	m_ee->Vars()[m_var_no] = val;
	return *this;
}

void ExpressionList::AddExpression(PExpression expression) {
	m_expressions.push_back(expression);
}

Val ExpressionList::Value() {
	size_t l = m_expressions.size();
	if (l == 0)
		return nan("");
	for (size_t i = 0; i < l - 1; i++)
		m_expressions[i]->Value();
	return m_expressions[l - 1]->Value();
}

PExpression StatementConverter::operator() (const assignment &a) {
#if 0
	for (size_t i = 0; i < a.varlist.size(); i++) {
#else
		size_t i = 0;
#endif
		try {
			const identifier& identifier_ = boost::get<identifier>(a.varlist[i]);
			Var* variable = m_param_converter->GetGlobalVar(identifier_);
			PExpression expression;
			if (i < a.explist.size())
				expression = m_param_converter->ConvertExpression(a.explist[i]);
			else
				expression = boost::make_shared<NilExpression>();
			return boost::make_shared<AssignmentExpression>(AssignmentExpression(variable, expression));
		} catch (boost::bad_get&) {
			throw ParamConversionException(L"Only assignment to variables allowed in optimized expressions");
		}
#if 0
	}
#endif
}

PExpression StatementConverter::operator() (const block &b) {
	return m_param_converter->ConvertBlock(b);
}

PExpression StatementConverter::operator() (const while_loop &w) {
	PExpression condition = m_param_converter->ConvertExpression(w.expression_);
	PExpression block = m_param_converter->ConvertBlock(w.block_);
	return boost::make_shared<WhileExpression>(condition, block);
}

PExpression StatementConverter::operator() (const repeat_loop &r) {
	PExpression condition = m_param_converter->ConvertExpression(r.expression_);
	PExpression block = m_param_converter->ConvertBlock(r.block_);
	return boost::make_shared<RepeatExpression>(condition, block);
}

PExpression StatementConverter::operator() (const if_stat &if_) {
	PExpression cond = m_param_converter->ConvertExpression(if_.if_exp);
	PExpression block = m_param_converter->ConvertBlock(if_.block_);
	std::vector<std::pair<PExpression, PExpression> > elseif; 
	for (size_t i = 0; i < if_.elseif_.size(); i++)
		elseif.push_back(std::make_pair(m_param_converter->ConvertExpression(if_.elseif_[i].get<0>()),
				m_param_converter->ConvertBlock(if_.elseif_[i].get<1>())));
	PExpression else_;
	if (if_.else_)
		else_ = m_param_converter->ConvertBlock(*if_.else_);
	else
		else_ = boost::make_shared<NilExpression>();
	return boost::make_shared<IfExpression>(cond, block, elseif, else_);
}

PExpression StatementConverter::operator() (const for_in_loop &a) {
	throw ParamConversionException(L"For in loops not supported by converter");
}

PExpression StatementConverter::operator() (const postfixexp &a) {
	throw ParamConversionException(L"Postfix expressions as statments not supported by converter");
}

PExpression StatementConverter::operator() (const for_from_to_loop &for_) {
	Var* var = m_param_converter->GetLocalVar(for_.identifier_);
	PExpression from = m_param_converter->ConvertExpression(for_.from);
	PExpression to = m_param_converter->ConvertExpression(for_.to);
	PExpression step;
	if (for_.step)
		step = m_param_converter->ConvertExpression(*for_.step);
	else
		step = boost::make_shared<NumberExpression>(1);
	PExpression block = m_param_converter->ConvertBlock(for_.block_);
	return boost::make_shared<ForLoopExpression>(var, from, to, step, block);
}

PExpression StatementConverter::operator() (const function_declaration &a) {
	throw ParamConversionException(L"Function declarations not supported by converter");
}

PExpression StatementConverter::operator() (const local_assignment &a) {
	PExpression expression;
#if 0
	for (size_t i = 0; i < a.varlist.size(); i++) {
#else
	size_t i = 0;
#endif
		try {
			const identifier& identifier_ = boost::get<identifier>(a.varlist[i]);
			Var* variable = m_param_converter->GetLocalVar(identifier_);
			if (i < a.explist.size())
				expression = m_param_converter->ConvertExpression(a.explist[i]);
			else
				expression = boost::make_shared<NilExpression>();
			return boost::make_shared<AssignmentExpression>(AssignmentExpression(variable, expression));
		} catch (boost::bad_get&) {
			throw ParamConversionException(L"Only assignment to variables allowed in optimized expressions");
		}
#if 0
	}
#endif
}

PExpression StatementConverter::operator() (const local_function_declaration &f) {
	throw ParamConversionException(L"Function declarations not supported by converter");
}

PExpression ExpressionConverter::ConvertTerm(const term& term_) {
	return m_param_converter->ConvertTerm(term_);
}

struct pow_functor : public std::binary_function<const Val&, const Val&, Val> {
	Val operator() (const Val& s1, const Val& s2) const {
		return pow(s1, s2);
	}
};

PExpression ExpressionConverter::ConvertPow(const pow_exp& exp) {
	pow_exp::const_reverse_iterator i = exp.rbegin();
	PExpression p = ConvertTerm(*i);
	while (++i != exp.rend()) {
		PExpression p2 = ConvertTerm(*i);
		p = boost::make_shared<BinExpression<pow_functor> >(p2, p);
	}
	return p;
}

PExpression ExpressionConverter::ConvertUnOp(const unop_exp& unop) {
	PExpression p(ConvertPow(unop.get<1>()));
	const std::vector<un_op>& v = unop.get<0>();
	for (std::vector<un_op>::const_reverse_iterator i = v.rbegin();
			i != v.rend();
			i++)
		switch (*i) {
			case NEG:
				p = boost::make_shared<UnExpression<std::negate<Val> > >(p);
				break;
			case NOT:
				p = boost::make_shared<UnExpression<std::logical_not<Val> > >(p);
				break;
			case LEN:
				throw ParamConversionException(L"Opeartor '#' not supported in optimized params");
				break;
		}
	return p;
}

PExpression ExpressionConverter::ConvertMul(const mul_exp& mul_) {
	PExpression p = ConvertUnOp(mul_.unop);
	for (size_t i = 0; i < mul_.muls.size(); i++) {
		PExpression p2 = ConvertUnOp(mul_.muls[i].get<1>());
		switch (mul_.muls[i].get<0>()) {
			case MUL:
				p = boost::make_shared<BinExpression<std::multiplies<Val> > >(p, p2);
				break;
			case DIV:
				p = boost::make_shared<BinExpression<std::divides<Val> > >(p, p2);
				break;
			case REM:
				p = boost::make_shared<BinExpression<std::modulus<Val> > >(p, p2);
				break;
		}
	}
	return p;
}

PExpression ExpressionConverter::ConvertAdd(const add_exp& add_) {
	PExpression p = ConvertMul(add_.mul);
	for (size_t i = 0; i < add_.adds.size(); i++) {
		PExpression p2 = ConvertMul(add_.adds[i].get<1>());
		switch (add_.adds[i].get<0>()) {
			case PLUS:
				p = boost::make_shared<BinExpression<std::plus<Val> > >(p, p2);
				break;
			case MINUS:
				p = boost::make_shared<BinExpression<std::minus<Val> > >(p, p2);
				break;
		}
	}
	return p;
}

PExpression ExpressionConverter::ConvertConcat(const concat_exp &concat) {
	if (concat.size() > 1)
		throw ParamConversionException(L"Concatenation parameter cannot be used is optimized parameters");
	return ConvertAdd(concat[0]);
}

PExpression ExpressionConverter::ConvertCmp(const cmp_exp& cmp_exp_) {
	PExpression p = ConvertConcat(cmp_exp_.concat);
	for (size_t i = 0; i < cmp_exp_.cmps.size(); i++) {
		PExpression p2 = ConvertConcat(cmp_exp_.cmps[i].get<1>());
		switch (cmp_exp_.cmps[i].get<0>()) {
			case LT:
				p = boost::make_shared<BinExpression<std::less<Val> > >(p, p2);
				break;
			case LTE:
				p = boost::make_shared<BinExpression<std::less_equal<Val> > >(p, p2);
				break;
			case EQ:
				p = boost::make_shared<BinExpression<std::equal_to<Val> > >(p, p2);
				break;
			case GTE:
				p = boost::make_shared<BinExpression<std::greater_equal<Val> > >(p, p2);
				break;
			case GT:
				p = boost::make_shared<BinExpression<std::greater<Val> > >(p, p2);
				break;
			case NEQ:
				p = boost::make_shared<BinExpression<std::not_equal_to<Val> > >(p, p2);
				break;
		}
	}
	return p;
}

PExpression ExpressionConverter::ConvertAnd(const and_exp& and_exp_) {
	PExpression p = ConvertCmp(and_exp_[0]);
	for (size_t i = 1; i < and_exp_.size(); i++)
		p = boost::make_shared<BinExpression<std::logical_and<Val> > >(p, ConvertCmp(and_exp_[i]));
	return p;
}

PExpression ExpressionConverter::ConvertOr(const or_exp& or_exp_) {
	PExpression p = ConvertAnd(or_exp_[0]);
	for (size_t i = 1; i < or_exp_.size(); i++)
		p = boost::make_shared<BinExpression<std::logical_or<Val> > >(p, ConvertAnd(or_exp_[i]));
	return p;
}

PExpression ExpressionConverter::ConvertExpression(const expression& expression_) {
	return ConvertOr(expression_.o);
}

PExpression TermConverter::operator()(const nil& nil_) {
	return boost::make_shared<NilExpression>();
}

PExpression TermConverter::operator()(const bool& bool_) {
	return boost::make_shared<NumberExpression>(bool_ ? 1. : 0.);
}

PExpression TermConverter::operator()(const double& double_) {
	return boost::make_shared<NumberExpression>(double_);
}

PExpression TermConverter::operator()(const std::wstring& string) {
	throw ParamConversionException(L"String cannot appear as termns in optimized params");
}

PExpression TermConverter::operator()(const threedots& threedots_) {
	throw ParamConversionException(L"Threedots opeartors cannot appear in optimized params");
}

PExpression TermConverter::operator()(const funcbody& funcbody_) {
	throw ParamConversionException(L"Function bodies cannot appear in optimized expressions");
}

PExpression TermConverter::operator()(const tableconstructor& tableconstrutor_) {
	throw ParamConversionException(L"Table construtor are not supported in optimized expressions");
}

PExpression TermConverter::operator()(const postfixexp& postfixexp_) {
	return m_param_converter->ConvertPostfixExp(postfixexp_);
}

PExpression TermConverter::operator()(const expression& expression_) {
	return m_param_converter->ConvertExpression(expression_);
}

PExpression PostfixConverter::operator()(const identifier& identifier_) {
	return boost::make_shared<VarExpression>(m_param_converter->FindVar(identifier_));
}

const std::vector<expression>& PostfixConverter::GetArgs(const std::vector<exp_ident_arg_namearg>& e) {
	if (e.size() != 1)
		throw ParamConversionException(L"Only postfix expression in form functioname(exp, exp, ...) are allowed");
	try {
		const namearg& namearg_ = boost::get<namearg>(e[0]);
		const args& args_ = boost::get<args>(namearg_);
		const boost::recursive_wrapper<std::vector<expression> > &exps_
			= boost::get<boost::recursive_wrapper<std::vector<expression> > >(args_);
		return exps_.get();
	} catch (boost::bad_get &e) {
		throw ParamConversionException(L"Only postfix expression in form functioname(exp, exp, ...) are allowed");
	}
}

PExpression PostfixConverter::operator()(const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &exp) {
	try {
		const exp_identifier& ei = exp.get<0>();
		const identifier& identifier_ = boost::get<identifier>(ei);
		const std::vector<expression>& args = GetArgs(exp.get<1>());
		return m_param_converter->ConvertFunction(identifier_, args);
	} catch (boost::bad_get &e) {
		throw ParamConversionException(L"Postfix expression (functioncall) must start with identifier in optimized params");
	}
}

PExpression SzbMoveTimeConverter::Convert(const std::vector<expression>& expressions) {
	if (expressions.size() < 3)
		throw ParamConversionException(L"szb_move_time requires three arguments");

	return boost::make_shared<SzbMoveTimeExpression>
		(m_param_converter->ConvertExpression(expressions[0]),
		 m_param_converter->ConvertExpression(expressions[1]),
		 m_param_converter->ConvertExpression(expressions[2]));
}

PExpression IsNanConverter::Convert(const std::vector<expression>& expressions) {
	if (expressions.size() < 1)
		throw ParamConversionException(L"isnan takes one argument");

	return boost::make_shared<IsNanExpression>
		(m_param_converter->ConvertExpression(expressions[0]));
}

PExpression NanConverter::Convert(const std::vector<expression>& expressions) {
	return boost::make_shared<NilExpression>();
}

std::wstring ParamValueConverter::GetParamName(const expression& e) {
#define ERROR throw ParamConversionException(L"First parameter of p function should be literal string")

	const or_exp& or_exp_ = e.o;
	if (or_exp_.size() != 1)
		ERROR;

	const and_exp& and_exp_ = or_exp_[0];
	if (and_exp_.size() != 1)
		ERROR;

	const cmp_exp& cmp_exp_ = and_exp_[0];
	if (cmp_exp_.cmps.size())
		ERROR;

	const concat_exp& concat_exp_ = cmp_exp_.concat;
	if (concat_exp_.size() != 1)
		ERROR;

	const add_exp& add_exp_ = concat_exp_[0];
	if (add_exp_.adds.size())
		ERROR;

	const mul_exp& mul_exp_ = add_exp_.mul;
	if (mul_exp_.muls.size())
		ERROR;

	const unop_exp& unop_exp_ = mul_exp_.unop;
	if (unop_exp_.get<0>().size())
		ERROR;

	const pow_exp& pow_exp_ = unop_exp_.get<1>();
	if (pow_exp_.size() != 1)
		ERROR;

	if (const std::wstring* name = boost::get<std::wstring*>(pow_exp_[0])) 
		return *name;
	else
		ERROR;
}

PExpression ParamValueConverter::Convert(const std::vector<expression>& expressions) {
	if (expressions.size() < 3)
		throw ParamConversionException(L"p function requires three arguemnts");
	std::wstring param_name = GetParamName(expressions[0]);
	ParamRef* param_ref = m_param_converter->GetParamRef(param_name);
	return boost::make_shared<ParamValue>(param_ref,
			m_param_converter->ConvertExpression(expressions[1]),
			m_param_converter->ConvertExpression(expressions[2]));
}

ParamConverter::ParamConverter(Szbase *szbase) : m_szbase(szbase) {
	m_function_converters[L"szb_move_time"] = boost::make_shared<SzbMoveTimeConverter>(this);
	m_function_converters[L"isnan"] = boost::make_shared<IsNanConverter>(this);
	m_function_converters[L"nan"] = boost::make_shared<NanConverter>(this);
	m_function_converters[L"p"] = boost::make_shared<ParamValueConverter>(this);
}

Var* ParamConverter::GetGlobalVar(std::wstring identifier) {
	std::map<std::wstring, Var*>::iterator i = m_vars_map[0].find(identifier);
	if (i == m_vars_map[0].end()) {
		size_t s = m_param->m_vars.size();
		m_param->m_vars.push_back(s);
		m_vars_map[0][identifier] = &m_param->m_vars.back();
		return &m_param->m_vars.back();
	} else {
		return i->second;
	}
}

Var* ParamConverter::GetLocalVar(const std::wstring& identifier) {
	std::map<std::wstring, Var*>::iterator i = m_vars_map.back().find(identifier);
	if (i == m_vars_map.back().end()) {
		size_t s = m_param->m_vars.size();
		m_param->m_vars.push_back(s);
		m_vars_map.back()[identifier] = &m_param->m_vars.back();
		return &m_param->m_vars.back();
	} else {
		return i->second;
	}
}

Var* ParamConverter::FindVar(const std::wstring& identifier) {
	for (std::vector<frame>::reverse_iterator i = m_vars_map.rbegin();
			i != m_vars_map.rend();
			i++) {
		std::map<std::wstring, Var*>::iterator j = i->find(identifier);
		if (j != i->end())
			return j->second;
	}
	throw ParamConversionException(std::wstring(L"Variable ") + identifier + L" is unbound");
}

ParamRef* ParamConverter::GetParamRef(const std::wstring param_name) {
	std::pair<szb_buffer_t*, TParam*> bp;
	if (m_szbase->FindParam(param_name, bp) == false)
		throw ParamConversionException(std::wstring(L"Param ") + param_name + L" not found");

	for (std::vector<ParamRef>::iterator i = m_param->m_par_refs.begin();
			i != m_param->m_par_refs.end();
			i++)
		if (i->m_buffer == bp.first
				&& i->m_param == bp.second)
			return &(*i);

	size_t pi = m_param->m_par_refs.size();
	ParamRef pr;
	pr.m_buffer = bp.first;
	pr.m_param = bp.second;
	pr.m_param_index = pi;
	m_param->m_par_refs.push_back(pr);

	return &m_param->m_par_refs.back();
}

PExpression ParamConverter::ConvertBlock(const block& block) {
	m_vars_map.push_back(frame());
	PExpression ret = ConvertChunk(block.chunk_.get());
	m_vars_map.pop_back();
	return ret;
}

PExpression ParamConverter::ConvertFunction(const identifier& identifier_, const std::vector<expression>& args) {
	std::map<std::wstring, boost::shared_ptr<FunctionConverter> >::iterator i = m_function_converters.find(identifier_);
	if (i == m_function_converters.end())
		throw ParamConversionException(std::wstring(L"Function ") + identifier_ + L" not supported in optimized params");
	return i->second->Convert(args);
}

PExpression ParamConverter::ConvertStatement(const stat& stat_) {
	StatementConverter sc(this);
	return boost::apply_visitor(sc, stat_);
}

PExpression ParamConverter::ConvertChunk(const chunk& chunk_) {
	boost::shared_ptr<ExpressionList> ret = boost::make_shared<ExpressionList>();
	for (std::vector<stat>::const_iterator i = chunk_.stats.begin();
			i != chunk_.stats.end();
			i++)
		ret->AddExpression(ConvertStatement(*i));
	return ret;
}

PExpression ParamConverter::ConvertTerm(const term& term_) {
	TermConverter tc(this);
	return boost::apply_visitor(tc, term_);
}

PExpression ParamConverter::ConvertExpression(const expression& expression_) {
	ExpressionConverter ec(this);
	return ec.ConvertExpression(expression_);
}

PExpression ParamConverter::ConvertPostfixExp(const postfixexp& postfixexp_) {
	PostfixConverter pc(this);
	return boost::apply_visitor(pc, postfixexp_);
}

void ParamConverter::ConvertParam(const chunk& chunk_, Param* param) {
	m_param = param;
	InitalizeVars();
	ConvertChunk(chunk_);
}

void ParamConverter::AddVariable(std::wstring name) {
	m_param->m_vars.push_back(Var(m_param->m_vars.size()));
	(*m_current_frame)[name] = &m_param->m_vars.back();
}

void ParamConverter::InitalizeVars() {
	m_vars_map.clear();
	m_vars_map.resize(1);
	m_current_frame = m_vars_map.begin();
	AddVariable(L"v");
	AddVariable(L"t");
	AddVariable(L"pt");
}

ExecutionEngine::ExecutionEngine(LuaOptDatablock *block) {
	m_buffer = block->buffer;
	szb_lock_buffer(m_buffer);
	m_start_time = probe2time(0, block->year, block->month);
	m_param = block->exec_param;
	for (std::vector<ParamRef>::iterator i = m_param->m_par_refs.begin();
			i != m_param->m_par_refs.end();
			i++) {
		i->SetExecutionEngine(this);
		m_blocks.push_back(szb_get_block(i->m_buffer, i->m_param, block->year, block->month));
	}
	m_vals.resize(m_param->m_vars.size());
	for (size_t i = 0; i < m_vals.size(); i++)
		m_param->m_vars[i].SetExecutionEngine(this);
}

void ExecutionEngine::CalculateValue(time_t t, double &val, bool &fixed) {
	m_fixed = true;
	m_vals[0] = nan("");
	m_vals[1] = t;
	m_vals[2] = PT_MIN10;
	m_param->m_expression.Value();
	fixed = m_fixed;
	val = m_vals[0];
}

void ExecutionEngine::Refresh() {
	for (std::vector<szb_datablock_t*>::iterator i = m_blocks.begin();
			i != m_blocks.end();
			i++) {
		if (*i != NULL)
			(*i)->Refresh();
#if 0
		else {
			std::
			std::vector<ParamRef>::iterator j = m_param->m_par_refs.begin();
			*i = szb_get_block(m_buffer, i->m_param, (*i)->year, (*i)->month)szb_get_block(i->m_buffer, i->m_param, block->year, block->month));
		}
#endif
	}
}

double ExecutionEngine::Value(size_t param_index, double time, double period_type) {
	int probe_index  = (time_t(time) - m_start_time) / SZBASE_PROBE;
	szb_datablock_t* block = m_blocks.at(param_index);
	double ret;
	if (block) {
		if (block->GetFixedProbesCount() < probe_index)
			m_fixed = false;
		ret = block->GetData(false)[probe_index];
	} else {
		m_fixed = false;
		ret = nan("");
	}
	return ret;
}

std::vector<double>& ExecutionEngine::Vars() {
	return m_vals;
}

ExecutionEngine::~ExecutionEngine() {
	szb_unlock_buffer(m_buffer);
}

} // LuaExec

LuaOptDatablock::LuaOptDatablock(szb_buffer_t * b, TParam * p, int y, int m) : LuaDatablock(b, p, y, m) {
	#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: DefinableDatablock::DefinableDatablock(%ls, %d.%d)", param->GetName().c_str(), year, month);
	#endif

	AllocateDataMemory();

	if (year < buffer->first_av_year)
		NOT_INITIALIZED;

	if (year == buffer->first_av_year && month < buffer->first_av_month)
		NOT_INITIALIZED;

	if (LoadFromCache()) {
		Refresh();
		return;
	} else {
		last_update_time = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);
		LuaExec::ExecutionEngine ee(this);
		ee.Refresh();
		CalculateValues(&ee);
	}
}

void LuaOptDatablock::CalculateValues(LuaExec::ExecutionEngine *ee) {
	time_t t = probe2time(first_non_fixed_probe, year, month);
	bool fixed = true;

	for (int i = first_non_fixed_probe; i < max_probes; i++, t += SZBASE_PROBE) {
		bool probe_fixed = true;
		ee->CalculateValue(t, data[i], fixed);
		if (!std::isnan(data[i])) {
			last_data_probe_index = i;
			if (first_data_probe_index < 0)
				first_data_probe_index = i;
		}
		if (fixed && probe_fixed)
			first_non_fixed_probe += 1;
	}
}

void LuaOptDatablock::Refresh() {
	time_t meaner_date = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);
	if (last_update_time != meaner_date) {
		LuaExec::ExecutionEngine ee(this);
		ee.Refresh();
		CalculateValues(&ee);
		last_update_time = meaner_date;
	}
}

#endif

#if LUA_PARAM_OPTIMISE
LuaDatablock *create_lua_data_block(szb_buffer_t *b, TParam* p, int y, int m) {
	LuaExec::Param *ep = p->GetLuaExecParam();
	if (ep == NULL) {
		ep = new LuaExec::Param;
		b->AddExecParam(ep);
		p->SetLuaExecParam(ep);
		lua_grammar::chunk param_code;
		std::wstring param_text = SC::U2S(p->GetLuaScript());
		std::wstring::const_iterator param_text_begin = param_text.begin();
		std::wstring::const_iterator param_text_end = param_text.end();
		ep->m_optimized = false;
		if (!lua_grammar::parse(param_text_begin, param_text_end, param_code)) {
			LuaExec::ParamConverter pc(Szbase::GetObject());
			try {
				pc.ConvertParam(param_code, ep);
				ep->m_optimized = true;
			} catch (LuaExec::ParamConversionException &e) {
				sz_log(2, "Parameter %ls cannot be optimized, reason: %ls", p->GetName().c_str(), e.what().c_str());
			}
		}
	}
	if (ep->m_optimized)
		return new LuaOptDatablock(b, p, y, m);
	else
		return new LuaNativeDatablock(b, p, y, m);


}
#else
LuaDatablock *create_lua_data_block(szb_buffer_t *b, TParam* p, int y, int m) {
	return new LuaNativeDatablock(b, p, y, m);
}
#endif
