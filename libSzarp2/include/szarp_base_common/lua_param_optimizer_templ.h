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
#ifndef __LUA_PARAM_TEMPL_H__
#define __LUA_PARAM_TEMPL_H__

#include <boost/make_shared.hpp>

namespace LuaExec {

//#define LUA_OPTIMIZER_DEBUG

#ifdef LUA_OPTIMIZER_DEBUG
#include <fstream>
std::ofstream lua_opt_debug_stream("/tmp/lua_optimizer_debug");
#endif


std::wstring get_string_expression(const expression &e);

template<class container_type> PStatement StatementConverter<container_type>::operator() (const assignment &a) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting assignement" << std::endl;
#endif
	boost::shared_ptr<StatementList> ret = boost::make_shared<StatementList>();
	for (size_t i = 0; i < a.varlist.size(); i++) {
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
			ret->AddStatement(boost::make_shared<AssignmentStatement>(variable, expression));
		} catch (boost::bad_get&) {
			throw ParamConversionException(L"Only assignment to variables allowed in optimized expressions");
		}
	}
	return ret->m_statements.size() == 1 ? ret->m_statements[0] : ret;
}

template<class container_type> PStatement StatementConverter<container_type>::operator() (const block &b) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream <<  "Coverting block" << std::endl;
#endif
	return m_param_converter->ConvertBlock(b);
}

template<class container_type> PStatement StatementConverter<container_type>::operator() (const while_loop &w) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting while loop" << std::endl;
#endif
	PExpression condition = m_param_converter->ConvertExpression(w.expression_);
	PStatement block = m_param_converter->ConvertBlock(w.block_);
	return boost::make_shared<WhileStatement>(condition, block);
}

template<class container_type> PStatement StatementConverter<container_type>::operator() (const repeat_loop &r) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting while loop" << std::endl;
#endif
	PExpression condition = m_param_converter->ConvertExpression(r.expression_);
	PStatement block = m_param_converter->ConvertBlock(r.block_);
	return boost::make_shared<RepeatStatement>(condition, block);
}

