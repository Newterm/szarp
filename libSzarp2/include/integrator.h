#ifndef __INTEGRATOR_H_
#define __INTEGRATOR_H_

#include <ctime>
#include <functional>
#include <map>

/**
 * Calculates integrals. Interpolates between valid (not non-data) points.
 * Never extrapolates. (No-data points outside valid interval don't affect
 * result at all).
 */
class Integrator {
private:
	struct CacheEntry {
		time_t start_time;
		time_t end_time;
		double result;
		double last_value;
		time_t last_data_time;	// <= end_time
		int priority{};

		void decreasePriority();
		void increasePriority();
	};

	class ParamCache {
	public:
		static void setMaxEntriesPerParam(const int entries);

		bool hasEntry(time_t start_time);
		CacheEntry getEntry(time_t start_time);
		void addEntry(CacheEntry&& entry);

	private:
		static unsigned int MAX_ENTRIES_PER_PARAM;
		std::map<time_t, CacheEntry> entries;
	};

public:

	using Cache = std::map<std::string, ParamCache>;
	using DataProvider = std::function<double(const std::string& param_name, time_t probe_time)>;
	using TimeMover = std::function<time_t(time_t start_time, int steps)>;
	using IsNoData = std::function<bool(double value)>;

	static void setMaxEntriesPerParam(const int entries);

	Integrator(DataProvider get_value, TimeMover move_time, IsNoData is_no_data, const double no_data);
	Integrator(DataProvider get_value, TimeMover move_time, IsNoData is_no_data, const double no_data, Cache& cache);

	double GetIntegral(const std::string& param_name, const time_t start_time, const time_t end_time);

private:
	static const time_t INVALID_TIME = 0;

	std::pair<time_t, double> GetNextValidValue(const std::string& param_name,
		time_t start_time, time_t end_time);

	const DataProvider get_value;
	const TimeMover move_time;
	const IsNoData is_no_data;
	const double m_no_data;

	Cache& m_cache;
	Cache m_internal_cache;	// used only when external cache is not provided
};

#endif // __INTEGRATOR_H__
