module;
#include <ranges>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <functional>
#include <utility>
#include <cassert>

export module TradingManager;
import AlgoTrading;
using namespace std;

namespace BackTesting {
	/**
	* @brief Represents the base account properties.
	*/
	export struct AccountProperties {
		/**
		 * @brief Starting balance of an account
		 * @note Base currency of the asset is considered as the account currency.
		 */
		double account_balance = 10000;

		/**
		 * @brief Specifies what leverage should be used during the simulation.
		 */
		unsigned int leverage = 50;

		/**
		 * @brief Specifies on which level of margin stop out should take place.
		 */
		float stop_out_level = 0.5;

		/**
		 * @brief Specifies on which level of margin a warning should be issued (margin call).
		 */
		float stop_out_warning_level = 0.55;
	};

	/**
	 * @brief Alias for iterator of the list of positions.
	 */
	using PositionsIterator = Position::ListIterator;

	/***
	 * @brief Represents an interface of an iterator queue.
	 */
	class PositionIteratorQueueBase {
	public:
		virtual ~PositionIteratorQueueBase() = default;

		/**
		 * @brief Push an iterator on the queue.
		 * @param iter The iterator to push.
		 */
		virtual void push(PositionsIterator iter) = 0;

		/**
		 * @brief Update the queue based on priority.
		 */
		virtual void update() = 0;

		/**
		 * @brief Remove the iterator from the queue.
		 * @param iter The iterator to remove.
		 * @return True if the iterator was removed, otherwise false.
		 */
		virtual bool remove(PositionsIterator iter) = 0;

		/**
		 * @brief Pop the iterator from the queue.
		 * @return The iterator that was popped.
		 */
		virtual PositionsIterator pop() = 0;

		/**
		 * @brief Get the top iterator from the queue.
		 * @return The top iterator from the queue.
		 */
		virtual const Position& top() = 0;

		/**
		 * @brief Check if the queue is empty.
		 * @return True if the queue is empty, otherwise false.
		 */
		virtual bool empty() const = 0;
	};

	/**
	 * @brief Priority queue for position iterators.
	 * @tparam PriorityCompT Priority comparator for position iterators.
	 */
	template <class PriorityCompT>
	class PositionIteratorQueue : public PositionIteratorQueueBase {
	public:
		void push(PositionsIterator iter) override {
			heap.push_back(iter);
			std::push_heap(heap.begin(), heap.end(), PriorityCompT{});
		}

		void update() override {
			std::make_heap(heap.begin(), heap.end(), PriorityCompT{});
		}

		bool remove(PositionsIterator iter) override {
			for (auto i = heap.begin(); i != heap.end(); ++i) {
				if (*i == iter) {
					heap.erase(i);
					update();
					return true;
				}
			}

			return false;
		}

		PositionsIterator pop() override {
			PositionsIterator popped = heap.front();
			pop_heap(heap.begin(), heap.end(), PriorityCompT{});
			heap.pop_back();
			return popped;
		}

		const Position& top() override {
			return *(heap.front());
		}

		bool empty() const override {
			return heap.empty();
		}

	private:
		vector<PositionsIterator> heap;
	};

	/**
	 * @brief Represents an event manager for price events.
	 * @tparam LongCompT Comparator for long positions.
	 * @tparam ShortCompT Comparator for short positions.
	 * @tparam LongPositionPredicate Predicate for long positions.
	 * @tparam ShortPositionPredicate Predicate for short positions.
	 */
	template <class LongCompT, class ShortCompT, class LongPositionPredicate, class ShortPositionPredicate>
	class PriceEventManager {
	public:
		using EventHandler = function<void(PositionsIterator)>;
		PriceEventManager(EventHandler&& handler) : _handler(move(handler)) {}

		void add(PositionsIterator iter) {
			auto& queue = getQueueByPositionType(iter);
			queue.push(iter);
		}

		void remove(PositionsIterator iter) {
			auto& queue = getQueueByPositionType(iter);
			const bool result = queue.remove(iter);
			assert(result == true);
		}

		void onTick(const Tick& tick) {
			checkEvents<LongPositionPredicate>(tick.bid, _long_positions);
			checkEvents<ShortPositionPredicate>(tick.ask, _short_positions);
		}
	private:
		EventHandler _handler;
		PositionIteratorQueue<LongCompT> _long_positions;
		PositionIteratorQueue<ShortCompT> _short_positions;

