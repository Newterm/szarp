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

#ifndef __SUMATOR_H__
#define __SUMATOR_H__

#include <vector>
#include <assert.h>

/** Simple fixed-size (cyclic) buffer calculating sum and average of its elements.
 * It can contain also non-data (empty) elements.  */
template <typename T> class TSumator {
	public:
		/** Class for buffer elements. */
		class TSumatorField {
			public:
				TSumatorField() 
					: data(0), is_data(false)
				{
				}
				TSumatorField(T _data, bool _is_data = true)
					: data(_data), is_data(_is_data)
				{
				}
				T data;	/**< element value, meaningfull only is is_data is true */
				bool is_data;	/**< true if element contains real data, false otherwise */
		};
		/**
		 * @param n size of sumator
		 * @param _min minimal number of data elements required to count sum and average values
		 */
		TSumator(size_t n, size_t _min = 1)
			: length(n), pointer(0), count(0), sum(0), min_count(_min)
		{
			fields.reserve(length);
		}
		/**
		 * Set minimal number of data elements required to count sum and average values
		 * @param _min minimal required elements count
		 */
		void SetMinCount(size_t _min);
		/**
		 * @return number of no no-data elements in buffer
		 */
		size_t GetCount();
		/**
		 * Add new element to buffer.
		 * @param new_data new element to add to buffer, if buffer is empty, it replaces 
		 * the oldest one
		 */
		void Push(TSumatorField new_data);
		/**
		 * Add new no-data (empty) element to buffer
		 */
		void Push();
		/**
		 * Add new element with given value
		 * @param value value of element to add to buffer
		 */
		void Push(T value);
		/**
		 * Return sum of elements in buffer. 
		 * @return sum of elements in buffer; you should check with @see HasData() if buffer really 
		 * contains enough elements
		 */
		T GetSum();
		/**
		 * Return average of all no no-data values in buffer.
		 * @return average values; you MUST check before if @see HasData() return true 
		 */
		T GetAvg();
		/**
		 * Check if buffer contains enough elements to calculate sum and average values.
		 * @return true if buffer contains enough elements, false otherwise
		 */
		bool HasData();
	protected:
		/** Move pointer to next buffer element */
		inline void IncrPointer();
		size_t length;		/**< length of buffer */
		std::vector<TSumatorField> fields;
					/**< vector of buffer elements */
		size_t pointer;		/**< current cycle buffer position */
		size_t count;		/**< current count of no no-data elements */
		T sum;		/**< sum of all no no-data elements */
		size_t min_count;	/**< required amount of no no-data elements */
};

template <typename T> 
void TSumator<T>::SetMinCount(size_t _min)
{
	assert (_min <= length);
	assert (_min > 0);
	min_count = _min;
}

template <typename T> 
size_t TSumator<T>::GetCount()
{
	return count;
}

template <typename T> 
void TSumator<T>::Push(TSumatorField new_data)
{
	if (fields[pointer].is_data) {
		count--;
		sum -= fields[pointer].data;
	}
	fields[pointer] = new_data;
	if (new_data.is_data) {
		count++;
		sum += new_data.data;
	}
	IncrPointer();
}

template <typename T> 
void TSumator<T>::Push()
{
	Push(TSumatorField());
}

template <typename T> 
void TSumator<T>::Push(T value)
{
	Push(TSumatorField(value));
}

template <typename T> 
T TSumator<T>::GetSum()
{
	return sum;
}

template <typename T> 
T TSumator<T>::GetAvg()
{
	assert (count > min_count);
	return sum / count;
}

template <typename T> 
bool TSumator<T>::HasData()
{
	return (count >= min_count);
}

template <typename T> 
void TSumator<T>::IncrPointer()
{
	pointer = (pointer + 1) % length;
}

#endif /** __SUMATOR_H__ */
