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

#define LUA_OPTIMIZER_DEBUG

#ifdef LUA_OPTIMIZER_DEBUG
#include <fstream>
std::ofstream lua_opt_debug_stream("/tmp/lua_optimizer_debug");
#endif

namespace LuaExec {

using namespace lua_grammar;

class ExecutionEngine;

class EmptyStatement : public Statement {
public:
	virtual void Execute() {}
};

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

class VarRef {
	std::vector<Var>* m_vec;
	size_t m_var_no;
public:
	VarRef() : m_vec(NULL) {}
	VarRef(std::vector<Var>* vec, size_t var_no) : m_vec(vec), m_var_no(var_no) {}
	Var& var() { return (*m_vec)[m_var_no]; }
};

class ParRefRef {
	std::vector<ParamRef>* m_vec;
	size_t m_par_no;
public:
	ParRefRef() : m_vec(NULL) {}
	ParRefRef(std::vector<ParamRef>* vec, size_t par_no) : m_vec(vec), m_par_no(par_no) {}
	ParamRef& par_ref() { return (*m_vec)[m_par_no]; }
};


class VarExpression : public Expression {
	VarRef m_var;
public:
	VarExpression(VarRef var) : m_var(var) {}

	virtual Val Value() {
		return m_var.var()();
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

class AssignmentStatement : public Statement {
	VarRef m_var;
	PExpression m_exp;
public:
	AssignmentStatement(VarRef var, PExpression exp) : m_var(var), m_exp(exp) {}

	virtual void Execute() {
		m_var.var() = m_exp->Value();
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

class InSeasonExpression : public Expression {
	TSzarpConfig* m_sc;
	PExpression m_t;
public:
	InSeasonExpression(TSzarpConfig* sc, PExpression t) : m_sc(sc), m_t(t) {}

	virtual Val Value() {
		struct tm *ptm;
		time_t t = m_t->Value();
#ifdef HAVE_LOCALTIME_R
		struct tm _tm;
		ptm = localtime_r(&t, &_tm);
#else
		ptm = localtime(&time);
#endif
		return m_sc->GetSeasons()->IsSummerSeason(ptm);
	}
};

class ParamValue : public Expression {
	ParRefRef m_param_ref;
	PExpression m_time;
	PExpression m_avg_type;
public:
	ParamValue(ParRefRef param_ref, PExpression time, const PExpression avg_type)
		: m_param_ref(param_ref), m_time(time), m_avg_type(avg_type) {}

	virtual Val Value() {
		return m_param_ref.par_ref().Value(m_time->Value(), m_avg_type->Value());
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


class IfStatement : public Statement {
	PExpression m_cond;
	PStatement m_consequent;
	std::vector<std::pair<PExpression, PStatement> > m_elseif;
	PStatement m_alternative;
public:
	IfStatement(const PExpression cond,
			const PStatement conseqeunt,
			const std::vector<std::pair<PExpression, PStatement> >& elseif,
			const PStatement alternative)
		: m_cond(cond), m_consequent(conseqeunt), m_elseif(elseif), m_alternative(alternative) {}

	virtual void Execute() {
		if (m_cond->Value())
			m_consequent->Execute();
		else {
			for (std::vector<std::pair<PExpression, PStatement> >::iterator i = m_elseif.begin();
					i != m_elseif.end();	
					i++) 
				if (i->first->Value()) {
					i->second->Execute();
					return;
				}
			m_alternative->Execute();
		}
	}

};

class ForLoopStatement : public Statement { 
	VarRef m_var;
	PExpression m_start;
	PExpression m_limit;
	PExpression m_step;
	PStatement m_stat;
public:
	ForLoopStatement(VarRef var, PExpression start, PExpression limit, PExpression step, PStatement stat) :
		m_var(var), m_start(start), m_limit(limit), m_step(step), m_stat(stat) {}

	virtual void Execute() {
		Var& var = m_var.var();
		var() = m_start->Value();
		double limit = m_limit->Value();
		double step = m_step->Value();
		while ((step > 0 && var() <= limit)
				|| (step <= 0 && var() >= limit)) {
			m_stat->Execute();
			var = var() + step;
		}
	}

};

class WhileStatement : public Statement {
	PExpression m_cond;
	PStatement m_stat;
public:
	WhileStatement(PExpression cond, PStatement stat) : m_cond(cond), m_stat(stat) {}

	virtual void Execute() {
		while (m_cond->Value())
			m_stat->Execute();
	}

};

class RepeatStatement : public Statement {
	PExpression m_cond;
	PStatement m_stat;
public:
	RepeatStatement(PExpression cond, PStatement stat) : m_cond(cond), m_stat(stat) {}
	virtual void Execute() {
		do
			m_stat->Execute();
		while (m_cond->Value());
	}
};


class ParamConversionException {
	std::wstring m_error;
public:
	ParamConversionException(std::wstring error) : m_error(error) {}
	const std::wstring& what() const  { return m_error; }
};

class ParamConverter;

class StatementConverter : public boost::static_visitor<PStatement> {
	ParamConverter* m_param_converter;
public:
	StatementConverter(ParamConverter *param_converter) : m_param_converter(param_converter) {}

	PStatement operator() (const assignment &a);

	PStatement operator() (const block &b);

	PStatement operator() (const while_loop &w);

	PStatement operator() (const repeat_loop &r);

	PStatement operator() (const if_stat &if_);

	PStatement operator() (const for_in_loop &a);

	PStatement operator() (const postfixexp &a);

	PStatement operator() (const for_from_to_loop &for_);

	PStatement operator() (const function_declaration &a);

	PStatement operator() (const local_assignment &a);

	PStatement operator() (const local_function_declaration &f);

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

class InSeasonConverter : public FunctionConverter {
public:
	InSeasonConverter(ParamConverter *param_converter) : FunctionConverter(param_converter) {}	

	PExpression Convert(const std::vector<expression>& expressions);
};

class ParamConverter {
	Szbase* m_szbase;
	Param* m_param;
	typedef std::map<std::wstring, VarRef> frame;
	std::vector<frame> m_vars_map;
	std::vector<std::map<std::wstring, VarRef> >::iterator m_current_frame;
	std::map<std::wstring, boost::shared_ptr<FunctionConverter> > m_function_converters;
public:
	ParamConverter(Szbase *szbase);

	VarRef GetGlobalVar(std::wstring identifier);

	VarRef GetLocalVar(const std::wstring& identifier);

	VarRef FindVar(const std::wstring& identifier);

	ParRefRef GetParamRef(const std::wstring param_name);

	PStatement ConvertBlock(const block& block);

	PExpression ConvertFunction(const identifier& identifier_, const std::vector<expression>& args);

	PStatement ConvertStatement(const stat& stat_);

	PStatement ConvertChunk(const chunk& chunk_);

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
	struct BlockListEntry {
		time_t start;
		time_t end;
		szb_datablock_t *block;
		BlockListEntry(time_t start_, time_t end_, szb_datablock_t* block_) : start(start_), end(end_), block(block_) {}
	};
	std::vector<TParam*> m_params;
	std::vector<std::list<BlockListEntry> > m_blocks;
	std::vector<std::list<BlockListEntry>::iterator> m_blocks_iterators;
	std::vector<double> m_vals;
	Param* m_param;
	bool m_fixed;
	BlockListEntry CreateBlock(size_t param_index, time_t t);
	BlockListEntry& AddBlock(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i);
	BlockListEntry& SearchBlockLeft(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i);
	BlockListEntry& SearchBlockRight(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i);
	BlockListEntry& GetBlock(size_t param_index, time_t time);
public:
	ExecutionEngine(LuaOptDatablock *block);

	void CalculateValue(time_t t, double &val, bool &fixed);

	void Refresh();

	double Value(size_t param_index, const double& time, const double& period_type);

	std::vector<double>& Vars();

	~ExecutionEngine();
};

Val& Var::operator()() {
	return m_ee->Vars()[m_var_no];
}

Val& Var::operator=(const Val& val) {
	return m_ee->Vars()[m_var_no] = val;
}

void StatementList::AddStatement(PStatement statement) {
	m_statements.push_back(statement);
}

void StatementList::Execute() {
	size_t l = m_statements.size();
	for (size_t i = 0; i < l; i++)
		m_statements[i]->Execute();
}

PStatement StatementConverter::operator() (const assignment &a) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting assignement" << std::endl;
#endif
#if 0
	for (size_t i = 0; i < a.varlist.size(); i++) {
#else
		size_t i = 0;
#endif
		try {
			const identifier& identifier_ = boost::get<identifier>(a.varlist[i]);
#ifdef LUA_OPTIMIZER_DEBUG
			lua_opt_debug_stream << "Identifier: " << SC::S2A(identifier_) << std::endl;
#endif
			VarRef variable = m_param_converter->GetGlobalVar(identifier_);
			PExpression expression;
			if (i < a.explist.size())
				expression = m_param_converter->ConvertExpression(a.explist[i]);
			else
				expression = boost::make_shared<NilExpression>();
			return boost::make_shared<AssignmentStatement>(variable, expression);
		} catch (boost::bad_get&) {
			throw ParamConversionException(L"Only assignment to variables allowed in optimized expressions");
		}
#if 0
	}
#endif
}

PStatement StatementConverter::operator() (const block &b) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream <<  "Coverting block" << std::endl;
#endif
	return m_param_converter->ConvertBlock(b);
}

PStatement StatementConverter::operator() (const while_loop &w) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting while loop" << std::endl;
#endif
	PExpression condition = m_param_converter->ConvertExpression(w.expression_);
	PStatement block = m_param_converter->ConvertBlock(w.block_);
	return boost::make_shared<WhileStatement>(condition, block);
}

PStatement StatementConverter::operator() (const repeat_loop &r) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting while loop" << std::endl;
#endif
	PExpression condition = m_param_converter->ConvertExpression(r.expression_);
	PStatement block = m_param_converter->ConvertBlock(r.block_);
	return boost::make_shared<RepeatStatement>(condition, block);
}

PStatement StatementConverter::operator() (const if_stat &if_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting if statement" << std::endl;
	lua_opt_debug_stream << "Converting if expression" << std::endl;
#endif
	PExpression cond = m_param_converter->ConvertExpression(if_.if_exp);
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting if block" << std::endl;
#endif
	PStatement block = m_param_converter->ConvertBlock(if_.block_);
	std::vector<std::pair<PExpression, PStatement> > elseif;
	for (size_t i = 0; i < if_.elseif_.size(); i++) {
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Converting elseif" << std::endl;
#endif
		elseif.push_back(std::make_pair(m_param_converter->ConvertExpression(if_.elseif_[i].get<0>()),
				m_param_converter->ConvertBlock(if_.elseif_[i].get<1>())));
	}
	PStatement else_;
	if (if_.else_) {
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Converting else" << std::endl;
#endif
		else_ = m_param_converter->ConvertBlock(*if_.else_);
	} else
		else_ = boost::make_shared<EmptyStatement>();
	return boost::make_shared<IfStatement>(cond, block, elseif, else_);
}

PStatement StatementConverter::operator() (const for_in_loop &a) {
	throw ParamConversionException(L"For in loops not supported by converter");
}

PStatement StatementConverter::operator() (const postfixexp &a) {
	throw ParamConversionException(L"Postfix expressions as statments not supported by converter");
}

PStatement StatementConverter::operator() (const for_from_to_loop &for_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream <<  "Converting for from to loop" << std::endl;
#endif
	VarRef var = m_param_converter->GetLocalVar(for_.identifier_);
	PExpression from = m_param_converter->ConvertExpression(for_.from);
	PExpression to = m_param_converter->ConvertExpression(for_.to);
	PExpression step;
	if (for_.step)
		step = m_param_converter->ConvertExpression(*for_.step);
	else
		step = boost::make_shared<NumberExpression>(1);
	PStatement block = m_param_converter->ConvertBlock(for_.block_);
	return boost::make_shared<ForLoopStatement>(var, from, to, step, block);
}

PStatement StatementConverter::operator() (const function_declaration &a) {
	throw ParamConversionException(L"Function declarations not supported by converter");
}

PStatement StatementConverter::operator() (const local_assignment &a) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting local assignment" << std::endl;
#endif
	PExpression expression;
#if 0
	for (size_t i = 0; i < a.varlist.size(); i++) {
#else
	size_t i = 0;
#endif
		try {
			const identifier& identifier_ = boost::get<identifier>(a.varlist[i]);
#ifdef LUA_OPTIMIZER_DEBUG
			lua_opt_debug_stream << "Identifier: " << SC::S2A(identifier_) << std::endl;
#endif
			VarRef variable = m_param_converter->GetLocalVar(identifier_);
			if (i < a.explist.size())
				expression = m_param_converter->ConvertExpression(a.explist[i]);
			else
				expression = boost::make_shared<NilExpression>();
			return boost::make_shared<AssignmentStatement>(variable, expression);
		} catch (boost::bad_get&) {
			throw ParamConversionException(L"Only assignment to variables allowed in optimized expressions");
		}
#if 0
	}
#endif
}

PStatement StatementConverter::operator() (const local_function_declaration &f) {
	throw ParamConversionException(L"Function declarations not supported by converter");
}

PExpression ExpressionConverter::ConvertTerm(const term& term_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting term" << std::endl;
#endif
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
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Converting power expression" << std::endl;
#endif
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
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting negate unop expression" << std::endl;
#endif
				p = boost::make_shared<UnExpression<std::negate<Val> > >(p);
				break;
			case NOT:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting not unop expression" << std::endl;
#endif
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
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting mul binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::multiplies<Val> > >(p, p2);
				break;
			case DIV:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting div binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::divides<Val> > >(p, p2);
				break;
			case REM:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting rem binop expression" << std::endl;
#endif
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
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting plus binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::plus<Val> > >(p, p2);
				break;
			case MINUS:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting minus binop expression" << std::endl;
#endif
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
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting less binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::less<Val> > >(p, p2);
				break;
			case LTE:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting less-equal binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::less_equal<Val> > >(p, p2);
				break;
			case EQ:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting equal binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::equal_to<Val> > >(p, p2);
				break;
			case GTE:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting greater equal binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::greater_equal<Val> > >(p, p2);
				break;
			case GT:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting greater binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::greater<Val> > >(p, p2);
				break;
			case NEQ:
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Converting not equal binop expression" << std::endl;
#endif
				p = boost::make_shared<BinExpression<std::not_equal_to<Val> > >(p, p2);
				break;
		}
	}
	return p;
}

PExpression ExpressionConverter::ConvertAnd(const and_exp& and_exp_) {
	PExpression p = ConvertCmp(and_exp_[0]);
	for (size_t i = 1; i < and_exp_.size(); i++) {
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Converting and binop expression" << std::endl;
#endif
		p = boost::make_shared<BinExpression<std::logical_and<Val> > >(p, ConvertCmp(and_exp_[i]));
	}
	return p;
}

PExpression ExpressionConverter::ConvertOr(const or_exp& or_exp_) {
	PExpression p = ConvertAnd(or_exp_[0]);
	for (size_t i = 1; i < or_exp_.size(); i++) {
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Converting or binop expression" << std::endl;
#endif
		p = boost::make_shared<BinExpression<std::logical_or<Val> > >(p, ConvertAnd(or_exp_[i]));
	}
	return p;
}

PExpression ExpressionConverter::ConvertExpression(const expression& expression_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting expression" << std::endl;
#endif
	return ConvertOr(expression_.o);
}

PExpression TermConverter::operator()(const nil& nil_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting nil expression" << std::endl;
#endif
	return boost::make_shared<NilExpression>();
}

PExpression TermConverter::operator()(const bool& bool_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting boolean value" << std::endl;
#endif
	return boost::make_shared<NumberExpression>(bool_ ? 1. : 0.);
}

PExpression TermConverter::operator()(const double& double_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting number expression" << std::endl;
#endif
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
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting postfix expression" << std::endl;
#endif
	return m_param_converter->ConvertPostfixExp(postfixexp_);
}

PExpression TermConverter::operator()(const expression& expression_) {
	return m_param_converter->ConvertExpression(expression_);
}

PExpression PostfixConverter::operator()(const identifier& identifier_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting variable expression, var: " << SC::S2A(identifier_) << std::endl;
#endif
	return boost::make_shared<VarExpression>(m_param_converter->FindVar(identifier_));
}

const std::vector<expression>& PostfixConverter::GetArgs(const std::vector<exp_ident_arg_namearg>& e) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting args, number of exp_ident_arg_namerg :) expressions:" << e.size() << std::endl;
#endif
	if (e.size() != 1)
		throw ParamConversionException(L"Only postfix expression in form functioname(exp, exp, ...) are allowed");
	try {
		const namearg& namearg_ = boost::get<namearg>(e[0]);
		const args& args_ = boost::get<args>(namearg_);
/*
		const boost::recursive_wrapper<std::vector<expression> > &exps_
			= boost::get<boost::recursive_wrapper<std::vector<expression> > >(args_);
*/
		const std::vector<expression> &exps_
			= boost::get<std::vector<expression> >(args_);
		return exps_;
	} catch (boost::bad_get &e) {
		throw ParamConversionException(L"Only postfix expression in form functioname(exp, exp, ...) are allowed");
	}
}

