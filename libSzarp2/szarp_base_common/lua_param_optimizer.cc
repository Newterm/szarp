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

#include "conversion.h"
#include "liblog.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "szarp_base_common/lua_param_optimizer.h"

#ifdef LUA_PARAM_OPTIMISE

namespace LuaExec {

ParamRef::ParamRef() {
	m_exec_engine = NULL;
}

void ParamRef::PushExecutionEngine(ExecutionEngine *exec_engine) {
	m_exec_engines.push_back(m_exec_engine);
	m_exec_engine = exec_engine;
}

void ParamRef::PopExecutionEngine() {
	assert(m_exec_engines.size());
	m_exec_engine = m_exec_engines.back();
	m_exec_engines.pop_back();
}

Var::Var(size_t var_no) : m_var_no(var_no), m_ee(NULL) {}

void Var::PushExecutionEngine(ExecutionEngine *exec_engine) {
	m_exec_engines.push_back(m_ee);
	m_ee = exec_engine;
}

void Var::PopExecutionEngine() {
	assert(m_exec_engines.size());
	m_ee = m_exec_engines.back();
	m_exec_engines.pop_back();
}


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

std::wstring get_string_expression(const expression &e) {
	const or_exp& or_exp_ = e;
	if (or_exp_.size() != 1)
		throw ParamConversionException(L"Expression is not string");

	const and_exp& and_exp_ = or_exp_[0];
	if (and_exp_.size() != 1)
		throw ParamConversionException(L"Expression is not string");

	const cmp_exp& cmp_exp_ = and_exp_[0];
	if (cmp_exp_.cmps.size())
		throw ParamConversionException(L"Expression is not string");

	const concat_exp& concat_exp_ = cmp_exp_.concat;
	if (concat_exp_.size() != 1)
		throw ParamConversionException(L"Expression is not string");

	const add_exp& add_exp_ = concat_exp_[0];
	if (add_exp_.adds.size())
		throw ParamConversionException(L"Expression is not string");

	const mul_exp& mul_exp_ = add_exp_.mul;
	if (mul_exp_.muls.size())
		throw ParamConversionException(L"Expression is not string");

	const unop_exp& unop_exp_ = mul_exp_.unop;
	if (unop_exp_.get<0>().size())
		throw ParamConversionException(L"Expression is not string");

	const pow_exp& pow_exp_ = unop_exp_.get<1>();
	if (pow_exp_.size() != 1)
		throw ParamConversionException(L"Expression is not string");

	try {
		const std::wstring& name = boost::get<std::wstring>(pow_exp_[0]);
		return name;
	} catch (boost::bad_get &e) {
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Failed to convert expression to string: " << e.what() << std::endl;
#endif
		throw ParamConversionException(L"Expression is not string");
	}
}

Val ParamRef::Value(const double& time, const double& period) {
	return m_exec_engine->Value(m_param_index, time, period);
}

template<> Val BinExpression<std::logical_or<Val> >::Value() {
	return m_e1->Value() || m_e2->Value();
}

template<> Val BinExpression<std::logical_and<Val> >::Value() {
	return m_e1->Value() && m_e2->Value();
}

template<> Val BinExpression<std::modulus<Val> >::Value() {
	return int(m_e1->Value()) % int(m_e2->Value());
}


}

#include "szarp_base_common/lua_param_optimizer_templ.h"

namespace LuaExec {

template class ParamConverterTempl<IPKContainer>;

bool optimize_lua_param(TParam* param, IPKContainer* container) {
	std::wstring param_text = SC::U2S(param->GetLuaScript());
	std::wstring::const_iterator param_text_begin = param_text.begin();
	std::wstring::const_iterator param_text_end = param_text.end();

	LuaExec::Param* exec_param = param->GetLuaExecParam();
	exec_param->m_optimized = false;	

	lua_grammar::chunk param_code;

	if (lua_grammar::parse(param_text_begin, param_text_end, param_code) && param_text_begin == param_text_end) {
		LuaExec::ParamConverter pc(container);
		try {
			pc.ConvertParam(param_code, exec_param);
			exec_param->m_optimized = true;
		} catch (LuaExec::ParamConversionException &ex) {
			sz_log(3, "Parameter %ls cannot be optimized, reason: %ls", param->GetName().c_str(), ex.what().c_str());
#ifdef LUA_OPTIMIZER_DEBUG
			lua_opt_debug_stream << "Parameter " << SC::S2A(param->GetName()) << " cannot be optimized, reason: " << SC::S2A(ex.what()) << std::endl;
#endif
		}
	} else
		sz_log(3, "Parameter %ls cannot be optimized, failed to parse its defiition", param->GetName().c_str());
	

	return exec_param->m_optimized;
}

}

#endif
