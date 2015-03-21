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
#ifndef __LUA_PARAM_OPTIMIZER_H__
#define __LUA_PARAM_OPTIMIZER_H__

#ifdef LUA_PARAM_OPTIMISE

#include <list>

#include <boost/variant.hpp>
#include "szarp_base_common/defines.h"
#include "szarp_base_common/time.h"
#include "szarp_base_common/lua_syntax.h"
#include "szarp_config.h"

namespace LuaExec {

typedef double Val;

using namespace lua_grammar;

class ExecutionEngine {
public:
	virtual double Value(size_t param_index, const double& time_, const double& period_type) = 0;
	virtual std::vector<double>& Vars() = 0;
	virtual ~ExecutionEngine() {}
};

struct ParamRef {
	TParam* m_param;
	size_t m_param_index;
	ExecutionEngine* m_exec_engine;
	std::vector<ExecutionEngine*> m_exec_engines;
public:
	ParamRef();
	void PushExecutionEngine(ExecutionEngine *exec_engine);
	void PopExecutionEngine();
	Val Value(const double &time, const double& period);
};

class Var {
	size_t m_var_no;
	ExecutionEngine* m_ee;
	std::vector<ExecutionEngine*> m_exec_engines;
public:
	Var(size_t var_no);
	Val& operator()();
	Val& operator=(const Val& val);
	void PushExecutionEngine(ExecutionEngine *exec_engine);
	void PopExecutionEngine();
};

class Expression {
public:
	virtual Val Value() = 0;
};
typedef boost::shared_ptr<Expression> PExpression;

class Statement {
public:
	virtual void Execute() = 0;
};
typedef boost::shared_ptr<Statement> PStatement;

struct Param {
        bool m_optimized;
        std::vector<Var> m_vars;
        std::vector<ParamRef> m_par_refs;
        PStatement m_statement;
	virtual ~Param() {}
};

class StatementList : public Statement {
	std::vector<PStatement> m_statements;
public:
	void AddStatement(PStatement statement);
	virtual void Execute();
};

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

template<> Val BinExpression<std::logical_or<Val> >::Value();

template<> Val BinExpression<std::logical_and<Val> >::Value();

template<> Val BinExpression<std::modulus<Val> >::Value();

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
		time_t t = m_t->Value();
		return m_sc->GetSeasons()->IsSummerSeason(t);
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

class SzbRoundTimeExpression : public Expression {
	PExpression m_time;
	PExpression m_period_type;
public:
	SzbRoundTimeExpression(PExpression time, PExpression period_type) :
		m_time(time), m_period_type(period_type) {}

