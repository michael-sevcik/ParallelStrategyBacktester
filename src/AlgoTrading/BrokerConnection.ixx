module;
#include <string>
#include <chrono>
#include <list>
#include <vector>


export module BrokerConnection;

import MarketData;

/**
 * @brief Represents a pending order to be placed in the market.
 */
export struct Order {
    /**
     * @brief The size of the order (e.g., number of shares, lots).
     */
    volume volume;
    
    // if there would be any limit orders
    ///**
    // * @brief The time at which the order should expire if not filled.
    // */
    //TimePoint expiration;

    /**
     * @brief Specifies wherther it is a long order or short order.
     * @value True if it is long othrerwise false.
     */
    bool is_long;

    /**
     * @brief Optional comment associated with the order.
     */
    std::string comment;

    /**
     * @brief Stop-loss price level.  If set to -1, no stop-loss is in place.
     */
    price stoploss = -1;

    /**
     * @brief Take-profit price level. If set to -1, no take-profit is in place.
     */
    price takeprofit = -1;

    bool hasStoploss() {
        return stoploss != -1;
    }

    bool hasTakeprofit() {
        return takeprofit != -1;
    }
};

/**
 * @brief Represents an open position in the market.
 */
export struct Position {
    /**
	 * @brief Comparator for Position struct based on id.
    */
    struct Comparator {
        bool operator()(const Position& a, const Position& b) const {
            return a.id < b.id;
        }
    };

    /**
     * @brief List of positions.
     */
    using List = std::list<Position>;

    /**
	 * @brief Iterator for the list of positions.
    */
    using ListIterator = List::iterator;

    /**
     * @brief Identifier of a position.
     */
    using Id = size_t;

    /**
     * @brief Identifier of a position
     */
    Id id;

    /**
     * @brief The time the position was opened.
     */
    TimePoint open_time;

    /**
     * @brief The price at which the position was opened.
     */
    price open_price;

    /**
     * @brief The size of the position (e.g., number of shares, lots).
     */
    volume volume;

    /**
     * @brief True if the position is long, false if short.
     */
    bool is_long;

    /**
    * @brief Optional comment associated with the position.
    */
    std::string comment;

    /**
     * @brief Stop-loss price level. If set to -1, no stop-loss is in place.
     */
    price stoploss = -1;

    /**
     * @brief Take-profit price level. If set to -1, no take-profit is in place.
     */
    price takeprofit = -1;

    bool hasStoploss() {
        return stoploss != -1;
    }

    bool hasTakeprofit() {
        return takeprofit != -1;
    }

    bool hasPriceEvent() {
        return hasStoploss() || hasTakeprofit();
    }

    /**
     * @brief Calculates unrealized prifit/loss based on the current price.
     * @param tick tick to use in calculation
     * @return prifit/loss
     */
    double calculateProfit(const Tick& tick) {
        double price_difference;
        if (is_long) {
            price_difference = tick.bid - open_price;
        }
        else {
            price_difference = open_price - tick.ask;
        }

        return price_difference * volume;
    }
    
};


/**
 * @brief Represents a historical trade (a position that has been closed).
 */
export struct Trade {
public:
    /**
     * @brief Reason of closing appropriate position. 
     */
    enum CloseType {
        // In case of insufficent margin
        FORCED, 
        // Trigered by hitting stop loss
        STOPLOSS,
        // Trigered by hitting take profit
        TAKEPROFIT,
        // By close position order.
        CUSTOM
    };

    /**
     * @brief The time the trade was opened.
     */
    TimePoint open_time;

    /**
     * @brief The time the trade was closed.
     */
    TimePoint close_time;

    /**
     * @brief The price at which the trade was opened.
     */
    price open_price;

    /**
     * @brief The price at which the trade was closed.
     */
    price close_price;

    /**
     * @brief The size of the trade (e.g., number of shares, lots).
     */
    volume volume;

    /**
     * @brief True if the trade was long, false if short.
     */
    bool is_long;

    /**
     * @brief Specifies why was the position closed.
     */
    CloseType close_type;

    /**
     * @brief Optional comment associated with the trade.
     */
    std::string comment;

    /**
     * @brief Calculate realized profit on a given trade.
     * @return profit on a given trade.
     */
    double calculateProfit() const {
        price difference = close_price - open_price;
        price volume_times_difference = difference * volume;
        return (is_long ? 1 : -1) * volume_times_difference;
    }
};

export using Trades = std::vector<Trade>;

/**
 * @brief Represents an interface of a broker connection.
 */
export class BrokerConnection
{
public:
    virtual ~BrokerConnection() = default;  // Destructor
    /**
     * @brief Retrieves historical price bars for a specified timeframe.
     *
     * @param period The timeframe of each bar (e.g., Timeframe::M15 for 15-minute bars).
     * @param count number of bars to return.
     * @param  number of bars to return.
     * @param  bars output parameter - the bars view will be stored here.
     * @return True if the bars were found, false otherwise - not enough data.
     */
    virtual bool getLastBars(Timeframe period, size_t count, BarsView& bars) = 0;

    /**
     * @brief Retrieves historical price bars for a specified symbol and timeframe.
     *
     * @param symbol The financial instrument's symbol (e.g., "EURUSD").
     * @param period The timeframe of each bar (e.g., Timeframe::M15 for 15-minute bars).
     * @param from Start timestamp for the requested data range.
     * @param to End timestamp for the requested data range.
     * @return A collection of Bar objects representing the price data.
     */
    //virtual Bars getBars(const std::string& symbol, Timeframe period, std::time_t from, std::time_t to) = 0;

    /**
     * @brief Tries to places a new order.
     *
     * @param order An Order object containing the order details.
     * @param positionId Output parameter for id of a potentially created position.
     * @return True on success otherwise false - insufficient funds/margin.
     */
    virtual bool tryCreatePosition(const Order& order, Position::Id& positionId) = 0;

    /**
     * @brief Gets position by its id.
     * @param positionId Id of a desired position.
     * @return Reference of a position with the given Id.
     */
    virtual const Position& getPosition(Position::Id& positionId) = 0;

    /**
     * @brief Closes an open position with the broker.
     *
     * @param position A Position id identifying the position to be closed.
     */
    virtual void closePosition(Position::Id positionId) = 0;

    /**
     * @brief Closes all open positions with the broker.
     */
    virtual void closeAllPositions() = 0;

    /**
	 * @brief Gets the current server time.
	 * @return The current server time.
	 */
    virtual TimePoint getTime() = 0;

    /**
     * @brief Gets the current equity of the account.
     * @return The current equity value.
     */
    virtual double getEquity() = 0;

    /**
     * @brief Gets the current balance of the account.
     * @return The current balance value.
     */
    virtual double getBalance() = 0;
};