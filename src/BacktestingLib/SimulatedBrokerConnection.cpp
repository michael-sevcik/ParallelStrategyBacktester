module;

#include <string>
#include <chrono>

export module SimulatedBrokerConnection;

import AlgoTrading;
import BrokerConnection;
import TradingManager;
import MarketDataManager;

using namespace BackTesting;

/**
 * @brief Implementation of the BrokerConnection interface for a simulated broker.
 */
export class SimulatedBrokerConnection : public BrokerConnection {
public:
	/**
	* @brief Constructor.
	* @param trading_manager_ptr Pointer to the trading manager.
	* @param market_data_manager_ptr Pointer to the market data manager.
	*/
	SimulatedBrokerConnection(
		TradingManager* trading_manager_ptr,
		MarketDataManager* market_data_manager_ptr) noexcept :
		_trading_manager_ptr(trading_manager_ptr),
		_market_data_manager_ptr(market_data_manager_ptr) {}

	bool getLastBars(Timeframe period, size_t count, BarsView& bars) override;
	TimePoint getTime() override;
	bool tryCreatePosition(const Order& order, Position::Id& positionId) override;
	const Position& getPosition(Position::Id& positionId) override;

	void closePosition(Position::Id positionId) override;
	void closeAllPositions() override;

	double getBalance() override;
	double getEquity() override;
private:
	TradingManager* _trading_manager_ptr;
	MarketDataManager* _market_data_manager_ptr;
};

bool SimulatedBrokerConnection::getLastBars(Timeframe period, size_t count, BarsView& bars) {
	return _market_data_manager_ptr->getLastBarsBefore(
		period,
		getTime(),
		count,
		bars);
}

bool SimulatedBrokerConnection::tryCreatePosition(const Order& order, Position::Id& positionId) {
	return _trading_manager_ptr->tryCreatePosition(order, positionId);
}

const Position& SimulatedBrokerConnection::getPosition(Position::Id& positionId) {
	return _trading_manager_ptr->getPosition(positionId);
}


void SimulatedBrokerConnection::closePosition(Position::Id positionId) {
	_trading_manager_ptr->closePosition(positionId);
}

void SimulatedBrokerConnection::closeAllPositions() {
	_trading_manager_ptr->closeAllPositions();
}

double SimulatedBrokerConnection::getBalance() {
	return _trading_manager_ptr->getBalance();
}

double SimulatedBrokerConnection::getEquity() {
	return _trading_manager_ptr->getEquity();
}

TimePoint SimulatedBrokerConnection::getTime() {
	return _trading_manager_ptr->getCurrentTime();
}
