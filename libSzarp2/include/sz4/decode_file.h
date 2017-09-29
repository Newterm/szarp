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
#ifndef __SZ4_DECODE_FILE_H__
#define __SZ4_DECODE_FILE_H__

namespace sz4 {

size_t decode_delta(long long& delta, const unsigned char* buffer, size_t size);

template<class T> size_t decode_time(T& time, const unsigned char* buffer, size_t size) {
	long long delta;
	size_t count = decode_delta(delta, buffer, size);
	time += delta;
	return count;
}

template<class V, class T> std::vector<value_time_pair<V, T> >
decode_file(const unsigned char *buffer, size_t size, T time) {
	size_t index = 0;
	std::vector<value_time_pair<V, T> > result;
	result.reserve(size / sizeof(V) / sizeof(T));

	while (index + sizeof(V) <= size) {
		value_time_pair<V, T> pair;

		memcpy(&pair.value, buffer + index, sizeof(V));
		index += sizeof(V);

		if (index == size)
			break;

		index += decode_time(time, buffer + index, size - index);
		pair.time = time;

		result.push_back(pair);
	}

	return result;
}

}

#endif

