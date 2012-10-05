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

void generic_param_entry::register_at_monitor(SzbParamMonitor* monitor) {
	monitor->add_observer(this, m_param, m_param_dir.external_file_string(), 0);
}

void generic_param_entry::deregister_from_monitor(SzbParamMonitor* monitor) {
	monitor->remove_observer(this);
}

void generic_param_entry::param_data_changed(TParam*, const std::string& path) {
	boost::mutex::scoped_lock lock(m_mutex);
	m_paths_to_update.push_back(path);
}

void buffer::remove_param(TParam* param) {
	if (m_param_ents.size() <= param->GetParamId())
		return;

	generic_param_entry* entry = m_param_ents[param->GetParamId()];
	if (!entry)
		return;

	entry->deregister_from_monitor(m_param_monitor);
	delete entry;

	m_param_ents[param->GetParamId()] = NULL;
}

template<class data_type, class time_type> generic_param_entry* param_entry_build(TParam* param, const boost::filesystem::wpath &buffer_directory) {
	return new param_entry_in_buffer<data_type, time_type>(param, buffer_directory);
}

template<class data_type> generic_param_entry* param_entry_build(TParam::TimeType type_time, TParam* param, const boost::filesystem::wpath &buffer_directory) {
	switch (type_time) {
		case TParam::SECOND:
			return param_entry_build<date_type, second_time_t>(param, buffer_directory);
		case TParam::NANOSECOND:
			return param_entry_build<date_type, nanosecond_time_t>(param, buffer_directory);
	}
}

generic_param_entry* param_entry_build(TParam::DataType data_type, TParam::TimeType type_time, TParam* param, const boost::filesystem::wpath &buffer_directory) {
	switch (data_type) {
		case TParam::SHORT:
			return param_entry_build<short>(time_type, param, buffer_directory);
		case TParam::INT:
			return param_entry_build<int>(time_type, param, buffer_directory);
		case TParam::FLOAT:
			return param_entry_build<float>(time_type, param, buffer_directory);
		case TParam::DOUBLE:
			return param_entry_build<double>(time_type, param, buffer_directory);
	}
}

generic_param_entry* buffer::create_param_entry(TParam* param) {
	return param_entry_build(param->GetDataType(), param->GetTimeType(), param, m_buffer_directory);
}


}
