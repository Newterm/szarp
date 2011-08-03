#ifndef LOPTCALCULATE_H__
#define LOPTCALCULATE_H__
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

namespace LuaExec {

class ExecutionEngine;

typedef double Val;

struct ParamRef {
	szb_buffer_t *m_buffer;
	TParam* m_param;
	size_t m_param_index;
	std::list<ExecutionEngine*> m_exec_engines;
	ExecutionEngine* m_exec_engine;
public:
	ParamRef();
	void PushExecutionEngine(ExecutionEngine *exec_engine);
	void PopExecutionEngine();
	Val Value(const double &time, const double& period);
};

class Var {
	size_t m_var_no;
	ExecutionEngine* m_ee;
	std::list<ExecutionEngine*> m_exec_engines;
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

class Statement {
public:
	virtual void Execute() = 0;
};

typedef boost::shared_ptr<Expression> PExpression;
typedef boost::shared_ptr<Statement> PStatement;

class StatementList : public Statement {
	std::vector<PStatement> m_statements;
public:
	void AddStatement(PStatement statement);
	virtual void Execute();
};

class Param {
public:
	bool m_optimized;
	std::vector<Var> m_vars;
	std::vector<ParamRef> m_par_refs;
	std::map<szb_buffer_t*, time_t> m_last_update_times;
	PStatement m_statement;
};

class ExecutionEngine {
	szb_buffer_t* m_buffer;
	std::vector<TParam*> m_params;
	struct ListEntry {
		time_t start_time;
		time_t end_time;
		szb_block_t *block;
	};
	std::vector<std::vector<std::list<ListEntry> > > m_blocks;
	std::vector<std::vector<std::list<ListEntry>::iterator> > m_blocks_iterators;
	std::vector<double> m_vals;
	Param* m_param;
	bool m_fixed;
	ListEntry GetBlockEntry(size_t param_index, time_t t, SZB_BLOCK_TYPE bt);
	szb_block_t* AddBlock(size_t param_index, time_t t, std::list<ListEntry>::iterator& i, SZB_BLOCK_TYPE bt);
	szb_block_t* SearchBlockLeft(size_t param_index, time_t t, std::list<ListEntry>::iterator& i, SZB_BLOCK_TYPE bt);
	szb_block_t* SearchBlockRight(size_t param_index, time_t t, std::list<ListEntry>::iterator& i, SZB_BLOCK_TYPE bt);
	szb_block_t* GetBlock(size_t param_index, time_t time, SZB_BLOCK_TYPE bt);
public:
	ExecutionEngine(szb_buffer_t *buffer, Param *param);

	void CalculateValue(time_t t, SZARP_PROBE_TYPE probe_type, double &val, bool &fixed);

	double Value(size_t param_index, const double& time, const double& period_type);
	double ValueBlock(ParamRef& ref, const time_t& time, SZB_BLOCK_TYPE block_type);
	double ValueAvg(ParamRef& ref, const time_t& time, const double& period_type);

	std::vector<double>& Vars();

	~ExecutionEngine();
};

Param* optimize_lua_param(TParam* p);

}

#endif