template<class container_type> PStatement StatementConverter<container_type>::operator() (const if_stat &if_) {
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

template<class container_type> PStatement StatementConverter<container_type>::operator() (const for_in_loop &a) {
	throw ParamConversionException(L"For in loops not supported by converter");
}

template<class container_type> PStatement StatementConverter<container_type>::operator() (const postfixexp &a) {
	throw ParamConversionException(L"Postfix expressions as statments not supported by converter");
}

template<class container_type> PStatement StatementConverter<container_type>::operator() (const for_from_to_loop &for_) {
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

template<class container_type> PStatement StatementConverter<container_type>::operator() (const function_declaration &a) {
	throw ParamConversionException(L"Function declarations not supported by converter");
}

template<class container_type> PStatement StatementConverter<container_type>::operator() (const local_assignment &a) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting local assignment" << std::endl;
#endif
	boost::shared_ptr<StatementList> ret = boost::make_shared<StatementList>();
	for (size_t i = 0; i < a.varlist.size(); i++) {
		try {
			const identifier& identifier_ = boost::get<identifier>(a.varlist[i]);
#ifdef LUA_OPTIMIZER_DEBUG
			lua_opt_debug_stream << "Identifier: " << SC::S2A(identifier_) << std::endl;
#endif
			VarRef variable = m_param_converter->GetLocalVar(identifier_);
			PExpression expression;
			if (i < a.explist.size())
				expression = m_param_converter->ConvertExpression(a.explist[i]);
			else
				expression = boost::make_shared<NilExpression>();
			ret->AddStatement(boost::make_shared<AssignmentStatement>(variable, expression));
		} catch (boost::bad_get&) {
			throw ParamConversionException(L"Only assignment to variables allowed in optimized expressions");
		}
	}
	return ret->m_statements.size() == 1 ? ret->m_statements[0] : ret;
}

template<class container_type> PStatement StatementConverter<container_type>::operator() (const local_function_declaration &f) {
	throw ParamConversionException(L"Function declarations not supported by converter");
}

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertTerm(const term& term_) {
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

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertPow(const pow_exp& exp) {
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

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertUnOp(const unop_exp& unop) {
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

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertMul(const mul_exp& mul_) {
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

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertAdd(const add_exp& add_) {
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

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertConcat(const concat_exp &concat) {
	if (concat.size() > 1)
		throw ParamConversionException(L"Concatenation parameter cannot be used is optimized parameters");
	return ConvertAdd(concat[0]);
}

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertCmp(const cmp_exp& cmp_exp_) {
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

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertAnd(const and_exp& and_exp_) {
	PExpression p = ConvertCmp(and_exp_[0]);
	for (size_t i = 1; i < and_exp_.size(); i++) {
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Converting and binop expression" << std::endl;
#endif
		p = boost::make_shared<BinExpression<std::logical_and<Val> > >(p, ConvertCmp(and_exp_[i]));
	}
	return p;
}

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertOr(const or_exp& or_exp_) {
	PExpression p = ConvertAnd(or_exp_[0]);
	for (size_t i = 1; i < or_exp_.size(); i++) {
#ifdef LUA_OPTIMIZER_DEBUG
		lua_opt_debug_stream << "Converting or binop expression" << std::endl;
#endif
		p = boost::make_shared<BinExpression<std::logical_or<Val> > >(p, ConvertAnd(or_exp_[i]));
	}
	return p;
}

template<class container_type> PExpression ExpressionConverter<container_type>::ConvertExpression(const expression& expression_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting expression" << std::endl;
#endif
	return ConvertOr(expression_);
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const nil& nil_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting nil expression" << std::endl;
#endif
	return boost::make_shared<NilExpression>();
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const bool& bool_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting boolean value" << std::endl;
#endif
	return boost::make_shared<NumberExpression>(bool_ ? 1. : 0.);
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const double& double_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting number expression" << std::endl;
#endif
	return boost::make_shared<NumberExpression>(double_);
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const std::wstring& string) {
	throw ParamConversionException(L"String cannot appear as termns in optimized params");
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const threedots& threedots_) {
	throw ParamConversionException(L"Threedots opeartors cannot appear in optimized params");
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const funcbody& funcbody_) {
	throw ParamConversionException(L"Function bodies cannot appear in optimized expressions");
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const tableconstructor& tableconstrutor_) {
	throw ParamConversionException(L"Table construtor are not supported in optimized expressions");
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const postfixexp& postfixexp_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting postfix expression" << std::endl;
#endif
	return m_param_converter->ConvertPostfixExp(postfixexp_);
}

template<class container_type> PExpression TermConverter<container_type>::operator()(const expression& expression_) {
	return m_param_converter->ConvertExpression(expression_);
}

template<class container_type> PExpression PostfixConverter<container_type>::operator()(const identifier& identifier_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting variable expression, var: " << SC::S2A(identifier_) << std::endl;
#endif
	return boost::make_shared<VarExpression>(m_param_converter->FindVar(identifier_));
}

template<class container_type> const std::vector<expression>& PostfixConverter<container_type>::GetArgs(const std::vector<exp_ident_arg_namearg>& e) {
#ifdef LUA_OPTIMIZER_DEBUG
       lua_opt_debug_stream << "Converting args, number of exp_ident_arg_namerg :) expressions:" << e.size() << std::endl;
#endif
       if (e.size() != 1)
	       throw ParamConversionException(L"Only postfix expression in form functioname(exp, exp, ...) are allowed");
       try {
	       const namearg& namearg_ = boost::get<namearg>(e[0]);
	       const args& args_ = boost::get<args>(namearg_);
	       const std::vector<expression> &exps_
		       = boost::get<std::vector<expression> >(args_);
	       return exps_;
       } catch (boost::bad_get &e) {
	       throw ParamConversionException(L"Only postfix expression in form functioname(exp, exp, ...) are allowed");
       }
}

template<class container_type> PExpression PostfixConverter<container_type>::operator()(const boost::tuple<exp_identifier, std::vector<exp_ident_arg_namearg> > &exp) {
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

template<class container_type> PExpression InSeasonConverter<container_type>::Convert(const std::vector<expression>& expressions) {
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

	TSzarpConfig* sc = m_ipk_container->GetConfig(prefix);
	if (sc == NULL)
		throw ParamConversionException(std::wstring(L"Prefix ") + prefix + L" given in in_season not found");
	return boost::make_shared<InSeasonExpression>(sc, FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[1]));
}

template<class container_type> PExpression SzbMoveTimeConverter<container_type>::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
       lua_opt_debug_stream << "Converting szb_move_time expression" << std::endl;
#endif
       if (expressions.size() < 3)
	       throw ParamConversionException(L"szb_move_time requires three arguments");

       return boost::make_shared<SzbMoveTimeExpression>
	       (FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[0]),
		FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[1]),
		FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[2]));
}

template<class container_type> PExpression SzbRoundTimeConverter<container_type>::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
       lua_opt_debug_stream << "Converting szb_round_time expression" << std::endl;
#endif
       if (expressions.size() < 2)
	       throw ParamConversionException(L"szb_round_time requires two arguments");

       return boost::make_shared<SzbRoundTimeExpression>
	       (FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[0]),
		FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[1]));
}