PExpression PostfixConverter::operator()(const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &exp) {
	try {
		const exp_identifier& ei = exp.get<0>();
		const identifier& identifier_ = boost::get<identifier>(ei);
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Converting postfix expression, identifier: " << SC::S2A(identifier_) << std::endl;
#endif
		const std::vector<expression>& args = GetArgs(exp.get<1>());
		return m_param_converter->ConvertFunction(identifier_, args);
	} catch (boost::bad_get &e) {
		throw ParamConversionException(L"Postfix expression (functioncall) must start with identifier in optimized params");
	}
}

PExpression SzbMoveTimeConverter::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting szb_move_time expression" << std::endl;
#endif
	if (expressions.size() < 3)
		throw ParamConversionException(L"szb_move_time requires three arguments");

	return boost::make_shared<SzbMoveTimeExpression>
		(m_param_converter->ConvertExpression(expressions[0]),
		 m_param_converter->ConvertExpression(expressions[1]),
		 m_param_converter->ConvertExpression(expressions[2]));
}

PExpression IsNanConverter::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting isnan expression"  << std::endl;
#endif
	if (expressions.size() < 1)
		throw ParamConversionException(L"isnan takes one argument");

	return boost::make_shared<IsNanExpression>
		(m_param_converter->ConvertExpression(expressions[0]));
}

