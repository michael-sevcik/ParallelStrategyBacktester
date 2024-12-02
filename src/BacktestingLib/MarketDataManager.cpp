module;
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include <forward_list>
#include <utility>
#include <span>
#include <vector>

export module MarketDataManager;

import AlgoTrading;
using namespace std;

/**
 * @brief Calculates bars from ticks.
 * @param timeframe the timeframe of the bars
 * @param ticks the ticks to calculate bars from
 * @return derived bars
 */
export Bars calculateBars(Timeframe timeframe, const Ticks& ticks) {
	Bars bars;
	if (ticks.empty()) {
		return bars;
	}

	std::chrono::milliseconds tf_duration = timeframe_durations[(int)timeframe];
	Bar bar;
	bar.openBar(ticks.front());
	for (size_t i = 1; i < ticks.size(); ++i) {
		const auto& tick = ticks[i];
		if(tick.timestamp - bar.open_timestamp >= tf_duration) {
			bars.push_back(bar);
			bar.openBar(tick);
		}
		else {
			bar.addTick(tick);
		}
	}

	bars.push_back(bar);
	return bars;
}

/**
 * @brief Creates a view of a vector between given indexes.
 * @tparam T Type of the vector elements
 * @param data vector from which to create the view
 * @param start inclusive start index - first element of the view.
 * @param end exclusive end index
 * @return vector view.
 */
template <typename T>
std::span<const T> vector_slice(const std::vector<T>& data, int start, int end) {
	if (start < 0 || start >= data.size() || end < 0 || end > data.size() || start > end) {
		throw std::out_of_range("Invalid indices");
	}

	return std::span<const T>(data.data() + start, end - start);
}


/**
 * @brief Manages market data.
 */
export class MarketDataManager {
public:
	/**
	* @brief constructs instance of MarketDataManager.
	* @param ticks the ticks from which to calculate bars.
	*/
	MarketDataManager(const Ticks& ticks) : _ticks(&ticks) {
		_first_tick_time = ticks.front().timestamp;
		_last_tick_time = ticks.back().timestamp;
	}

	/**
	 * @brief Gets last bars of given timeframe before specified time
	 * @param time_frame specifies the timeframe of the bars
	 * @param before specifies the time before which the bars should be returned
	 * @param count_of_bars how many bars to return
	 * @param bars output parameter - the bars view will be stored here
	 * @return True if the bars were found, false otherwise - not enough data.
	 */
	bool getLastBarsBefore(Timeframe time_frame, TimePoint before, size_t count_of_bars, BarsView& bars) {
		if (before <= _first_tick_time || before > _last_tick_time) {
			return false;
		}
		
		Bars* bars_ptr = nullptr;
		if (!try_get_existing_bars(time_frame, bars_ptr)) {
			if (!create_bars(time_frame, bars_ptr)) {
				return false;
			}
		}

		// Find the last bar before the specified time
		// Create appropriate view of the bars
		int last_bar_index = find_index_of_bar_before(*bars_ptr, before);
		int first_bar_index = last_bar_index - count_of_bars + 1;
		if (first_bar_index < 0) {
			return false;
		}
		
		bars = vector_slice(*bars_ptr, first_bar_index, last_bar_index + 1);
		return true;
	}
private:
	const Ticks* _ticks;
	TimePoint _first_tick_time;
	TimePoint _last_tick_time;
	unordered_map<Timeframe, Bars*> _bars_by_timeframe;
	forward_list<Bars> _bars;
	mutable shared_mutex _mutex;

	bool try_get_existing_bars(Timeframe timeframe, Bars*& bars_ptr) const {
		shared_lock lock(_mutex);
		auto it = _bars_by_timeframe.find(timeframe);
		if (it == _bars_by_timeframe.end()) {
			return false;
		}

		bars_ptr = it->second;
		return true;
	}

	int find_index_of_bar_before(const Bars& bars, TimePoint tp) {
		size_t i = 0;
		for (; i < bars.size(); ++i) {
			if (bars[i].open_timestamp > tp) {
				return i - 1;
			}
		}

		return bars.size() - 1;
	}

	bool create_bars(Timeframe timeframe, Bars*& bars_ptr) {
		unique_lock lock(_mutex);
		auto it = _bars_by_timeframe.find(timeframe);
		if (it != _bars_by_timeframe.end()) {
			// Bars for this timeframe already exist
			bars_ptr = it->second;
			return true;
		}

		auto& bars_emplaced = _bars.emplace_front(calculateBars(timeframe, *_ticks));
		_bars_by_timeframe[timeframe] = &bars_emplaced;
		bars_ptr = &bars_emplaced;
		return true;
	}

};

