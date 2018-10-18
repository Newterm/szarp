#include "integrator.h"
#include <cassert>
#include <limits>

Integrator::CacheEntry::CacheEntry()
: start_time(0), end_time(0), result(0), last_value(0), last_data_time(0)
{}

Integrator::CacheEntry::CacheEntry(const time_t start_time, const time_t end_time, const double result, const double last_value, const time_t last_data_time)
: start_time(start_time), end_time(end_time), result(result), last_value(last_value), last_data_time(last_data_time)
{}

void Integrator::CacheEntry::decreasePriority() {
	if (priority > std::numeric_limits<decltype(priority)>::lowest()) {
		priority--;
	}
}

void Integrator::CacheEntry::increasePriority() {
	if (priority < 0) {
		priority++;
	}
}

unsigned int Integrator::ParamCache::MAX_ENTRIES_PER_PARAM = 5;

void Integrator::ParamCache::setMaxEntriesPerParam(const int entries) {
	MAX_ENTRIES_PER_PARAM = entries;
}

void Integrator::setMaxEntriesPerParam(const int entries) {
	ParamCache::setMaxEntriesPerParam(entries);
}

bool Integrator::ParamCache::hasEntry(time_t start_time) {
	return entries.find(start_time) != entries.end();
}

Integrator::CacheEntry Integrator::ParamCache::getEntry(time_t start_time) {
	for (auto& time_entry: entries) {
		time_entry.second.decreasePriority();
	}
	CacheEntry& entry = entries.at(start_time);
	entry.increasePriority();
	return entry;
}

void Integrator::ParamCache::addEntry(Integrator::CacheEntry&& entry) {
	// case (1): entry exists - update
	auto existing_entry = entries.find(entry.start_time);
	if (existing_entry != entries.end()) {
		existing_entry->second = entry;
		return;
	}

	// case (2): max entries was not reached - append
	if (entries.size() < MAX_ENTRIES_PER_PARAM) {
		entries[entry.start_time] = entry;
		return;
	}

	// case (3): remove least important entry, then add
	int min_priority = std::numeric_limits<decltype(entry.priority)>::max();
	time_t entry_to_remove{};
	for (const auto& time_entry : entries) {
		if (time_entry.second.priority < min_priority) {
			min_priority = time_entry.second.priority;
			entry_to_remove = time_entry.first;
		}
	}
	entries.erase(entry_to_remove);
	entries[entry.start_time] = entry;
}

Integrator::Integrator(DataProvider get_value, TimeMover move_time, IsNoData is_no_data, const double no_data)
: get_value(get_value), move_time(move_time), is_no_data(is_no_data), m_no_data(no_data), m_cache(m_internal_cache)
{}

Integrator::Integrator(DataProvider get_value, TimeMover move_time, IsNoData is_no_data, const double no_data, Cache& cache)
: get_value(get_value), move_time(move_time), is_no_data(is_no_data), m_no_data(no_data), m_cache(cache)
{}

double Integrator::GetIntegral(const std::string& param_name, const time_t start_time, const time_t end_time) {

	if (start_time > end_time) {
		return m_no_data;
	}
	double result = 0;
	time_t curr_time = 0;
	time_t last_data_time = 0;
	double curr_value = 0;

	bool basing_on_cache = false;

	{
		auto param_cache = m_cache.find(param_name);
		if (param_cache != m_cache.end()) {
			if (param_cache->second.hasEntry(start_time)) {
				const auto& entry = param_cache->second.getEntry(start_time);
				if (entry.end_time == end_time) {
					return entry.result;
				}
				if (entry.end_time < end_time) {
					result = entry.result;
					last_data_time = entry.last_data_time;	// new data may have arrived after the nodata points and probably we can use interpolation
					curr_time = entry.end_time;
					curr_value = entry.last_value;
					basing_on_cache = true;
				}
			}
		}
	}

	if (!basing_on_cache) {
		const auto first = GetNextValidValue(param_name, start_time, end_time);
		if (first.first == INVALID_TIME) {
			return m_no_data;
		}

		curr_time = first.first;
		last_data_time = curr_time;
		curr_value = first.second;
		assert(first.first >= start_time);
	}
	while (curr_time < end_time) {
		// seek first non-nodata value, if not found before end_time, then use curr_value
		const auto next = GetNextValidValue(param_name, move_time(curr_time, 1), end_time);

		// if INVALID_TIME finish the loop (we don't extrapolate on no_data)
		if (next.first == INVALID_TIME) {
			break;
		}
		const time_t next_time = next.first;
		const double next_value = next.second;

		result += (curr_value + next_value) / 2 * (next_time - last_data_time);

		curr_value = next_value;
		curr_time = next_time;
		last_data_time = curr_time;
	}

	m_cache[param_name].addEntry(CacheEntry{start_time, end_time, result, curr_value, last_data_time});
	return result;
}

std::pair<time_t, double> Integrator::GetNextValidValue(const std::string& param_name,
		time_t start_time, time_t end_time) {

	time_t data_time = INVALID_TIME;
	time_t curr_time = start_time;
	double value = 0;
	while (curr_time <= end_time) {
		value = get_value(param_name, curr_time);
		if (!is_no_data(value)) {
			data_time = curr_time;
			break;
		}
		curr_time = move_time(curr_time, 1);
	}
	return std::make_pair(data_time, value);
}
