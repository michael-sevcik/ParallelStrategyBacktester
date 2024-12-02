module;

#include <array>
#include <chrono>
#include <span>
#include <vector>

export module MarketData;

export {
	using namespace std::chrono_literals;
	using TimePoint = std::chrono::system_clock::time_point;

	/**
	 * @brief Represents various timeframes for market data.
	 */
	enum class Timeframe {
		MIN1,
		MIN5,
		MIN15,
		MIN30,
		H1,
		H4,
		D1,
		W1,
	};
	
	/**
	 * @brief Durations for the respective timeframes.
	 */
	constexpr std::chrono::milliseconds timeframe_durations[]{
		1min,
		5min,
		15min,
		30min,
		1h,
		4h,
		24h,
		168h
	};

	/**
	 * @brief Represents a price and volume in the market.
	*/
	using price = double;

	/**
	 * @brief Represents the number of units of an underlying asset.
	*/
	using volume = size_t;

	/**
	 * @brief Identifies which property of a Tick have changed.
	 */
	enum ChangeFlag {
		ASK = 2,
		BID = 4,
		ASK_AND_BID = 6,
		VOLUME = 312,
	};

	/**
	 * @brief Represents a single market tick (a price update).
	 *
	 * @param time Time of the price update.
	 * @param bid Best bid price at that moment.
	 * @param ask Best ask price at that moment.
	 * @param volume Trading volume associated with the tick.
	 * @param flags Potential flags indicating special conditions (e.g., trade outside regular hours).
	 */
	struct Tick {
		TimePoint timestamp;
		/**
		 * @brief The bid price refers to the highest price a buyer
		 * will pay for a security
		 */
		price bid;
		/**
		 * @brief The ask price refers to the lowest price a seller 
		 * will accept for a security
		 */
		price ask;
		volume volume;
		ChangeFlag flags;
	};

	using Ticks = std::vector<Tick>;

	/**
	 * @brief Represents an OHLC (Open-High-Low-Close) bar for a specific timeframe.
	 *
	 * @param open_timestamp The opening time of the bar
	 * @param open Opening price of the bar.
	 * @param high Highest price within the bar's timeframe.
	 * @param low Lowest price within the bar's timeframe.
	 * @param close Closing price of the bar.
	 * @param tick_volume Total trading volume based on ticks within the bar.
	 * @param spread Difference between the bid and ask prices at the close of the bar.
	 */
	struct Bar {
		TimePoint open_timestamp;
		price open;
		price high;
		price low;
		price close;
		volume tick_volume;

		void openBar(const Tick& tick) {
			open_timestamp = tick.timestamp;
			open = tick.ask;
			high = tick.bid;
			low = tick.ask;
			close = tick.bid;
			tick_volume = tick.volume;
		}

		void addTick(const Tick& tick) {
			high = std::max(high, tick.bid);
			low = std::min(low, tick.ask);
			close = tick.bid;
			tick_volume += tick.volume;
		}
	};

	/**
	 * @brief Bar collection.
	 */
	using Bars = std::vector<Bar>;

	/**
	 * @brief Bar view.
	 */
	using BarsView = std::span<const Bar>;
}