template<class container_type> PExpression IsNanConverter<container_type>::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
       lua_opt_debug_stream << "Converting isnan expression"  << std::endl;
#endif
       if (expressions.size() < 1)
	       throw ParamConversionException(L"isnan takes one argument");

       return boost::make_shared<IsNanExpression>
	       (FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[0]));
}

template<class container_type> PExpression NanConverter<container_type>::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
       lua_opt_debug_stream << "Converting nan expression" << std::endl;
#endif
       return boost::make_shared<NilExpression>();
}


template<class container_type> std::wstring ParamValueConverter<container_type>::GetParamName(const expression& e) {
	try {
		return get_string_expression(e);
	} catch (const ParamConversionException& e) {
		throw ParamConversionException(L"First parameter of p function should be literal string");
	}

}

template<class container_type> PExpression ParamValueConverter<container_type>::Convert(const std::vector<expression>& expressions) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting p(..) expression" << std::endl;
#endif
	if (expressions.size() < 3)
		throw ParamConversionException(L"p function requires three arguemnts");
	std::wstring param_name = GetParamName(expressions[0]);
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Parameter name: " << SC::S2A(param_name) << std::endl;
#endif
	ParRefRef param_ref = FunctionConverter<container_type>::m_param_converter->GetParamRef(param_name);
	return boost::make_shared<ParamValue>(param_ref,
			FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[1]),
			FunctionConverter<container_type>::m_param_converter->ConvertExpression(expressions[2]));
}



template<class container_type> ParamConverterTempl<container_type>::ParamConverterTempl(container_type *ipk_container) : m_ipk_container(ipk_container) {
	m_function_converters[L"szb_move_time"] = boost::make_shared<SzbMoveTimeConverter<container_type> >(this);
	m_function_converters[L"szb_round_time"] = boost::make_shared<SzbRoundTimeConverter<container_type> >(this);
	m_function_converters[L"isnan"] = boost::make_shared<IsNanConverter<container_type > >(this);
	m_function_converters[L"nan"] = boost::make_shared<NanConverter<container_type> >(this);
	m_function_converters[L"p"] = boost::make_shared<ParamValueConverter<container_type> >(this);
	m_function_converters[L"in_season"] = boost::make_shared<InSeasonConverter<container_type> >(this, m_ipk_container);
}

