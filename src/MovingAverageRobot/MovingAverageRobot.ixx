module;
#include <deque>
#include <stdexcept>
#include <chrono>

export module MovingAverageRobot;


import AlgoTrading;
constexpr Timeframe UsedTimeframe = Timeframe::MIN5;
/**
 * @brief Represents a helper class for calculating averages.
 */
class MovingAverage {
public:
	/**
	 * @brief Number of stored MA values
	 */
	static const size_t N = 3;
	MovingAverage() noexcept : _period() {}

	/**
	 * @brief Initializes the MovingAverage with the given bars and period.
	 * @param bars the bars to use for initialization
	 * @param period The period of the moving average
	 */
	void initialize(const BarsView bars, size_t period) {
		_period = period;
		if (bars.size() < N + period - 1) {
			throw std::invalid_argument("Insufficient number of bars");
		}

		price accumulation = 0;
		for (size_t i = 0; i < period; i++) {
			price current_close = bars[i].close;
			accumulation += current_close;
			_last_prices.push_back(current_close);
		}

		price base_mean = accumulation / period;
		_last_values.push_back(base_mean);

		for (size_t i = period; i < period + N - 1; i++) {
			price current_close = bars[i].close;
			price old = addNewPrice(current_close);
			price next_value = calculateNextValue(old, current_close, _last_values.back());
			_last_values.push_back(next_value);
		}
	}

	MovingAverage& operator=(MovingAverage&& other) noexcept = default;

	enum CrossingState {
		NO_CROSSING,
		BULLISH, 
		BEARISH
	};

	/**
	* @brief Checks if the two moving averages have crossed.
	* @param slower the slower moving average
	* @param faster the faster moving average
	* @return the crossing state
	*/
	static CrossingState have_crossed(const MovingAverage& slower, const MovingAverage& faster) {
		if (slower[0] > faster[0]
			&& slower[1] <= faster[1]
			&& slower[2] < faster[2]) {
			return BULLISH;
		}

		if (slower[0] < faster[0]
			&& slower[1] >= faster[1]
			&& slower[2] > faster[2]) {
			return BEARISH;
		}

		return NO_CROSSING;
	}	

	/**
	 * @brief Updates the moving average with the new bar.
	 * @param bar the new bar.
	 */
	void add(const Bar& bar) {
		price current_close = bar.close;
		price old = addNewPrice(current_close);
		price next_value = calculateNextValue(old, current_close, _last_values.back());
		_last_values.pop_front();
		_last_values.push_back(next_value);
	}

	/**
	 * @brief Gets a value of the moving average.
	 * @param i index of the value
	 * @return value of the moving average
	 */
	price operator[](size_t i) const {
		return _last_values[i];
	}

private:
	size_t _period;
	std::deque<price> _last_values;
	std::deque<price> _last_prices;

	price addNewPrice(price new_price) {
		price old = _last_prices.front();
		_last_prices.pop_front();
		_last_prices.push_back(new_price);
		return old;
	}

	price calculateNextValue(price old, price to_add, price current_mean) const {
		return current_mean + ((to_add - old) / _period);
	}
};

export class MovingAverageRobot : public ATS {
private:
	/**
	 * @brief Number of additional bars needed for calculating crossovers
	 */
	static const size_t NUMBER_OF_ADDITIONAL_BARS = 2;

	enum State {
		WAITING_FOR_BARS,
		TRADING,
		WAITING_FOR_NEXT_BAR,
	};

	State _state = WAITING_FOR_BARS;
	TimePoint _wait_for_timestamp;
	BrokerConnection* _broker = nullptr;
	size_t _short_period;
	size_t _long_period;

	float _allowed_risk_on_trade;
	float _risk_reward_ratio;

	MovingAverage _shortMA;
	MovingAverage _longMA;

	price findMinimumPrice() const;

	price findMaximumPrice() const;

	volume calculateVolume(price price_difference) const {
		return _allowed_risk_on_trade * _broker->getBalance() / price_difference;
	}