PExpression NanConverter::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting nan expression" << std::endl;
#endif
	return boost::make_shared<NilExpression>();
}

std::wstring get_string_expression(const expression &e) {
#define ERROR throw ParamConversionException(L"Expression is not string")
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

	try {
		const std::wstring& name = boost::get<std::wstring>(pow_exp_[0]);
		return name;
	} catch (boost::bad_get &e) {
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Failed to convert expression to string: " << e.what() << std::endl;
#endif
		ERROR;
	}
}

std::wstring ParamValueConverter::GetParamName(const expression& e) {
	try {
		return get_string_expression(e);
	} catch (const ParamConversionException& e) {
		throw ParamConversionException(L"First parameter of p function should be literal string");
	}

}

PExpression ParamValueConverter::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting p(..) expression" << std::endl;
#endif
	if (expressions.size() < 3)
		throw ParamConversionException(L"p function requires three arguemnts");
	std::wstring param_name = GetParamName(expressions[0]);
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Parameter name: " << SC::S2A(param_name) << std::endl;
#endif
	ParRefRef param_ref = m_param_converter->GetParamRef(param_name);
	return boost::make_shared<ParamValue>(param_ref,
			m_param_converter->ConvertExpression(expressions[1]),
			m_param_converter->ConvertExpression(expressions[2]));
}

