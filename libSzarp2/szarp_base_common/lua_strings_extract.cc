
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

#include <algorithm>
#include <vector>

#include <boost/variant.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>

#include "szarp_base_common/lua_syntax.h"
#include "szarp_base_common/lua_syntax_fusion_adapt.h"

struct extract_ {
	template<class T> std::vector<std::wstring> operator()(const T& x) const;
};

struct variant_visitor : public boost::static_visitor<std::vector<std::wstring> > {
	template<class T> std::vector<std::wstring> operator()(const T& x) const {
		return extract_()(x);
	}
};

struct extract_variant {
	template <class T> std::vector<std::wstring> operator()(const T &x) const {
		return boost::apply_visitor(variant_visitor(), x);
	}
};

template<class F, class V> struct acc {
	F _f;	
	mutable std::vector<V> _v;
	template <class	T> void operator()(const T& x) const {
		std::vector<V> v = _f(x);
		_v.insert(_v.end(), v.begin(), v.end());
	}
};

struct extract_seq {
	template <class T> std::vector<std::wstring> operator()(const T &x) const {
		acc<extract_, std::wstring> _acc;
		boost::fusion::for_each(x, _acc);
		return _acc._v;	
	}
};

struct extract_vec {
	template <class T> std::vector<std::wstring> operator()(const std::vector<T> &x) const {
		return std::for_each(x.begin(), x.end(), acc<extract_, std::wstring>())._v;
	}
};

struct extract_string {
	template <class T> std::vector<std::wstring> operator()(const T &x) const {
		return std::vector<std::wstring>(1, x);
	}
};

struct extract_optional {
	template <class T> std::vector<std::wstring> operator()(const T &x) const {
		if (x)
			return extract_()(*x);
		else
			return std::vector<std::wstring>();
	}
};

struct extract_recursive_wrapper {
	template <class T> std::vector<std::wstring> operator()(const boost::recursive_wrapper<T> &x) const {
		return extract_()(x.get());
	}
};

struct extract_none {
	template <class T> std::vector<std::wstring> operator()(const T &x) const {
		return std::vector<std::wstring>();
	}
};

//from http://lists.boost.org/Archives/boost/2004/02/60473.php
template<typename T> struct is_variant : boost::mpl::false_ {};
template<BOOST_VARIANT_ENUM_PARAMS(typename T)>
	struct is_variant< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
		: boost::mpl::true_ {}; 

template<typename T> struct is_vector : boost::mpl::false_ {};
template<typename T>
	struct is_vector< std::vector<T> >
		: boost::mpl::true_ {}; 

template<typename T> struct is_optional: boost::mpl::false_ {};
template<typename T>
	struct is_optional< boost::optional<T> >
		: boost::mpl::true_ {}; 

template<typename T> struct is_recursive_wrapper: boost::mpl::false_ {};
template<typename T>
	struct is_recursive_wrapper < boost::recursive_wrapper<T> >
		: boost::mpl::true_ {}; 

template<class T> struct extract_impl :
		boost::mpl::eval_if<boost::fusion::traits::is_sequence<T>,
			boost::mpl::identity<extract_seq>, 
			boost::mpl::eval_if<is_variant<T>,
				boost::mpl::identity<extract_variant>,
				boost::mpl::eval_if<is_vector<T>,
					boost::mpl::identity<extract_vec>,
					boost::mpl::eval_if<boost::is_same<T, std::wstring>,
						boost::mpl::identity<extract_string>,
						boost::mpl::eval_if<is_optional<T>,
							boost::mpl::identity<extract_optional>,
							boost::mpl::eval_if<is_recursive_wrapper<T>,
								boost::mpl::identity<extract_recursive_wrapper>,
								boost::mpl::identity<extract_none>
							>
						>
					>
				>
			>
		>::type
{};

template<class T> std::vector<std::wstring> extract_::operator()(const T& x) const {
	return extract_impl<T>()(x);
}

bool extract_strings_from_formula(const std::wstring& formula, std::vector<std::wstring> &strings) {
	lua_grammar::chunk c;
	std::wstring::const_iterator s = formula.begin();
	std::wstring::const_iterator e = formula.end();
	if (lua_grammar::parse(s, e, c)) {
		strings = extract_()(c);
		return true;
	} else 
		return false; 
}

#endif /* ifdef LUA_PARAM_OPTIMISE */

