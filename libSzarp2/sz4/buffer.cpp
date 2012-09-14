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

#include "szarp_config.h"
#include "sz4/buffer.h"

namespace sz4 {

void buffer::param_data_changed(TParam* param, const std::string& path) {
	boost::mutex::scoped_lock lock(m_mutex);
	if (param->GetParamId() >= m_param_ents_to_update.size())
		m_param_ents_to_update.resize(param->GetParamId() + 1);
	m_param_ents_to_update[param->GetParamId()].push_back(path);
}

generic_param_entry* buffer::create_param_entry(TParam* param) {
	switch (param->GetDataType()) {
		case TParam::SHORT:
			switch (param->GetTimeType()) {
				case TParam::SECOND:
					return new param_entry_in_buffer<short, second_time_t>(param, m_buffer_directory);
				case TParam::NANOSECOND:
					return new param_entry_in_buffer<short, nanosecond_time_t>(param, m_buffer_directory);
			}
		case TParam::INT:
			switch (param->GetTimeType()) {
				case TParam::SECOND:
					return new param_entry_in_buffer<int, second_time_t>(param, m_buffer_directory);
				case TParam::NANOSECOND:
					return new param_entry_in_buffer<int, nanosecond_time_t>(param, m_buffer_directory);
			}
		case TParam::FLOAT:
			switch (param->GetTimeType()) {
				case TParam::SECOND:
					return new param_entry_in_buffer<float, second_time_t>(param, m_buffer_directory);
				case TParam::NANOSECOND:
					return new param_entry_in_buffer<float, nanosecond_time_t>(param, m_buffer_directory);
			}
		case TParam::DOUBLE:
			switch (param->GetTimeType()) {
				case TParam::SECOND:
					return new param_entry_in_buffer<double, second_time_t>(param, m_buffer_directory);
				case TParam::NANOSECOND:
					return new param_entry_in_buffer<double, nanosecond_time_t>(param, m_buffer_directory);
			}
	}
	/*NOT REACHED*/
	return NULL;
}


}
