#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <sstream>

import Utils;
using namespace utils;

static std::string csvTest = 
"<DATE>	<TIME>	<BID>	<ASK>	<LAST>	<VOLUME>	<FLAGS>\n\
2022.11.10	15:00:00.017	0.86682	0.86712			6\n\
2022.11.10	15:00:00.228	0.86681	0.86710			6\n\
2022.11.10	15:00:00.525	0.86677	0.86707			6\n\
2022.11.10	15:00:00.787	0.86679	0.86709			6\n\
2022.11.10	15:00:01.049	0.86682	0.86712			6\n\
2022.11.10	15:00:01.273	0.86680	0.86711			6\n\
2022.11.10	15:00:01.596	0.86682	0.86713			6\n";

size_t countRows() {
	std::istringstream iss(csvTest);
	CSVParser parser(&iss, 7, '\t', true);
	auto row_stream = parser.getRowStream();
	size_t row_count = 0;
	Row row;
	while (row_stream) {
		row_stream >> row;
		++row_count;
	}

	return row_count;
}

TEST(CSVParsingTests, CountRows) {
	EXPECT_EQ(countRows(), 8);
}