	virtual Val Value() {
		return szb_round_time(m_time->Value(),  SZARP_PROBE_TYPE(m_period_type->Value()));
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

template<class T> class FunctionConverter;

class ParamConversionException {
	std::wstring m_error;
public:
	ParamConversionException(std::wstring error) : m_error(error) {}
	const std::wstring& what() const  { return m_error; }
};

template<class container_type> class ParamConverterTempl {
	container_type* m_ipk_container;
	Param* m_param;
	typedef std::map<std::wstring, VarRef> frame;
	std::vector<frame> m_vars_map;
	std::vector<std::map<std::wstring, VarRef> >::iterator m_current_frame;
	std::map<std::wstring, boost::shared_ptr<FunctionConverter<container_type> > > m_function_converters;
public:
	ParamConverterTempl(container_type* container);

	VarRef GetGlobalVar(std::wstring identifier);

	VarRef GetLocalVar(const std::wstring& identifier);

	VarRef FindVar(const std::wstring& identifier);

	ParRefRef GetParamRef(const std::wstring param_name);

	PStatement ConvertBlock(const block& block);

	PExpression ConvertFunction(const identifier& identifier_, const std::vector<expression>& args);

	PStatement ConvertStatement(const lua_grammar::stat& stat_);

	PStatement ConvertChunk(const chunk& chunk_);

	PExpression ConvertTerm(const term& term_);
	
	PExpression ConvertExpression(const expression& expression_);

	PExpression ConvertPostfixExp(const postfixexp& postfixexp_);

	void ConvertParam(const chunk& chunk_, Param* param);

	void AddVariable(std::wstring name);

	void InitalizeVars();

};

template<class container_type> class StatementConverter : public boost::static_visitor<PStatement> {
	ParamConverterTempl<container_type>* m_param_converter;
public:
	StatementConverter(ParamConverterTempl<container_type> *param_converter) : m_param_converter(param_converter) {}

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

template<class container_type> class ExpressionConverter {
	ParamConverterTempl<container_type>* m_param_converter;

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
	ExpressionConverter(ParamConverterTempl<container_type> *param_converter) : m_param_converter(param_converter) {}
	PExpression ConvertExpression(const expression& expression_);
};

template<class container_type> class TermConverter : public boost::static_visitor<PExpression> {
	ParamConverterTempl<container_type>* m_param_converter;
public:
	TermConverter(ParamConverterTempl<container_type> *param_converter) : m_param_converter(param_converter) { }
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

template<class container_type> class PostfixConverter : public boost::static_visitor<PExpression> {
	ParamConverterTempl<container_type>* m_param_converter;
public:
	PostfixConverter(ParamConverterTempl<container_type>* param_converter) : m_param_converter(param_converter) {}

	PExpression operator()(const identifier& identifier_);

	const std::vector<expression>& GetArgs(const std::vector<exp_ident_arg_namearg>& e);

	PExpression operator()(const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &exp);
};

template<class container_type> class FunctionConverter { 
protected:
	ParamConverterTempl<container_type>* m_param_converter;
public:
	FunctionConverter(ParamConverterTempl<container_type> *param_converter) : m_param_converter(param_converter) {}	

	virtual PExpression Convert(const std::vector<expression>& expressions) = 0;
};

template<class container_type> class SzbRoundTimeConverter : public FunctionConverter<container_type> {
public:
	SzbRoundTimeConverter(ParamConverterTempl<container_type> *param_converter) : FunctionConverter<container_type>(param_converter) {}	

	virtual PExpression Convert(const std::vector<expression>& expressions);
};


template<class container_type> class SzbMoveTimeConverter : public FunctionConverter<container_type> {
public:
	SzbMoveTimeConverter(ParamConverterTempl<container_type> *param_converter) : FunctionConverter<container_type>(param_converter) {}	

	virtual PExpression Convert(const std::vector<expression>& expressions);
};

template<class container_type> class IsNanConverter : public FunctionConverter<container_type> {
public:
	IsNanConverter(ParamConverterTempl<container_type> *param_converter) : FunctionConverter<container_type>(param_converter) {}	

	virtual PExpression Convert(const std::vector<expression>& expressions);
};

template<class container_type> class NanConverter : public FunctionConverter<container_type> {
public:
	NanConverter(ParamConverterTempl<container_type> *param_converter) : FunctionConverter<container_type>(param_converter) {}	

	PExpression Convert(const std::vector<expression>& expressions);
};

template<class container_type> class ParamValueConverter : public FunctionConverter<container_type> {
	std::wstring GetParamName(const expression& e);
public:
	ParamValueConverter(ParamConverterTempl<container_type> *param_converter) : FunctionConverter<container_type>(param_converter) {}	

	PExpression Convert(const std::vector<expression>& expressions);
};

template<class container_type> class InSeasonConverter : public FunctionConverter<container_type> {
	container_type* m_ipk_container;
public:
	InSeasonConverter(ParamConverterTempl<container_type> *param_converter, container_type* container) : FunctionConverter<container_type>(param_converter), m_ipk_container(container) {}	

	PExpression Convert(const std::vector<expression>& expressions);
};

typedef ParamConverterTempl<IPKContainer> ParamConverter;

template<class ipk_container_type> bool optimize_lua_param(TParam* param, ipk_container_type* container);

}

#endif /* LUA_PARAM_OPTIMISE */

#endif /* __LUA_PARAM_OPTIMIZER_H__ */