PExpression InSeasonConverter::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting isnan(..) expression" << std::endl;
#endif
	if (expressions.size() < 2)
		throw ParamConversionException(L"in season function requires two arguments");
	std::wstring prefix;
	try {
		prefix = get_string_expression(expressions[0]);
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "in_season parameter: " << SC::S2A(prefix) << std::endl;
#endif
	} catch (ParamConversionException &e) {
		throw ParamConversionException(L"First argument to in_season function should be literal configuration prefix");
	}

	IPKContainer* ic = IPKContainer::GetObject();
	TSzarpConfig* sc = ic->GetConfig(prefix);
	if (sc == NULL)
		throw ParamConversionException(std::wstring(L"Prefix ") + prefix + L" given in in_season not found");
	return boost::make_shared<InSeasonExpression>(sc, m_param_converter->ConvertExpression(expressions[1]));
}

ParamConverter::ParamConverter(Szbase *szbase) : m_szbase(szbase) {
	m_function_converters[L"szb_move_time"] = boost::make_shared<SzbMoveTimeConverter>(this);
	m_function_converters[L"isnan"] = boost::make_shared<IsNanConverter>(this);
	m_function_converters[L"nan"] = boost::make_shared<NanConverter>(this);
	m_function_converters[L"p"] = boost::make_shared<ParamValueConverter>(this);
	m_function_converters[L"in_season"] = boost::make_shared<InSeasonConverter>(this);
}