template<class container_type> VarRef ParamConverterTempl<container_type>::GetGlobalVar(std::wstring identifier) {
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

template<class container_type> VarRef ParamConverterTempl<container_type>::GetLocalVar(const std::wstring& identifier) {
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

template<class container_type> VarRef ParamConverterTempl<container_type>::FindVar(const std::wstring& identifier) {
	for (std::vector<frame>::reverse_iterator i = m_vars_map.rbegin();
			i != m_vars_map.rend();
			i++) {
		std::map<std::wstring, VarRef>::iterator j = i->find(identifier);
		if (j != i->end())
			return j->second;
	}
	throw ParamConversionException(std::wstring(L"Variable ") + identifier + L" is unbound");
}

template<class container_type> ParRefRef ParamConverterTempl<container_type>::GetParamRef(const std::wstring param_name) {
	TParam* param = m_ipk_container->GetParam(param_name);
	if (param == NULL)
		throw ParamConversionException(std::wstring(L"Param ") + param_name + L" not found");

	size_t i;
	for (i = 0; i < m_param->m_par_refs.size(); i++) {
		ParamRef &ref = m_param->m_par_refs[i];
		if (ref.m_param->GetParamId() == param->GetParamId() && ref.m_param->GetConfigId() == param->GetConfigId())
			return ParRefRef(&m_param->m_par_refs, i);
	}

	ParamRef pr;
	pr.m_param = param;
	pr.m_param_index = i;
	m_param->m_par_refs.push_back(pr);

	return ParRefRef(&m_param->m_par_refs, i);
}

template<class container_type> PStatement ParamConverterTempl<container_type>::ConvertBlock(const block& block) {
	m_vars_map.push_back(frame());
	PStatement ret = ConvertChunk(block.chunk_.get());
	m_vars_map.pop_back();
	return ret;
}

template<class container_type> PExpression ParamConverterTempl<container_type>::ConvertFunction(const identifier& identifier_, const std::vector<expression>& args) {
	typename std::map<std::wstring, boost::shared_ptr<FunctionConverter<container_type> > >::iterator i = m_function_converters.find(identifier_);
	if (i == m_function_converters.end())
		throw ParamConversionException(std::wstring(L"Function ") + identifier_ + L" not supported in optimized params");
	return i->second->Convert(args);
}

template<class container_type> PStatement ParamConverterTempl<container_type>::ConvertStatement(const lua_grammar::stat& stat_) {
	StatementConverter<container_type> sc(this);
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting statement" << std::endl;
#endif
	return boost::apply_visitor(sc, stat_);
}

template<class container_type> PStatement ParamConverterTempl<container_type>::ConvertChunk(const chunk& chunk_) {
	boost::shared_ptr<StatementList> ret = boost::make_shared<StatementList>();
	for (std::vector<lua_grammar::stat>::const_iterator i = chunk_.stats.begin();
			i != chunk_.stats.end();
			i++)
		ret->AddStatement(ConvertStatement(*i));
	return ret->m_statements.size() == 1 ? ret->m_statements[0] : ret;
}

template<class container_type> PExpression ParamConverterTempl<container_type>::ConvertTerm(const term& term_) {
	TermConverter<container_type> tc(this);
	return boost::apply_visitor(tc, term_);
}

template<class container_type> PExpression ParamConverterTempl<container_type>::ConvertExpression(const expression& expression_) {
	ExpressionConverter<container_type> ec(this);
	return ec.ConvertExpression(expression_);
}

template<class container_type> PExpression ParamConverterTempl<container_type>::ConvertPostfixExp(const postfixexp& postfixexp_) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Converting postfix expression" << std::endl;
#endif
	PostfixConverter<container_type> pc(this);
	return boost::apply_visitor(pc, postfixexp_);
}

template<class container_type> void ParamConverterTempl<container_type>::ConvertParam(const chunk& chunk_, Param* param) {
#ifdef LUA_OPTIMIZER_DEBUG
	lua_opt_debug_stream << "Starting to optimize param" << std::endl;
#endif
	m_param = param;
	InitalizeVars();
	param->m_statement = ConvertChunk(chunk_);
}

template<class container_type> void ParamConverterTempl<container_type>::AddVariable(std::wstring name) {
	size_t s = m_param->m_vars.size();
	m_param->m_vars.push_back(Var(s));
	VarRef i(&m_param->m_vars, s);
	(*m_current_frame)[name] = i;
}

template<class container_type> void ParamConverterTempl<container_type>::InitalizeVars() {
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
	AddVariable(L"PT_SEC10");
	AddVariable(L"PT_SEC");
	AddVariable(L"PT_HALFSEC");
	AddVariable(L"PT_MSEC10");
}

template<class IPKContainerType> bool optimize_lua_param(TParam* param, IPKContainerType* container) {
	std::wstring param_text = SC::U2S(param->GetLuaScript());
	std::wstring::const_iterator param_text_begin = param_text.begin();
	std::wstring::const_iterator param_text_end = param_text.end();

	LuaExec::Param* exec_param = param->GetLuaExecParam();
	exec_param->m_optimized = false;	

	lua_grammar::chunk param_code;

	if (lua_grammar::parse(param_text_begin, param_text_end, param_code) && param_text_begin == param_text_end) {
		LuaExec::ParamConverterTempl<IPKContainerType> pc(container);
		try {
			pc.ConvertParam(param_code, exec_param);
			exec_param->m_optimized = true;
		} catch (LuaExec::ParamConversionException &ex) {
			sz_log(2, "Parameter %ls cannot be optimized, reason: %ls", param->GetName().c_str(), ex.what().c_str());
#ifdef LUA_OPTIMIZER_DEBUG
			lua_opt_debug_stream << "Parameter " << SC::S2A(param->GetName()) << " cannot be optimized, reason: " << SC::S2A(ex.what()) << std::endl;
#endif
		}
	} else
		sz_log(2, "Parameter %ls cannot be optimized, failed to parse its defiition", param->GetName().c_str());
	

	return exec_param->m_optimized;
}


}

#endif

