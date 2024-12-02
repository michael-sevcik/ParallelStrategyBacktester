module;

#include <vector>
#include <numeric>
#include <execution>
#include <utility>


export module StrategyOptimizer;
import StrategyTester;
import AlgoTrading;

namespace Backtesting {

/**
 * @brief Class for optimizing the parameters of a strategy.
 * @tparam AOS_T Type of the strategy.
 * @tparam Param_T Type of the parameters for the strategy factory method.
 */
export template <class AOS_T, class Param_T>
class StrategyOptimizer {
public:
	/**
	 * @brief Represents factory method for creating a robots with given parameters.
	 */
	using FactoryMethodPtr = AOS_T(*)(Param_T);

	/**
	 * @brief Constructor for the StrategyOptimizer.
	 * @param _strategy_tester_ptr the strategy tester to use to test parameter combinations.
	 * @param factory_method Factory method to use creating robots with given parameters. 
	 */
	StrategyOptimizer(
		StrategyTester* _strategy_tester_ptr,
		FactoryMethodPtr factory_method) :
		_strategy_tester_ptr(_strategy_tester_ptr),
		_factory_method(factory_method) {}

	/**
	 * @brief Tests all combinations of parameters and returns the best one.
	 * @tparam ExPo Execution policy type.
	 * @param expo The execution policy to use.
	 * @param combinations Combinations of parameters to test.
	 * @return pair of the best trading results and the best parameters.
	 */
	template <class ExPo = std::execution::par>
	std::pair<TradingResults, Param_T> findBestParameters(ExPo&& expo, const std::vector<Param_T>& combinations) {
		// lambda for transforming parameters into results parameter pairs
		auto transform = [
			_strategy_tester_ptr = this->_strategy_tester_ptr,
			_factory_method = this->_factory_method]
			(Param_T params) {
			AOS_T aos = _factory_method(params);
			auto results = _strategy_tester_ptr->run(aos);
			return std::make_pair(results, params);
			};

		// lambda for reducing the results parameter pairs
		auto reduce = [](std::pair<TradingResults, Param_T> a, std::pair<TradingResults, Param_T> b) {
			return a.first.account_balance > b.first.account_balance ? a : b;
			};

		return std::transform_reduce(
			expo,
			combinations.begin(),
			combinations.end(),
			std::pair<TradingResults, Param_T>(),
			reduce,
			transform);
	}

	/**
	 * @brief Finds the best parameters in a parallel manner.
	 * @param combinations Combinations of parameters to test.
	 * @return pair of the best trading results and the best parameters.
	 */
	std::pair<TradingResults, Param_T> findBestParametersParallel(const std::vector<Param_T>& combinations) {
		return findBestParameters(std::execution::par, combinations);
	}

	/**
	 * @brief Finds the best parameters in a sequential manner.
	 * @param combinations Combinations of parameters to test.
	 * @return pair of the best trading results and the best parameters.
	 */
	std::pair<TradingResults, Param_T> findBestParametersSeq(const std::vector<Param_T>& combinations) {
		return findBestParameters(std::execution::seq, combinations);
	}

private:
	StrategyTester* _strategy_tester_ptr;
	FactoryMethodPtr _factory_method;

};

}