		PositionIteratorQueueBase& getQueueByPositionType(const PositionsIterator& iter) {
			if ((*iter).is_long) {
				return _long_positions;
			}

			return _short_positions;
		}

		template <class EventPredicate>
		void checkEvents(price p, PositionIteratorQueueBase& q) {
			EventPredicate predicate;
			while (!q.empty() && predicate(p, q.top())) {
				_handler(q.pop());
			}
		}

	};

	/**
	 * @brief Represents the state of the account.
	 */
	export enum AccountState {
		OK,
		NONPOSITIVE_ACCOUNT_BALANCE,
		MARGIN_CALL,
		MARGIN_CALL_WARNING
	};

	/**
	 * @brief Manager for keeping account state.
	 */
	class AccountBalanceManager {
	public:
		AccountBalanceManager(
			double account_balance,
			unsigned int leverage,
			float stop_out_level,
			float stop_out_warning_level) noexcept :
			_account_balance(account_balance),
			_leverage(leverage),
			_stop_out_level(stop_out_level),
			_stop_out_warning_level(stop_out_warning_level) {}

		AccountBalanceManager(const AccountProperties& properties) noexcept :
			_account_balance(properties.account_balance),
			_leverage(properties.leverage),
			_stop_out_level(properties.stop_out_level),
			_stop_out_warning_level(properties.stop_out_warning_level) {}

		double getBalance() const {
			return _account_balance;
		}

		double getTotalEquity() const {
			return _account_balance + _open_position_equity;
		}

		double getTotalExpenses() const {
			return _long_positions_expanses + _short_positions_expanses;
		}
		
		double getUsedMargin() const {
			return getTotalExpenses() / _leverage;
		}

		double getUsedMargin(double additional_expanses) const {
			return (getTotalExpenses() + additional_expanses) / _leverage;
		}

		double getFreeMargin() const {
			return getTotalEquity() - getUsedMargin();
		}

		double calculateRequiredMargin(volume volume, price open_price) const {
			return (volume * open_price) / _leverage;
		}

		float getMarginLevel() const {
			return getTotalEquity() / getUsedMargin();
		}

		bool canOrderBeProcessed(
			volume volume,
			price open_price,
			price close_price) const {
			double price_difference = close_price - open_price;
			if (price_difference < 0) {
				price_difference *= -1;
			}

			double new_equity = getTotalEquity() - ((double)volume * price_difference);
			double new_used_margin = getUsedMargin() + calculateRequiredMargin(volume, open_price);
			
			float after_processing_margin_level = new_equity / new_used_margin;
			return after_processing_margin_level > _stop_out_level;
		}

		void addPosition(const Position& pos) {
			if (pos.is_long) {
				_long_volume += pos.volume;
				_long_positions_expanses += pos.volume * pos.open_price;
			}
			else {
				_short_volume += pos.volume;
				_short_positions_expanses += pos.volume * pos.open_price;
			}
		}

		void realizePosition(const Trade& trade) {
			if (trade.is_long) {
				_long_volume -= trade.volume;
				_long_positions_expanses -= trade.volume * trade.open_price;
			}
			else {
				_short_volume -= trade.volume;
				_short_positions_expanses -= trade.volume * trade.open_price;
			}

			_account_balance += trade.calculateProfit();
		}

		AccountState onTick(const Tick& tick) {
			updateOpenPositionEquity(tick);
			if (_account_balance <= 0) {
				return AccountState::NONPOSITIVE_ACCOUNT_BALANCE;
			}

			float margin_level = getMarginLevel();
			if (margin_level <= _stop_out_level) {
				return AccountState::MARGIN_CALL;
			}

			if (margin_level <= _stop_out_warning_level) {
				return AccountState::MARGIN_CALL_WARNING;
			}

			return AccountState::OK;
		}



	private:
		float _stop_out_level;
		float _stop_out_warning_level;
		double _account_balance;
		unsigned int _leverage;
		double _open_position_equity = 0;
		double _long_positions_expanses = 0;
		volume _long_volume = 0;

		double _short_positions_expanses = 0;
		volume _short_volume = 0;

		void updateOpenPositionEquity(const Tick& tick) {
			// calculate profits = current_value - expanse
			double long_profit = tick.bid * _long_volume - _long_positions_expanses;
			double short_profit = _short_positions_expanses - tick.ask * _short_volume;

			_open_position_equity = long_profit + short_profit;
		}
	};


