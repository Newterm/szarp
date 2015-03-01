#ifndef __SZ4_FIXED_STACK_TOP_H__
#define __SZ4_FIXED_STACK_TOP_H__
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

#include <stack>

namespace sz4 {

class fixed_stack_top {
	fixed_stack_type& m_stack;
public:
	fixed_stack_top(fixed_stack_type& stack)
			: m_stack(stack) {
		m_stack.push_back(true);
	}

	void _and(bool _and) {
		m_stack.back() = m_stack.back() & _and;
	}

	bool top() const {
		return m_stack.back();
	}

	~fixed_stack_top() {
		m_stack.pop_back();
	}
};

}

#endif
