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

#include "sz4/base.h"
#include "sz4/real_param_entry.h"
#include "sz4/real_param_entry_templ.h"

namespace sz4 
{

template class file_block<int16_t, sz4::second_time_t, base>;
template class file_block<double, sz4::second_time_t, base>;
template class file_block<float, sz4::second_time_t, base>;
template class file_block<int32_t, sz4::second_time_t, base>;
template class file_block<uint32_t, sz4::second_time_t, base>;
template class file_block<uint16_t, sz4::second_time_t, base>;
template class file_block<int16_t, sz4::nanosecond_time_t, base>;
template class file_block<double, sz4::nanosecond_time_t, base>;
template class file_block<float, sz4::nanosecond_time_t, base>;
template class file_block<int32_t, sz4::nanosecond_time_t, base>;
template class file_block<uint32_t, sz4::nanosecond_time_t, base>;
template class file_block<uint16_t, sz4::nanosecond_time_t, base>;

template class file_block_entry<int16_t, sz4::second_time_t, base>;
template class file_block_entry<double, sz4::second_time_t, base>;
template class file_block_entry<float, sz4::second_time_t, base>;
template class file_block_entry<int32_t, sz4::second_time_t, base>;
template class file_block_entry<uint32_t, sz4::second_time_t, base>;
template class file_block_entry<uint16_t, sz4::second_time_t, base>;
template class file_block_entry<int16_t, sz4::nanosecond_time_t, base>;
template class file_block_entry<double, sz4::nanosecond_time_t, base>;
template class file_block_entry<float, sz4::nanosecond_time_t, base>;
template class file_block_entry<int32_t, sz4::nanosecond_time_t, base>;
template class file_block_entry<uint32_t, sz4::nanosecond_time_t, base>;
template class file_block_entry<uint16_t, sz4::nanosecond_time_t, base>;

template class sz4_file_block_entry<int16_t, sz4::second_time_t, base>;
template class sz4_file_block_entry<double, sz4::second_time_t, base>;
template class sz4_file_block_entry<float, sz4::second_time_t, base>;
template class sz4_file_block_entry<int32_t, sz4::second_time_t, base>;
template class sz4_file_block_entry<uint32_t, sz4::second_time_t, base>;
template class sz4_file_block_entry<uint16_t, sz4::second_time_t, base>;
template class sz4_file_block_entry<int16_t, sz4::nanosecond_time_t, base>;
template class sz4_file_block_entry<double, sz4::nanosecond_time_t, base>;
template class sz4_file_block_entry<float, sz4::nanosecond_time_t, base>;
template class sz4_file_block_entry<int32_t, sz4::nanosecond_time_t, base>;
template class sz4_file_block_entry<uint32_t, sz4::nanosecond_time_t, base>;
template class sz4_file_block_entry<uint16_t, sz4::nanosecond_time_t, base>;

template class szbase_file_block_entry<int16_t, sz4::second_time_t, base>;
template class szbase_file_block_entry<double, sz4::second_time_t, base>;
template class szbase_file_block_entry<float, sz4::second_time_t, base>;
template class szbase_file_block_entry<int32_t, sz4::second_time_t, base>;
template class szbase_file_block_entry<int16_t, sz4::nanosecond_time_t, base>;
template class szbase_file_block_entry<double, sz4::nanosecond_time_t, base>;
template class szbase_file_block_entry<float, sz4::nanosecond_time_t, base>;
template class szbase_file_block_entry<int32_t, sz4::nanosecond_time_t, base>;

template class real_param_entry_in_buffer<int16_t, sz4::second_time_t, base>;
template class real_param_entry_in_buffer<double, sz4::second_time_t, base>;
template class real_param_entry_in_buffer<float, sz4::second_time_t, base>;
template class real_param_entry_in_buffer<int32_t, sz4::second_time_t, base>;
template class real_param_entry_in_buffer<int16_t, sz4::nanosecond_time_t, base>;
template class real_param_entry_in_buffer<double, sz4::nanosecond_time_t, base>;
template class real_param_entry_in_buffer<float, sz4::nanosecond_time_t, base>;
template class real_param_entry_in_buffer<int32_t, sz4::nanosecond_time_t, base>;

}
