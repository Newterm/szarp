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

#ifndef __LUA_SYNTAX_FUSION_ADAPT_H__
#define __LUA_SYNTAX_FUSION_ADAPT_H__

#include "config.h"




#ifdef LUA_PARAM_OPTIMISE

BOOST_FUSION_ADAPT_STRUCT(
	lua_grammar::namelist,
	(std::vector<std::wstring>, namelist_)
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


#endif //LUA_PARAM_OPTIMISE

#endif
