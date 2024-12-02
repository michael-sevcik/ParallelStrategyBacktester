module;

#include <chrono>
#include <iterator>

export module StrategyTester;

import AlgoTrading;
import SimulatedBrokerConnection;
import TradingManager;
import MarketDataManager;

export namespace Backtesting {
	using namespace BackTesting;
	using namespace std::chrono_literals;

	/**
	* @brief Specifies in what steps simulation should happen
	*/
	enum SimulationPeriod {
		TICK,
		S1,
		S5,
		S10,
		S30,
		MIN1
	};

	/**
	 * @brief durations of the simulation periods
	 */
	constexpr std::chrono::milliseconds timeframe_durations[]{
		1ms,
		1s,
		5s,
		10s,
		30s,
		1min,
	};

	/**
	 * @brief Represents the results of the trading
	*/
	using TradingResults = TradingManager::Results;

	/**
	 * @brief Represents the account properties
	*/
	using AccountProperties = BackTesting::AccountProperties;

	/**
	 * @brief Class that simulates the trading of a strategy
	*/
	class StrategyTester {
	public:
		/**
		 * @brief Construct a Strategy Tester object
		 * @param ticks_ptr ticks to use in simulation.
		 * @param period the period of the simulation.
		 * @param account_properties the account properties to use in simulation.
		 */
		StrategyTester(
			Ticks* ticks_ptr,
			SimulationPeriod period,
			AccountProperties&& account_properties) :
			_ticks_ptr(ticks_ptr),
			_market_data_manager(*ticks_ptr),
			_period(period),
			_account_properties(account_properties) {}

		/**
		 * @brief Runs the simulation of the strategy
		 * @param robot Robot to simulate.
		 * @return Results of the robot's trading.
		 */
		TradingResults run(ATS& robot) {
			TradingManager trading_manager(_account_properties);
			SimulatedBrokerConnection broker_connection(&trading_manager, &_market_data_manager);

			if (robot.start(&broker_connection) == ATS::ReturnCode::STOP) {
				return trading_manager.end();
			}

			// TODO: Use the SimulationPeriod
			if (_period == SimulationPeriod::TICK) {
				goThroughTicks(trading_manager, robot);
			}
			else {
				goThroughTicks(_period, trading_manager, robot);
			}

			robot.end();
			return trading_manager.end();
		}

	private:
		Ticks* _ticks_ptr;
		MarketDataManager _market_data_manager;
		SimulationPeriod _period;
		AccountProperties _account_properties;

		/**
		 * @brief Goes through the ticks and simulates the trading tick by tick.
		 * @param trading_manager Trading manager to use in simulation.
		 * @param robot the robot to simulate.
		 */
		void goThroughTicks(TradingManager& trading_manager, ATS& robot) {
			for (const auto& tick : *_ticks_ptr) {
				if (!handleTick(trading_manager, robot, tick)) {
					break;
				}
			}
		}

		/**
		 * @brief Goes through the ticks and simulates the trading with the specified period.
		 * @param period How much ticks to skip.
		 * @param trading_manager Trading manager to use in simulation.
		 * @param robot the robot to simulate.
		 */
		void goThroughTicks(SimulationPeriod period, TradingManager& trading_manager, ATS& robot) {
			TimePoint wait_for_timestamp = _ticks_ptr->front().timestamp;
			for (const auto& tick : *_ticks_ptr) {
				if (tick.timestamp < wait_for_timestamp) {
					continue;
				}

				wait_for_timestamp += timeframe_durations[(int)period];
				if (!handleTick(trading_manager, robot, tick)) {
					break;
				}
			}
		}

		/**
		* @brief Handles the tick in the simulation.
		* @param trading_manager Trading manager to use in simulation.
		* @param robot the robot to simulate.
		* @param tick the tick to handle.
		* @return true if the simulation should continue, false otherwise.
		*/
		bool handleTick(TradingManager& trading_manager, ATS& robot, const Tick& tick) {
			switch (trading_manager.onTick(tick))
			{
			case AccountState::MARGIN_CALL_WARNING:
				robot.onMarginCallWarning();
				break;
			case AccountState::NONPOSITIVE_ACCOUNT_BALANCE:
				return false;
			}

			if (robot.onTick(tick) == ATS::ReturnCode::STOP) {
				return false;
			}

			return true;
		}
	};
};

