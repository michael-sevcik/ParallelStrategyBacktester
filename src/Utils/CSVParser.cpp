module;
#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <ranges>
#include <vector>
#include <memory>
#include <charconv>
#include <optional>
#include <utility>
#include <stdexcept>

export module Utils : CSVParser;

using namespace std;
export namespace utils {

/**
 * @brief Represents an exception that is thrown when the end of the stream is reached.
 */
class EndOfStreamException : public std::runtime_error {
public:
	EndOfStreamException() :
		std::runtime_error("End of stream reached") {}
};

/**
 * @brief Represents an exception that is thrown when the file cannot be opened.
 */
class CanNotOpenFileError : public std::runtime_error {
public:
	CanNotOpenFileError() :
		std::runtime_error("Cannot open the provided file.") {}
};


class CellStream;
class Row;
class CSVParser;


/**
 * @brief Represents a parser for CSV files.
 */
class CSVParser {
	friend class RowStream;

public:
	/**
	* @brief Initializes the CSVParser with the specified path, column count, delimiter and whether to skip the first line.
	* @param path The path to the file to parse.
	* @param column_count The number of columns in the file.
	* @param delimiter The delimiter used in the file.
	* @param skip_first_line Whether to skip the first line.
	* @throws CanNotOpenFileError if the file cannot be opened.
	*/
	CSVParser(const std::string& path, unsigned int column_count, char delimiter, bool skip_first_line = true);

	/**
	 * @brief Initializes the CSVParser with the specified input stream, column count, delimiter and whether to skip the first line.
	 * @param is_ptr The input stream.
	 * @param column_count count of columns in the file.
	 * @param delimiter the delimiter used in the file.
	 * @param skip_first_line Whether to skip the first line.
	 */
	CSVParser(std::istream* is_ptr, unsigned int column_count, char delimiter, bool skip_first_line = true) :
		_is_ptr(is_ptr), _delimiter(delimiter), _column_count(column_count) {
		if (skip_first_line) {
			string line;
			getline(*_is_ptr, line);

		}
	}

	/**
	 * @brief Gets a stream of rows.
	 * @return stream of rows.
	 */
	RowStream getRowStream();

private:
	std::ifstream _ifs;
	std::istream* _is_ptr;
	unsigned int _column_count;
	char _delimiter;
};

class Row {
	friend class CellStream;
public:
	Row() : _current_column_count(0), _splitted_row() {}

	using SplittedRow = ranges::split_view<ranges::ref_view<string>, ranges::single_view<char>>;
	Row(SplittedRow splitted_row) :
		_splitted_row(splitted_row) {
		_current_column_count = ranges::distance(splitted_row);
		_splitted_row;
	}

	void printRow() {
		for (auto&& cell : _splitted_row.value()) {
			std::cout << string_view(cell.begin(), cell.end()) << " delim ";
		}
		std::cout << std::endl;
	}

	string toString(char delimiter = ' ') {
		string result = "";
		bool first = true;
		for (auto&& cell : _splitted_row.value()) {
			if (!first) {
				result += delimiter;
			}
			else {
				first = false;
			}
			result += string_view(cell.begin(), cell.end());
		}

		return result;
	}

	size_t getColumnCount() {
		return _current_column_count;
	}

	CellStream getCellStream();

private:
	std::optional<ranges::split_view<ranges::ref_view<string>, ranges::single_view<char>>> _splitted_row;
	int _current_column_count;
};

template <typename T> concept arithmetic = std::is_arithmetic_v<T>;

class Cell {
public:
	Cell() = default;
	Cell(string_view sv) : _sv(sv) {}

	template <arithmetic T>
	T toNumberOrDefault(T default_value) {
		if (_sv.empty()) {
			return default_value;
		}

		auto start_ptr = _sv.data();
		auto end_ptr = _sv.data() + _sv.length();
		T value;
		auto res = std::from_chars(start_ptr, end_ptr, value);

		if (res.ec != std::errc() || res.ptr != end_ptr) {
			return default_value;
		}

		return value;
	}



	template <arithmetic T>
	bool toNumber(T& value) {
		if (_sv.empty()) {
			return false;
		}

		auto start_ptr = _sv.data();
		auto end_ptr = _sv.data() + _sv.length();
		auto res = std::from_chars(start_ptr, end_ptr, value);

		if (res.ec != std::errc() || res.ptr != end_ptr) {
			return false;
		}

		return true;
	}

	string toString() {
		return string(_sv.begin(), _sv.end());
	}

	string_view toView() {
		return _sv;
	}

	bool empty() {
		return _sv.empty();
	}
private:
	string_view _sv;
};

class CellStream {
public:
	CellStream(Row* row): _row(row) {
		auto&& splitted_row = _row->_splitted_row.value();
		_start_it = splitted_row.begin();
		_end_it = splitted_row.end();
	}

	/**
	 * @brief gets the next Cell.
	 * @return the next cell.
	 * @throws EndOfStreamException if the end of the stream is reached.
	 */
	Cell next() {
		if (!good()) {
			throw EndOfStreamException();
		}

		auto&& cell_range = *_start_it++;
		return Cell(string_view(cell_range.begin(), cell_range.end()));
	}

	CellStream& operator>>(Cell& cell) {
		cell = next();
		return *this;
	}


	bool good() const {
		return _start_it != _end_it;
	}

	operator bool() const {
		return good();
	}
private:
	size_t _index = 0;
	Row* _row;
	decltype(declval<Row::SplittedRow>().begin()) _start_it;
	decltype(declval<Row::SplittedRow>().end()) _end_it;
};

class RowStream {
public:
	RowStream(CSVParser* parser) : _parser(parser) {}

	//template<typename T>
	RowStream& operator>>(Row& row) {
		getline(*(_parser->_is_ptr), _current_line);

		auto split = ranges::views::split(_current_line, _parser->_delimiter);
		row = Row(split);
		return *this;
	}

	operator bool() const {
		return _parser->_is_ptr->good() && _parser->_is_ptr->peek() != '\n'; // True if there are more lines to process
	}

private:
	CSVParser* _parser;
	string _current_line;
};


CSVParser::CSVParser(const std::string& path, unsigned int column_count, char delimiter, bool skip_first_line) :
	_column_count(column_count), _delimiter(delimiter) {
	_is_ptr = &_ifs;
	_ifs.open(path);
	if (_ifs.fail()) {
		throw CanNotOpenFileError();
	}

	if (skip_first_line) {
		string line;
		getline(*_is_ptr, line);
	}
}

RowStream CSVParser::getRowStream() {
	return RowStream(this);
}

CellStream Row::getCellStream() {
	return CellStream(this);
}

}