VarRef ParamConverter::GetGlobalVar(std::wstring identifier) {
	std::map<std::wstring, VarRef>::iterator i = m_vars_map[0].find(identifier);
	if (i == m_vars_map[0].end()) {
		size_t s = m_param->m_vars.size();
		m_param->m_vars.push_back(s);
		VarRef i(&m_param->m_vars, s);
		m_vars_map[0][identifier] = i;
		return i;
	} else {
		return i->second;
	}
}

VarRef ParamConverter::GetLocalVar(const std::wstring& identifier) {
	std::map<std::wstring, VarRef>::iterator i = m_vars_map.back().find(identifier);
	if (i == m_vars_map.back().end()) {
		size_t s = m_param->m_vars.size();
		m_param->m_vars.push_back(s);
		VarRef i(&m_param->m_vars, s);
		m_vars_map.back()[identifier] = i;
		return i;
	} else {
		return i->second;
	}
}

VarRef ParamConverter::FindVar(const std::wstring& identifier) {
	for (std::vector<frame>::reverse_iterator i = m_vars_map.rbegin();
			i != m_vars_map.rend();
			i++) {
		std::map<std::wstring, VarRef>::iterator j = i->find(identifier);
		if (j != i->end())
			return j->second;
	}
	throw ParamConversionException(std::wstring(L"Variable ") + identifier + L" is unbound");
}

