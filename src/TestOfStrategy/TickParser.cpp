module;

#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <format>
#include <chrono>

export module TickParser;

import AlgoTrading;
import Utils;

using namespace std;
using namespace utils;

/**
 * @brief Represents a parser for ticks in the specified custom format.
 */
export class TickParser {
public:

	TickParser() {
		_intermediate_tick.timestamp = std::chrono::system_clock::now();
		_intermediate_tick.bid = 0.0;
		_intermediate_tick.ask = 0.0;
		_intermediate_tick.volume = 0.0;
		_intermediate_tick.flags = ChangeFlag::ASK_AND_BID;
	}

	/**
	* @brief Parses the ticks from the specified file.
	* @param path The path to the file with ticks.
	* @return The parsed ticks.
	*/
	Ticks getTicks(const std::string& path) {
		Ticks ticks;
		try {
			CSVParser parser(path, 6, '\t', true);
			RowStream row_stream = parser.getRowStream();
			Row row;
			while (row_stream) {
				row_stream >> row;
				ticks.emplace_back(createTick(row));
			}
		}
		catch (const std::runtime_error&) {
			return ticks;
		}

		return ticks;
	}

private:
	Tick _intermediate_tick;

	ChangeFlag getFlags(Cell cell) const {
		int flag_value;
		if (!cell.toNumber(flag_value))
		{
			throw format_error("Flag is not parsable - either empty or malformatted.");
		}

		switch (flag_value)
		{
		case ChangeFlag::ASK_AND_BID:
			return ChangeFlag::ASK_AND_BID;
		case ChangeFlag::ASK:
			return ChangeFlag::ASK;
		case ChangeFlag::BID:
			return ChangeFlag::BID;
		case ChangeFlag::VOLUME:
			return ChangeFlag::VOLUME;
		default:
			throw format_error("Unexpected flag value");
		}
	}

	Tick createTick(Row& row) {
		CellStream cell_stream = row.getCellStream();

		// format of the csv file: <DATE>	<TIME>	<BID>	<ASK>	<LAST>	<VOLUME>	<FLAGS>
		{
			std::string timestamp = "";
			timestamp += cell_stream.next().toView();
			timestamp += " ";
			timestamp += cell_stream.next().toView();
			_intermediate_tick.timestamp = convertToTime(timestamp);
		}

		_intermediate_tick.bid = cell_stream.next().toNumberOrDefault(_intermediate_tick.bid);
		_intermediate_tick.ask = cell_stream.next().toNumberOrDefault(_intermediate_tick.ask);
		cell_stream.next(); // skip column <LAST>

		Cell cell = cell_stream.next();
		_intermediate_tick.volume = cell.toNumberOrDefault(_intermediate_tick.volume);

		_intermediate_tick.flags = getFlags(cell_stream.next());
		return _intermediate_tick;
	}

	TimePoint convertToTime(std::string& timestamp) {
		static string format = "%Y.%m.%d %H:%M:%S";
		std::istringstream iss(timestamp);
		TimePoint tp;
		iss >> std::chrono::parse(format.c_str(), tp);
		if (iss.fail()) {
			throw format_error("Enexpected format of timestamps - conversion failed.");
		}

		return tp;
	}
};
