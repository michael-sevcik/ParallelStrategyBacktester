#include <iostream>
#include <vector>
#include <filesystem>
#include <chrono>
#include <string>
#include <functional>

import AlgoTrading;
import Utils;
import MovingAverageRobot;
import Backtesting;
import TickParser;

using namespace std;
using namespace utils;
using namespace Backtesting;

/**
 * @brief Prints the time point in a human readable format.
 * @param tp time point to print.
 */
void printTimePoint(TimePoint tp) {
	auto t = std::chrono::system_clock::to_time_t(tp);
	std::cout << std::ctime(&t);
}

/**
 * @brief Prints the tick in a human readable format.
 * @param tick Tick to print.
 */
void printTick(const Tick& tick) {
	std::cout << "Timestamp: ";
	printTimePoint(tick.timestamp);
	std::cout << std::endl;
	std::cout << "Bid: " << tick.bid << std::endl;
	std::cout << "Ask: " << tick.ask << std::endl;
	std::cout << "Volume: " << tick.volume << std::endl;
	std::cout << "Flags: " << tick.flags << std::endl;
	std::cout << std::endl;
}

/**
 * @brief Prints the results of the trading in a human readable format.
 * @param results The trading results to print.
 */
void printResults(const TradingResults& results) {
	std::cout << "====Unclosed Positions====" << endl;
	for (auto&& pos : results.unclosed_positions) {
		std::cout << (pos.is_long ? "Long" : "Short") << " position opened on ";
		printTimePoint(pos.open_time);
		std::cout << " with volume: " << pos.volume << " with price: " << pos.open_price << endl << endl;
	}

	std::cout << endl;
	std::cout << "====Trades====" << endl;
	for (auto&& trade : results.trades) {
		std::cout << (trade.is_long ? "Long" : "Short") << " position opened on ";
		printTimePoint(trade.open_time);
		std::cout << " with volume: " << trade.volume << " with price: " << trade.open_price << '.' << endl;
		std::cout << "Realized on ";
		printTimePoint(trade.close_time);
		std::cout << " on price: " << trade.close_price << " with profit/loss: "\
			<< trade.calculateProfit() << '.' << endl << endl;
	}

	std::cout << "Final account balance: " << results.account_balance << endl;
	//std::cout << "Final total equity (includes open positions): " << results.total_equity << endl;
}

/**
 * @brief Structure for storing parameters of the MovingAverageRobot.
 */
struct MovingAverageRobotParameters {
	size_t fast_MA_period;
	size_t slow_MA_period;
	float allowed_loss_on_trade;
	float risk_reward_ratio;
};

/**
 * @brief Generates combinations of parameters for the MovingAverageRobot.
 * @return Vector of parameter combinations.
 */
std::vector<MovingAverageRobotParameters> getParameterCombinations() {
	std::vector<MovingAverageRobotParameters> parameter_combinations;
	for (size_t fast_MA_period = 5; fast_MA_period < 12; fast_MA_period++) {
		for (size_t slow_MA_period = 12; slow_MA_period < 40; slow_MA_period++) {
			for (float allowed_loss_on_trade = 0.005; allowed_loss_on_trade < 0.025; allowed_loss_on_trade += 0.005) {
				for (float risk_reward_ratio = 1; risk_reward_ratio < 2; risk_reward_ratio += 0.2) {
					parameter_combinations.emplace_back(
						fast_MA_period,
						slow_MA_period,
						allowed_loss_on_trade,
						risk_reward_ratio);
				}
			}
		}
	}

	return parameter_combinations;
}

/**
 * @brief Creates a MovingAverageRobot with given parameters.
 * @param params Parameters for the robot.
 * @return Created robot.
 */
MovingAverageRobot createRobot(MovingAverageRobotParameters params) {
	return MovingAverageRobot(
		params.fast_MA_period,
		params.slow_MA_period,
		params.allowed_loss_on_trade,
		params.risk_reward_ratio);
}

/**
 * @brief Measures the time of execution of a function.
 * @tparam ReturnType Type of the return value of the function.
 * @tparam TimeT Type of the time measurement.
 * @param func Function to measure.
 * @param result Reference to the variable where the result of the function will be stored.
 * @return Time of execution of the function.
 */
template <typename ReturnType, typename TimeT = std::chrono::milliseconds>
size_t measure(std::function<ReturnType()> func, ReturnType& result) {
	using namespace std::chrono;

	auto start = high_resolution_clock::now();
	result = func();
	auto end = high_resolution_clock::now();

	auto duration = duration_cast<TimeT>(end - start);
	return duration.count();
}

int main(int argc, char* argv[]) {
	// first we need load and parse the ticks
	std::cout << "Parsing ticks (advised to run with release configuration)." << endl;
	TickParser tick_parser; // custom helper class for parsing the specific csv format.
	Ticks ticks;
	string path_to_csv_file;

	// for loading ticks we need a path to the file containing them in csv format
	std::cout << "Insert a path to a csv file containing ticks: ";
	std::getline(std::cin, path_to_csv_file);

	// load and parse the ticks using tick_parser
	auto parsing_duration = measure<Ticks>(
		[&]() {
		return tick_parser.getTicks(path_to_csv_file);
		}, 
		ticks);
	
	// check if ticks where parsed successfully
	if (ticks.empty()) {
		std::cout << endl << "The file is empty or we are not able to parse the ticks; please check the provided path, or file structure." << endl;
		return -1;
	}

	// print information about the tick parsing
	std::cout << "Number of ticks: " << ticks.size() << \
		" loaded and parsed in " << parsing_duration << " milliseconds" << endl;

	// Create a strategy tester
	StrategyTester tester(&ticks, SimulationPeriod::S1, AccountProperties());

	// measure running of one robot
	std::cout << "Simulating of one robot took ";
	TradingResults results;
	MovingAverageRobot robot(9, 20, 0.01, 1.6);
	auto one_robot_duration = measure<TradingResults>(
		[&]() { return tester.run(robot); }, results);
	std::cout << one_robot_duration << " milliseconds" << endl;

	/* ==================================================\
	** =====Moving to parameter combinations testing=====\
	*/                                                   

	// get combinations of parameters
	auto comb = getParameterCombinations();
	
	// initialize the StrategyOptimizer with pointer to configured StrategyTester and robot factory method
	StrategyOptimizer<MovingAverageRobot, MovingAverageRobotParameters> optimizer(&tester, createRobot);
	
	// measure the parallel testing of parameter combinations
	std::cout << "Simulating of " << comb.size() << " robots took ";
	std::pair<TradingResults, MovingAverageRobotParameters> best_pair;
	auto parallel_sim_duration = measure<std::pair<TradingResults, MovingAverageRobotParameters>>(
		[&]() { return optimizer.findBestParametersParallel(comb); }, best_pair);
	std::cout << parallel_sim_duration << " milliseconds in parallel and ";

	// measure the sequential testing of parameter combinations
	auto seq_sim_duration = measure<std::pair<TradingResults, MovingAverageRobotParameters>>(
		[&]() { return optimizer.findBestParametersSeq(comb); }, best_pair);
	std::cout << seq_sim_duration << " milliseconds in sequential." << endl;

	// calculate speedup
	float speedup = static_cast<float>(seq_sim_duration) / parallel_sim_duration;
	std::cout << "Which means we have achieved " << speedup << " speedup factor." << endl;

	// print trading results on demand.
	std::cout << endl << "Press a key to print trading results of a robot with the best parameters" << endl;
	auto [best_results, best_params] = best_pair;
	cin.get();
	printResults(best_results);

	return 0;
}