	void trade(const Tick& tick);
public:
	/**
	* @brief Constructs the MovingAverageRobot with the given parameters.
	* @param short_period the period of the short moving average
	* @param long_period the period of the long moving average
	* @param allowed_risk_on_trade the allowed risk on a single trade
	* @param risk_reward_ration the risk reward ratio
	*/
	MovingAverageRobot(
		size_t short_period,
		size_t long_period,
		float allowed_risk_on_trade,
		float risk_reward_ration) noexcept :
		_short_period(short_period),
		_long_period(long_period),
		_allowed_risk_on_trade(allowed_risk_on_trade),
		_risk_reward_ratio(risk_reward_ration) { }

	ATS::ReturnCode start(BrokerConnection* broker_connection) override {
		_broker = broker_connection;
		return OK;
	}

	int onTick(const Tick& tick) override {
		// wait till we have N + K bars
		BarsView view;

		switch (_state)
		{
		case MovingAverageRobot::WAITING_FOR_BARS: {
			BarsView short_view;
			if (_broker->getLastBars(UsedTimeframe, _long_period + NUMBER_OF_ADDITIONAL_BARS, view)
				&& _broker->getLastBars(UsedTimeframe, _short_period + NUMBER_OF_ADDITIONAL_BARS, short_view)) {
				_shortMA.initialize(short_view, _short_period);
				_longMA.initialize(view, _long_period);
				_state = TRADING;
			}
			break;
		}
		case MovingAverageRobot::TRADING:
			if (_broker->getLastBars(UsedTimeframe, 1, view)) {
				const Bar& bar = view[0];
				_wait_for_timestamp = view[0].open_timestamp + timeframe_durations[(int)UsedTimeframe];
				_shortMA.add(bar);
				_longMA.add(bar);
				trade(tick);
				_state = WAITING_FOR_NEXT_BAR;
			}
			break;

		case MovingAverageRobot::WAITING_FOR_NEXT_BAR:
			if (_wait_for_timestamp <= tick.timestamp) {
				_state = TRADING;
			}
			break;
		}

		return 0;
	}

	void end() override {
		_broker->closeAllPositions();
	}
};

price MovingAverageRobot::findMinimumPrice() const
{
	BarsView view;
	_broker->getLastBars(UsedTimeframe, _long_period, view);
	price min = view.front().low;
	for (const Bar& bar : view) {
		if (bar.low < min) {
			min = bar.low;
		}
	}

	return min;
}

price MovingAverageRobot::findMaximumPrice() const
{
	BarsView view;
	_broker->getLastBars(UsedTimeframe, _long_period, view);
	price max = view.front().low;
	for (const Bar& bar : view) {
		if (bar.high > max) {
			max = bar.low;
		}
	}

	return max;
}

/**
 * @brief Trades based on the moving averages
 * @param tick the current tick
 */
void MovingAverageRobot::trade(const Tick& tick) {
	bool should_place_order = false;
	price stoploss_current_difference = 0;
	Order order;

	switch (MovingAverage::have_crossed(_longMA, _shortMA))
	{
	case MovingAverage::CrossingState::BULLISH:
		order.stoploss = findMinimumPrice();
		order.is_long = true;
		stoploss_current_difference = tick.bid - order.stoploss;
		order.takeprofit = tick.bid + stoploss_current_difference * _risk_reward_ratio;
		should_place_order = true;
		break;
	case MovingAverage::CrossingState::BEARISH:
		order.stoploss = findMaximumPrice();
		order.is_long = false;
		stoploss_current_difference = order.stoploss - tick.ask;
		order.takeprofit = tick.ask - stoploss_current_difference * _risk_reward_ratio;
		should_place_order = true;
		break;
	default: ;
	}

	if (should_place_order) {
		order.volume = calculateVolume(stoploss_current_difference);
		Position::Id id;
		_broker->tryCreatePosition(order, id);
	}
}