ParRefRef ParamConverter::GetParamRef(const std::wstring param_name) {
	std::pair<szb_buffer_t*, TParam*> bp;
	if (m_szbase->FindParam(param_name, bp) == false)
		throw ParamConversionException(std::wstring(L"Param ") + param_name + L" not found");

	for (size_t i = 0; i < m_param->m_par_refs.size(); i++)
		if (m_param->m_par_refs[i].m_buffer == bp.first && m_param->m_par_refs[i].m_param == bp.second)
			return ParRefRef(&m_param->m_par_refs, i);

	size_t pi = m_param->m_par_refs.size();
	ParamRef pr;
	pr.m_buffer = bp.first;
	pr.m_param = bp.second;
	pr.m_param_index = pi;
	m_param->m_par_refs.push_back(pr);

	return ParRefRef(&m_param->m_par_refs, pi);
}

PStatement ParamConverter::ConvertBlock(const block& block) {
	m_vars_map.push_back(frame());
	PStatement ret = ConvertChunk(block.chunk_.get());
	m_vars_map.pop_back();
	return ret;
}

PExpression ParamConverter::ConvertFunction(const identifier& identifier_, const std::vector<expression>& args) {
	std::map<std::wstring, boost::shared_ptr<FunctionConverter> >::iterator i = m_function_converters.find(identifier_);
	if (i == m_function_converters.end())
		throw ParamConversionException(std::wstring(L"Function ") + identifier_ + L" not supported in optimized params");
	return i->second->Convert(args);
}

PStatement ParamConverter::ConvertStatement(const stat& stat_) {
	StatementConverter sc(this);
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting statement" << std::endl;
#endif
	return boost::apply_visitor(sc, stat_);
}

PStatement ParamConverter::ConvertChunk(const chunk& chunk_) {
	boost::shared_ptr<StatementList> ret = boost::make_shared<StatementList>();
	for (std::vector<stat>::const_iterator i = chunk_.stats.begin();
			i != chunk_.stats.end();
			i++)
		ret->AddStatement(ConvertStatement(*i));
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
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting postfix expression" << std::endl;
#endif
	PostfixConverter pc(this);
	return boost::apply_visitor(pc, postfixexp_);
}

void ParamConverter::ConvertParam(const chunk& chunk_, Param* param) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Starting to optimize param" << std::endl;
#endif
	m_param = param;
	InitalizeVars();
	param->m_statement = ConvertChunk(chunk_);
}

void ParamConverter::AddVariable(std::wstring name) {
	size_t s = m_param->m_vars.size();
	m_param->m_vars.push_back(Var(s));
	VarRef i(&m_param->m_vars, s);
	(*m_current_frame)[name] = i;
}

void ParamConverter::InitalizeVars() {
	m_vars_map.clear();
	m_vars_map.resize(1);
	m_current_frame = m_vars_map.begin();
	AddVariable(L"v");
	AddVariable(L"t");
	AddVariable(L"pt");
	AddVariable(L"PT_MIN10");
	AddVariable(L"PT_HOUR");
	AddVariable(L"PT_HOUR8");
	AddVariable(L"PT_DAY");
	AddVariable(L"PT_WEEK");
	AddVariable(L"PT_MONTH");
}

