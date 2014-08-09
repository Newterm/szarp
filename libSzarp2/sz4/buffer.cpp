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

#include <tr1/functional>

#include "szarp_config.h"
#include "szarp_base_common/lua_param_optimizer.h"
#include "sz4/defs.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"
#include "sz4/base.h"
#include "sz4/buffer.h"
#include "sz4/util.h"
#include "sz4/real_param_entry.h"
#include "sz4/lua_optimized_param_entry.h"
#include "sz4/lua_param_entry.h"
#include "sz4/rpn_param_entry.h"

namespace sz4 {

void generic_param_entry::param_data_changed(TParam* param, const std::string& path) {
	handle_param_data_changed(param, path);
	boost::lock_guard<boost::recursive_mutex> lock(m_lock);
	std::for_each(m_referring_params.begin(), m_referring_params.end(),
		std::tr1::bind(&generic_param_entry::param_data_changed, std::tr1::placeholders::_1, param, path));
	std::for_each(m_observers.begin(), m_observers.end(),
		std::tr1::bind(&param_observer::param_data_changed, std::tr1::placeholders::_1, param));
}

void generic_param_entry::add_refferring_param(generic_param_entry* param_entry) {
	boost::lock_guard<boost::recursive_mutex> lock(m_lock);
	m_referring_params.push_back(param_entry);
}

void generic_param_entry::remove_refferring_param(generic_param_entry* param_entry) {
	boost::lock_guard<boost::recursive_mutex> lock(m_lock);
	std::list<generic_param_entry*>::iterator i = std::find(m_referring_params.begin(), m_referring_params.end(), param_entry);
	if (i != m_referring_params.end())
		m_referring_params.erase(i);
}

void generic_param_entry::add_refferred_param(generic_param_entry *param_entry) {
	boost::lock_guard<boost::recursive_mutex> lock(m_lock);
	m_referred_params.push_back(param_entry);
}

void generic_param_entry::refferred_param_removed(generic_param_entry* param_entry) {
	boost::lock_guard<boost::recursive_mutex> lock(m_lock);
	std::list<generic_param_entry*>::iterator i = std::find(m_referred_params.begin(), m_referred_params.end(), param_entry);
	if (i != m_referred_params.end())
		m_referred_params.erase(i);
}

void generic_param_entry::register_observer(param_observer *observer) {
	boost::lock_guard<boost::recursive_mutex> lock(m_lock);
	m_observers.push_back(observer);
}

void generic_param_entry::deregister_observer(param_observer *observer) {
	boost::lock_guard<boost::recursive_mutex> lock(m_lock);
	std::list<param_observer*>::iterator i = std::find(m_observers.begin(), m_observers.end(), observer);
	if (i != m_observers.end())
		m_observers.erase(i);
}

generic_param_entry::~generic_param_entry() {
	std::for_each(m_referring_params.begin(), m_referring_params.end(), std::bind2nd(std::mem_fun(&generic_param_entry::refferred_param_removed), this));
	std::for_each(m_referred_params.begin(), m_referred_params.end(), std::bind2nd(std::mem_fun(&generic_param_entry::remove_refferring_param), this));
}

}

#include "sz4/buffer_templ.h"

namespace sz4 {

template class buffer_templ<base_types>;

}
