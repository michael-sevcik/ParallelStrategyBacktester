module;

export module AlgoTrading:Robot;

import BrokerConnection;
import MarketData;
  
/**
 * @brief Represents a Interface for an automatic trading system.
 */
export class ATS {
public:
	/**
	 * @brief Represents a return code for the ATS.
	 */
	enum ReturnCode {

		OK = 0, // success
		STOP = -1, // error

	};

	/**
	 * @brief Default constructor.
	 */
	virtual ~ATS() = default;

	/**
	 * @brief Called when the ATS is started.
	 * @return ReturnCode
	 */
	virtual int onTick(const Tick&) = 0;

	/**
	 * @brief Called when the ATS is being started.
	 * @param brokerConnection The broker connection.
	 * @return returns specifying if the ATS was started successfully.
	 */
	virtual ReturnCode start(BrokerConnection* brokerConnection) { 
		return ReturnCode::OK;
	}

	/**
	 * @brief Is called when a margin level of the account reaches margin warning level.
	 */
	virtual void onMarginCallWarning() {}

	/**
	 * @brief Called when the ATS is being stopped.
	 */
	virtual void end() = 0;
};