ExecutionEngine::ExecutionEngine(LuaOptDatablock *block) {
	m_buffer = block->buffer;
	szb_lock_buffer(m_buffer);
	m_start_time = probe2time(0, block->year, block->month);
	m_param = block->exec_param;
	m_blocks.resize(m_param->m_par_refs.size());
	m_blocks_iterators.resize(m_param->m_par_refs.size());
	for (size_t i = 0; i < m_param->m_par_refs.size(); i++) {
		m_param->m_par_refs[i].SetExecutionEngine(this);
		m_blocks[i] = std::list<BlockListEntry>(1, CreateBlock(i, m_start_time));
		m_blocks_iterators[i] = m_blocks[i].begin();
	}
	m_vals.resize(m_param->m_vars.size());
	for (size_t i = 0; i < m_vals.size(); i++)
		m_param->m_vars[i].SetExecutionEngine(this);
	m_vals[3] = PT_MIN10;
	m_vals[4] = PT_HOUR;
	m_vals[5] = PT_HOUR8;
	m_vals[6] = PT_WEEK;
	m_vals[7] = PT_MONTH;
}

void ExecutionEngine::CalculateValue(time_t t, double &val, bool &fixed) {
	m_fixed = true;
	m_vals[0] = nan("");
	m_vals[1] = t;
	m_vals[2] = PT_MIN10;
	m_param->m_statement->Execute();
	fixed = m_fixed;
	val = m_vals[0];
}

void ExecutionEngine::Refresh() {
	for (size_t i = 0; i < m_param->m_par_refs.size(); i++)
		for (std::list<BlockListEntry>::iterator j =  m_blocks[i].begin();
				j != m_blocks[i].end();
				j++)
			if (j->block)
				j->block->Refresh();
			else {
				BlockListEntry b = CreateBlock(i, j->start);
				j->block = b.block;
			}
}

ExecutionEngine::BlockListEntry ExecutionEngine::CreateBlock(size_t param_index, time_t t) {
	int year, month;
	szb_time2my(t, &year, &month);
	time_t start = probe2time(0, year, month);
	time_t end = start + SZBASE_PROBE * szb_probecnt(year, month);
	szb_datablock_t *block = szb_get_block(m_param->m_par_refs[param_index].m_buffer, m_param->m_par_refs[param_index].m_param, year, month);
	return BlockListEntry(start, end, block);
}

ExecutionEngine::BlockListEntry& ExecutionEngine::AddBlock(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i) {
	BlockListEntry ble(CreateBlock(param_index, t));
	m_blocks[param_index].insert(i, ble);
	std::advance(i, -1);
	return *i;
}

ExecutionEngine::BlockListEntry& ExecutionEngine::SearchBlockLeft(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i) {
	do {
		if (i == m_blocks[param_index].begin())
			return AddBlock(param_index, t, i);

		if ((--i)->end <= t)
			return AddBlock(param_index, t, ++i);

		if (i->start <= t)
			return *i;
		
	} while (true);	

}

ExecutionEngine::BlockListEntry& ExecutionEngine::SearchBlockRight(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i) {
	do {
		if (++i == m_blocks[param_index].end())
			return AddBlock(param_index, t, i);

		if (i->start > t)
			return AddBlock(param_index, t, i);

		if (i->end > t)
			return *i;
		
	} while (true);	

}

ExecutionEngine::BlockListEntry& ExecutionEngine::GetBlock(size_t param_index, time_t time) {
	std::list<BlockListEntry>::iterator& i = m_blocks_iterators[param_index];
	if (i->start <= time && time < i->end)
		return *i;
	else if (i->start > time)
		return SearchBlockLeft(param_index, time, i);
	else
		return SearchBlockRight(param_index, time, i);
}

double ExecutionEngine::Value(size_t param_index, const double& time_, const double& period_type) {
	time_t time = time_;
	double ret;

	if (period_type == PT_MIN10) {
		BlockListEntry& block = GetBlock(param_index, time);
		if (block.block) {
			int probe_index  = (time - block.start) / SZBASE_PROBE;
			if (block.block->GetFixedProbesCount() < probe_index)
				m_fixed = false;
			ret = block.block->GetData(false)[probe_index];
		} else {
			m_fixed = false;
			ret = nan("");
		}
	} else {
		bool fixed;
		ParamRef& v = m_param->m_par_refs[param_index];	
		ret = szb_get_avg(v.m_buffer, v.m_param, time, szb_move_time(time, 1, (SZARP_PROBE_TYPE)period_type, 0), NULL, NULL, (SZARP_PROBE_TYPE)period_type, &fixed);
		if (!fixed)
			m_fixed = false;
	}
	return ret;
}