	/**
	 * @brief Represents entity that is responsible for managing trading
	 * (including margin calls).
	 */
	export class TradingManager {
	public:
		/**
		* @brief Represents the results of the trading
		*/
		struct Results {
			/**
			* @brief Account balance at the end of the simulation.
			*/
			double account_balance;
			
			/**
			 * @brief total equity at the end of the simulation
			 */
			double total_equity;

			/**
			* @brief Unclosed positions at the end of the simulation.
			*/
			Position::List unclosed_positions;

			/**
			* @brief Trades made by the end of the simulation.
			*/
			Trades trades;
		};

		/**
		 * @brief Constructs a Trading Manager object
		 * @param properties the account properties to use in simulation.
		 */
		TradingManager(const AccountProperties& properties): _account_manager(properties) {}

		/**
		 * @brief Simulates the trading on a given tick.
		 * @param tick The tick to simulate.
		 * @return State of the account after the simulation.
		 */
		AccountState onTick(const Tick& tick);

		/**
		 * @brief gets the position by id
		 * @param id Id of the position to get.
		 * @return position reference with the given id.
		 */
		const Position& getPosition(Position::Id id);

		/**
		 * @brief Closes the position with the given id.
		 * @param id Id of the position to close.
		 */
		void closePosition(Position::Id id);

		/**
		 * @brief Closes all positions.
		 */
		void closeAllPositions() {
			closeAllPositions(Trade::CloseType::FORCED);
		}

		/**
		 * @brief Gets the current balance of the account.
		 * @return The current balance of the account.
		 */
		double getBalance() {
			return _account_manager.getBalance();
		}

		/**
		 * @brief Gets the current equity of the account (balanced plus unrealized profit/loss).
		 * @return the current equity of the account (balanced plus unrealized profit/loss).
		 */
		double getEquity() {
			return _account_manager.getTotalEquity();
		}

		/**
		 * @brief Tries to fulfill a given order.
		 * @param order A given order.
		 * @param pos Output parameter - id of created position.
		 * @return True on success, otherwise false - insufficient funds/margin.
		 */
		bool tryCreatePosition(const Order& order, Position::Id& pos);

		/**
		 * @brief Gets the time of the current tick.
		 * @return the time of the current tick.
		 */
		TimePoint getCurrentTime() const {
			return _current_tick.timestamp;
		}

		/**
		 * @brief Finializes the simulation and returns the results.
		 * @return Trading results.
		 */
		Results end() {
			return {
				_account_manager.getBalance(),
				_account_manager.getTotalEquity(),
				move(_positions),
				move(_trades),
			};
		}
		 
	private:
		using CPositionIteratorRef = const PositionsIterator&;
		using IteratorLongStoplossComparer = decltype([](CPositionIteratorRef left, CPositionIteratorRef right) {
			return (*left).stoploss < (*right).stoploss;
			});
		using IteratorLongTakeprofitComparer = decltype([](CPositionIteratorRef left, CPositionIteratorRef right) {
			return (*left).takeprofit > (*right).takeprofit;
			});

		using IteratorShortStoplossComparer = decltype([](CPositionIteratorRef left, CPositionIteratorRef right) {
			return (*left).stoploss > (*right).stoploss;
			});
		using IteratorShortTakeprofitComparer = decltype([](CPositionIteratorRef left, CPositionIteratorRef right) {
			return (*left).takeprofit < (*right).takeprofit;
			});

		using LongStoplossPredicate = decltype([](price current_p, const Position& pos) {
			return current_p <= pos.stoploss;
			});

		using ShortStoplossPredicate = decltype([](price current_p, const Position& pos) {
			return current_p >= pos.stoploss;
			});

		using LongTakeprofitPredicate = decltype([](price current_p, const Position& pos) {
			return current_p >= pos.takeprofit;
			});

		using ShortTakeprofitPredicate = decltype([](price current_p, const Position& pos) {
			return current_p <= pos.takeprofit;
			});

		using StoplossManager = PriceEventManager<IteratorLongStoplossComparer, IteratorShortStoplossComparer, LongStoplossPredicate, ShortStoplossPredicate>;
		using TakeprofitManager = PriceEventManager<IteratorLongTakeprofitComparer, IteratorShortTakeprofitComparer, LongTakeprofitPredicate, ShortTakeprofitPredicate>;

