#include "lua_syntax.h"

#include "szbase.h"



namespace LuaExec {

typedef double Val;

class ExecutionEngine {
public:

};


class ParamExecutionContext {
	time_t m_start_time;
	std::vector<szb_data_block_t*> m_blocks;
public:	
	void PrepareParam(ParamRef& param_ref) {
		for (size_t i = 0; i < m_par_refs.size(); i++) {


		}
	}
};

struct ParamRef {
	TParam* m_param;
	size_t m_param_index;
	ParamExecutionContext* m_exec_ctx;
public:
	void SetContext(ParamExecutionContext *exec_ctx) { m_exec_ctx = exec_ctx; }
	Val Value(const double &time, const double& period);
};

class Var {
	Val* m_val;
public:
	Val& operator()() { return *m_val; };
	Var& operator=()(const Val* val)  { *m_val = val; return *this; }
	void SetPVal(Val* val) { m_val = val; }
};

struct Param {
	std::vector<Var> m_vars;
	std::vector<ParamRef> m_par_refs;
	std::vector<Expression> m_expressions;
};

class Expression {
public:
	virtual Val Value() = 0;
};

class NilExpression : public Expression {
public:
	virtual Val Value() { return std::nan(); }
};

class NumberExpression : public Expression {
	Var m_val;
public:
	NumberExpression(Val &val) : m_val(val) {}

	virtual Val Value() {
		return m_val();
	}
};

class VarExpression : public Expression {
	Var& m_var;
public:
	VarExpression(Var &var) : m_var(var) {}

	virtual Val Value() {
		return m_var();
	}
};

template<class unop> class UnExpression : public Expression {
	Expression m_e;
	op m_op;
public:
	UnExpression(const Expression& e) : m_e(e) {}

	virtual Val Value() {
		return m_op(m_e);
	}
};

template<class op> class BinExpression : public Expression {
	Expression m_e1, m_e2; 
	op m_op;
public:
	BinExpression(const Expression& e1, const Expression& e2) : m_e1(e1), m_e2(e2) {}

	virtual Val Value() {
		return m_op(m_e1.Value(), m_e2.Value());
	}
};

template<std::logical_or<Val> > class BinExpression : public Expression {
	Expression m_e1, m_e2; 
	op m_op;
public:
	BinExpression(const Expression& e1, const Expression& e2) : m_e1(e1), m_e2(e2) {}

	virtual Val Value() {
		return m_e1.Value() || m_e2.Value();
	}
};

template<std::logical_and<Val> > class BinExpression : public Expression {
	Expression m_e1, m_e2; 
	op m_op;
public:
	BinExpression(const Expression& e1, const Expression& e2) : m_e1(e1), m_e2(e2) {}

	virtual Val Value() {
		return m_e1.Value() && m_e2.Value();
	}
};

class AssignmentExpression : public Expression {
	Var& m_var;
	Expression m_exp;
public:
	AssignmentExpression(Var& m_var, const Expression &exp) : m_var(var), m_exp(exp) {}

	virtual Val Value() {
		return m_var = m_exp.Value();
	}
};

class IsNanExpression : public Expression {
	Expression m_exp;
public:
	IsNanExpression(const Expression &exp) : m_exp(exp) {}

	virtual Val Value() {
		return isnan(m_exp.Value());
	}
};

class ParamValue : public Expression {
	ParamRef& m_param_ref;
	Exression m_time;
	Exression m_avg_type;
public:
	ParamValue(ParamRef& param_ref, const Expression &time, const Expression& avg_type)
		: m_param_ref(param_ref), m_time(time), m_avg_type(avg_type) {}

	virtual Val Value() {
		return m_param_ref.Value(m_time.Value(), m_avg_type.Value());
	}
};

class IfExpression : public Expression {
	Exression m_cond;
	Exression m_consequent;
	std::vector<std::pair<Expression, Expression> > m_elseif;
	Exression m_alternative;
public:
	ParamValue(const Expression& cond,
			const Expression& conseqeunt,
			const std::vector<std::pair<Expression, Expression> >& elseif,
			const Expression& alternative)
		: m_param(param), m_consequent(conseqent), m_elseif(elseif), m_alternative(alternative) {}

	virtual Val Value() {
		if (m_cond.Value())
			return m_consequent.Value()
		else for (std::vector<std::pair<Expression, Expression> >::iterator i = m_elseif.begin();
				i != m_elseif.end();	
				i++) 
			if (i->first.Value())
				return i->second.Value()
		return m_alternative.Value();
	}
};

class SzbMoveTimeExpression : public Expression {
	Expression m_start_time;
	Expression m_displacement;
	Expression m_period_type;
public:
	SzbMoveTimeExpression(const Expression& start_time, const Expression& displacement, const Expression &period_type) : 
		m_start_time(start_time), m_displacement(displacement), m_period_type(period_type) {}

	virtual Val Value() {
		return szb_move_time(m_start_time.Value(), m_displacement.Value(), m_period_type.Value(), 0);
	}
};

class SzbMoveTimeExpression : public Expression {
	Expression m_start_time;
	Expression m_displacement;
	Expression m_period_type;
public:
	SzbMoveTimeExpression(const Expression& start_time, const Expression& displacement, const Expression &period_type) : 
		m_start_time(start_time), m_displacement(displacement), m_period_type(period_type) {}

	virtual Val Value() {
		return szb_move_time(m_start_time.Value(), m_displacement.Value(), m_period_type.Value(), 0);
	}
};

class ForLoopExpression : public Expression { 
	Var& m_var;
	Expression m_start;
	Expression m_limit;
	Expression m_step;
	Expression m_exp;
public:
	ForLoopExpression(const Expression& start, const Expression& limit, const Expression& step, const Expression& exp) :
		m_start(start), m_limit(limit), m_step(step), m_exp(exp) {}

	virtual Val Value() {
		m_var = m_start.Value();
		double limit = m_limit.Value();
		double step = m_step.Value();
		while ((step > 0 && m_var() <= limit)
				|| (step <= 0 && m_var >= limit)) {
			m_exp.Value();
			m_var = m_var() + step;
		}
		return std::nan();
	}

};

class WhileExpression : public Expression {
	Expression m_cond;
	Expression m_exp;
public:
	WhileExpression(const Expression& cond, const Expression& exp) : m_cond(cond), m_exp(exp) {}
	virtual Val Value() {
		while (m_cond.Value())
			m_exp.Value();
	}
};

class RepeatExpression : public Expression {
	Expression m_cond;
	Expression m_exp;
public:
	RepeatExpression(const Expression& cond, const Expression& exp) : m_cond(cond), m_exp(exp) {}
	virtual Val Value() {
		do
			m_exp.Value();
		while (m_cond.Value());
	}
};
