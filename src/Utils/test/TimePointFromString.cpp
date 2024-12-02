#include <gtest/gtest.h>
#include <iostream>
#include <chrono>
#include <sstream>
#include <string>

using namespace std;

import Utils;
using namespace utils;

class CellConvertingTest : public ::testing::Test {
};

TEST_F(CellConvertingTest, ToNumberSuccess) {
    std::string string = "123.456";
    Cell cell(string);
    double number;
    EXPECT_TRUE(cell.toNumber(number));

    EXPECT_EQ(number, 123.456);
}

//TEST_F(CellConvertingTest, ToNumberEmptyString) {
//    std::string string = "123.456";
//    Cell cell(string);
//    double number;
//    //EXPECT_FALSE(cell.toNumber(number));
//}

std::chrono::system_clock::time_point convertToTime(istream& is) {
    static string format = "%Y-%m-%d %H:%M:%S";
    std::chrono::system_clock::time_point tp;
    is >> std::chrono::parse(format.c_str(), tp);
    return tp;
}

size_t converting() {
    std::string datetime_str = "2024-04-08 21:46:46.211";
    std::istringstream iss(datetime_str);
	std::chrono::system_clock::time_point tp = convertToTime(iss);
    size_t count_since_epoch = tp.time_since_epoch().count();

    if (iss.fail()) {
        return 0;
    }
    
	return count_since_epoch;
}


TEST(DateParsingTest, AddTwoNumbers) {
    EXPECT_EQ(converting(), 17126128062110000);
}

int testDuration() {
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point tp2 = tp + std::chrono::hours(1);
	std::chrono::duration<double> diff = tp2 - tp;
	return diff.count();
}

TEST(DurationTest, testDuration) {
	EXPECT_EQ(testDuration(), 3600);
}

size_t testMaxMilisecondsValue() {
    return std::chrono::milliseconds::max().count();
}

TEST(Miliseconds, testMaxMilisecondsValue) {
    EXPECT_EQ(testMaxMilisecondsValue(), 9223372036854775807);

}