std::vector<double>& ExecutionEngine::Vars() {
	return m_vals;
}

ExecutionEngine::~ExecutionEngine() {
	szb_unlock_buffer(m_buffer);
}

Val ParamRef::Value(const double& time, const double& period) {
	return m_exec_engine->Value(m_param_index, time, period);
}

} // LuaExec

LuaOptDatablock::LuaOptDatablock(szb_buffer_t * b, TParam * p, int y, int m) : LuaDatablock(b, p, y, m) {
#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: DefinableDatablock::DefinableDatablock(%ls, %d.%d)", param->GetName().c_str(), year, month);
#endif
	AllocateDataMemory();
	exec_param = p->GetLuaExecParam();
	if (year < buffer->first_av_year)
		NOT_INITIALIZED;
	if (year == buffer->first_av_year && month < buffer->first_av_month)
		NOT_INITIALIZED;
	int year, month;
	time_t end_date = szb_search_last(buffer, param);
	szb_time2my(end_date, &year, &month);
	if (this->year > year || (this->year == year && this->month > month))
		NOT_INITIALIZED;
	if (end_date > GetBlockLastDate())
		end_date = GetBlockLastDate();
	int probes_to_compute = szb_probeind(end_date) + 1;
	for (int i = probes_to_compute; i < max_probes; i++)
		data[i] = SZB_NODATA;

	last_update_time = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);
	if (LoadFromCache()) {
		Refresh();
		return;
	} else {
		LuaExec::ExecutionEngine ee(this);
		ee.Refresh();
		CalculateValues(&ee, probes_to_compute);
	}
}

void LuaOptDatablock::CalculateValues(LuaExec::ExecutionEngine *ee, int end_probe) {
	time_t t = probe2time(first_non_fixed_probe, year, month);

	for (int i = first_non_fixed_probe; i < end_probe; i++, t += SZBASE_PROBE) {
		bool probe_fixed = true;
		ee->CalculateValue(t, data[i], probe_fixed);
		if (!std::isnan(data[i])) {
			last_data_probe_index = i;
			if (first_data_probe_index < 0)
				first_data_probe_index = i;
		}
		if (probe_fixed && first_non_fixed_probe == i)
			first_non_fixed_probe = i + 1;
		
	}
}

void LuaOptDatablock::Refresh() {
	if (first_non_fixed_probe == max_probes)
		return;

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);
	if (last_update_time == updatetime)
		return;

	time_t end_date = szb_search_last(buffer, param);
	if (end_date > GetBlockLastDate())
		end_date = GetBlockLastDate();

	int end_probe = szb_probeind(end_date) + 1;
	LuaExec::ExecutionEngine ee(this);
	ee.Refresh();
	CalculateValues(&ee, end_probe);
	last_update_time = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);
}

#endif

#if LUA_PARAM_OPTIMISE
LuaDatablock *create_lua_data_block(szb_buffer_t *b, TParam* p, int y, int m) {
	LuaExec::Param *ep = p->GetLuaExecParam();
	if (ep == NULL) {
		ep = new LuaExec::Param;
		p->SetLuaExecParam(ep);
		b->AddExecParam(p);
		lua_grammar::chunk param_code;
		std::wstring param_text = SC::U2S(p->GetLuaScript());
		std::wstring::const_iterator param_text_begin = param_text.begin();
		std::wstring::const_iterator param_text_end = param_text.end();
		ep->m_optimized = false;
		if (lua_grammar::parse(param_text_begin, param_text_end, param_code)) {
			Szbase* szbase = Szbase::GetObject();
			LuaExec::ParamConverter pc(szbase);
			try {
				pc.ConvertParam(param_code, ep);
				ep->m_optimized = true;
				for (std::vector<LuaExec::ParamRef>::iterator i = ep->m_par_refs.begin();
					 	i != ep->m_par_refs.end();
						i++)
					szbase->AddLuaOptParamReference(i->m_param, p);
			} catch (LuaExec::ParamConversionException &e) {
				sz_log(3, "Parameter %ls cannot be optimized, reason: %ls", p->GetName().c_str(), e.what().c_str());
#ifdef LUA_OPTIMIZER_DEBUG
				lua_opt_debug_stream << "Parameter " << SC::S2A(p->GetName()) << " cannot be optimized, reason: " << SC::S2A(e.what()) << std::endl;
#endif
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
