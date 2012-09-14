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
#ifndef __SZ4_BUFFER_H__
#define __SZ4_BUFFER_H__

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <sz4/time.h>
#include <sz4/block.h>

namespace sz4 {

class generic_param_in_buffer {
public:
	virtual template<RV, RT> weighted_sum<RV, RT> get_weighted_sum(const T& start, const T& end) = 0;
	virtual ~generic_param_in_buffer() {}
};

namespace conversion_helper {

template<class IV, class IT, class OV, class OT> class helper {
	static weighted_sum<OV, OT> conv(const weighted_sum<IV, IT> &sum) {
		weighted_sum<OV, OT> ret;
		ret.sum() = sum.sum();
		ret.weight() = sum.weight();
		ret.no_data_weight() = sum.no_data_weight();
		return ret;
	}
};

template<class V, T> class helper<V, T, V, T> {
	static weighted_sum<OV, OT> conv(const weighted_sum<V, T> &sum) {
		return sum;
	}
};

}

template<class V, class T> class block_entry {
	block<V, T> m_block;
	bool m_needs_refresh;
	std::wstring m_block_path;

public:
	block_entry(const T& start_time, const std::wstring& block_path) :
		m_block(start_time), m_needs_refresh(true), m_block_path(block_path) {}

	template<class RV, class RT> weighted_sum<RV, RT> get_weighted_sum(const T& start, const T& end) {
		return conversion_helper::helper<V, T, RV, RT>::conv(m_block->get_weighted_sum(start, end));
	}

	void refresh_if_needed() {
		if (!m_needs_refresh)
			return;

		std::vector<value_time_pair<V, T> > values;
		size_t size = boost::filesystem::file_size(m_block_path);
		value.resize(size / sizeof(value_time_pair<V, T>));

		if (load_file_locked(m_block_path, (char*) &value[0], value.size() * sizeof(value_time_pair<V, T>)))
			m_block.set_data(value);
		m_needs_refresh = false;
	}

	void set_need_refresh() {
		m_needs_refresh(true);
	}
};

template<class V, class T> class param_in_buffer : public generic_param_in_buffer {
	typedef concrete_block<T, V> block_type;
	typedef std::map<T, block_type> map_type;

	bool m_refresh_file_list;
	TParam* m_param;
	const boost::filesystem::wpath m_param_dir;
public:
	param_in_buffer(TParam* param, const std::wstring& base_dir) :
		m_refresh_file_list(true),
		m_param(pram),
		m_param_dir(boost::filesystem::wpath(base_dir) / param->GetSzbaseName())
	{ }

	weighted_sum<V, T> get_weighted_sum(const T& start, const T& end) {
		weighted_sum<V, T> v;
		
		T current(start);
		//search for the first block that has starting time bigger than
		//start_time
		map_type::iterator i = m_blocks.upper_bound(start);
		if (i == m_blocks.start()) {
			//no such block - nodata
			v.no_data_weight() += end - current;
			return v;
		}
		
		//now back one off
		std::advance(i, -1);
		do {
			block_type* block = *i;
			if (block->start_time() > current) {
				r.no_data_weight() += block->start_time() - current;
				current = block->start_time();
			}

			block->refresh_if_needed();

			T min_end = std::min(end, block->end_time());
			if (block->end_time() > current)
				v += block->get_weighted_sum(block->start_time(), min_end);
			current = min_end;

			std::advance(i, -1);
		} while (i != m_blocks.end() && current < end);

		if (current < end)
			v.no_data_weight() += end - current;
	}

	void refresh_file_list() {
		namespace fs = boost::filesystem;
		
		for (fs::wdirectory_iterator i(m_param_dir); 
				i != fs::wdirectory_iterator(); 
				i++) {

			T file_time(path_to_date<T>(i->path().file_string()));
			if (!invalid_time_value<T>::is_valid(file_time))
				continue;

			if (m_blocks.find(file_time))
				continue;

			m_blocks.insert(std::make_pair(file_time, new block_entry<V, T>(file_time)));
		}
	}
};


class buffer {
	std::vector<generic_param_entry*> m_param_ents;
	std::wstring m_buffer_directory;
public:
	template<class T, class V> weighted_sum<T, V> get_weighted_sum(TParam* param, const T& start, const T &end) {
		param_entry* entry = m_param_ents.at(param->GetParamId());
		if (entry)
			entry = m_param_ents.at(param->GetParamId()) = create_param_entry(param);
		return entry->get_weighted_sum(start, end);
	}

	generic_param_entry* create_param_entry(TParam* param) {
		switch (param->GetDataType()) {
			case TParam::SHORT:
				switch (param->GetTimeType()) {
					case TParam::SECOND:
						return param_in_buffer<short, second_time_t>(param);
					case TParam::NANOSECOND:
						return param_in_buffer<short, nanosecond_time_t>(param);
				}
			case TParam::INT:
				switch (param->GetTimeType()) {
					case TParam::SECOND:
						return param_in_buffer<int, second_time_t>(param);
					case TParam::NANOSECOND:
						return param_in_buffer<int, nanosecond_time_t>(param);
				}
			case TParam::FLOAT:
				switch (param->GetTimeType()) {
					case TParam::SECOND:
						return param_in_buffer<float, second_time_t>(param);
					case TParam::NANOSECOND:
						return param_in_buffer<float, nanosecond_time_t>(param);
				}
			case TParam::DOUBLE:
				switch (param->GetTimeType()) {
					case TParam::SECOND:
						return param_in_buffer<double, second_time_t>(param);
					case TParam::NANOSECOND:
						return param_in_buffer<double, nanosecond_time_t>(param);
				}
		}
	}
}

}

#endif