		size_t _new_id;
		Position::List _positions;
		Trades _trades;
		Tick _current_tick;
		unordered_map<Position::Id, PositionsIterator> _position_iterators_by_id;
		StoplossManager _stoploss_manager = StoplossManager(function([this](PositionsIterator iter) {
			_takeprofit_manager.remove(iter);
			closePosition(iter, Trade::CloseType::STOPLOSS);
			}));

		TakeprofitManager _takeprofit_manager = TakeprofitManager(function([this](PositionsIterator iter) {
			_stoploss_manager.remove(iter);
			closePosition(iter, Trade::CloseType::TAKEPROFIT);
			}));

		AccountBalanceManager _account_manager;

		void unregisterPositionEvents(PositionsIterator iter) {
			auto& position = *iter;
			if (position.hasStoploss()) {
				_stoploss_manager.remove(iter);
			}

			if (position.hasTakeprofit()) {
				_takeprofit_manager.remove(iter);
			}
		}

		void registerPositionEvents(PositionsIterator iter) {
			auto& position = *iter;
			if (position.hasStoploss()) {
				_stoploss_manager.add(iter);
			}

			if (position.hasTakeprofit()) {
				_takeprofit_manager.add(iter);
			}
		}

		void forcedClosePosition(PositionsIterator iter) {
			unregisterPositionEvents(iter);
			closePosition(iter, Trade::CloseType::FORCED);
		}

		void closePosition(PositionsIterator iter, Trade::CloseType ct);

		void closeAllPositions(Trade::CloseType ct) {
			while (!_positions.empty()) {
				auto iter = _positions.begin();
				unregisterPositionEvents(iter);
				closePosition(iter, ct);
			}
		}
	};
	
	AccountState TradingManager::onTick(const Tick& tick) {
		_current_tick = tick;

		// Check if any price events should happen (handled by callbacks)
		_stoploss_manager.onTick(tick);
		_takeprofit_manager.onTick(tick);

		AccountState state = _account_manager.onTick(tick);
		switch (state)
		{
		case BackTesting::NONPOSITIVE_ACCOUNT_BALANCE:
			closeAllPositions(Trade::CloseType::FORCED);
			break;
		case BackTesting::MARGIN_CALL:
			if (!_positions.empty()) {
				forcedClosePosition(_positions.begin());
			}
			break;
		}

		return state;
	}
	
	const Position& TradingManager::getPosition(Position::Id id) {
		return *_position_iterators_by_id[id];
	}
	
	void TradingManager::closePosition(Position::Id id) {
		auto position_iter = _position_iterators_by_id[id];
		unregisterPositionEvents(position_iter);
		closePosition(position_iter, Trade::CloseType::FORCED);
	}

	/**
	* @brief Tries to fulfill a given order.
	* @param order A given order.
	* @param pos Output parameter - id of created position.
	* @return True on success, otherwise false - insufficient funds/margin.
	*/
	bool TradingManager::tryCreatePosition(const Order& order, Position::Id& pos) {
		// prepare open prices based on order type
		price open_price = _current_tick.ask;
		price eventual_close_price = _current_tick.bid;
		if (!order.is_long) {
			price open_price = _current_tick.bid;
			price eventual_close_price = _current_tick.ask;
		}

		// check whether we can process the order
		if (!_account_manager.canOrderBeProcessed(
			order.volume,
			open_price,
			eventual_close_price)) {
			return false;
		}

		// process the given order
		Position& position = _positions.emplace_front(
			_new_id++,
			_current_tick.timestamp,
			open_price,
			order.volume,
			order.is_long,
			order.comment,
			order.stoploss,
			order.takeprofit);

		auto position_iter = _positions.begin();
		_position_iterators_by_id[position.id] = position_iter;

		_account_manager.addPosition(position);
		registerPositionEvents(position_iter);

		pos = position.id;
		return true;
	}
	
	void TradingManager::closePosition(PositionsIterator iter, Trade::CloseType ct) {
		auto& pos = *iter;
		auto& trade = _trades.emplace_back(
			pos.open_time,
			_current_tick.timestamp,
			pos.open_price,
			(pos.is_long ? _current_tick.bid : _current_tick.ask),
			pos.volume,
			pos.is_long,
			ct,
			move(pos.comment)
		);

		_account_manager.realizePosition(trade);

		auto by_id_iterator = _position_iterators_by_id.find(pos.id);
		assert(by_id_iterator == _position_iterators_by_id.end());

		_position_iterators_by_id.erase(by_id_iterator);
		_positions.erase(iter);
	}
}