#include <gtest/gtest.h>
#include <iostream>

import AlgoTrading;
import Backtesting;

class BarsCalculationTest : public ::testing::Test {
};

struct TestCase {
	Timeframe timeframe;
	size_t expectedCount;
};
class BarsCalculationTest2 : public ::testing::TestWithParam<TestCase> {
};


void printBar(const Bar& bar) {
	//std::cout << "Time: " << bar.time << std::endl;
	std::time_t now_c = std::chrono::system_clock::to_time_t(bar.open_timestamp);
	std::cout << "=========BAR=========" << std::endl;
	std::cout << "   Time: " << std::ctime(&now_c);
	std::cout << "   Open: " << bar.open << std::endl;
	std::cout << "   High: " << bar.high << std::endl;
	std::cout << "   Low: " << bar.low << std::endl;
	std::cout << "   Close: " << bar.close << std::endl;
	std::cout << "   Volume: " << bar.tick_volume << std::endl;
}



TEST_P(BarsCalculationTest2, CalculateBars) {
	auto [timeframe, expectedCount] = GetParam();
	auto now = std::chrono::system_clock::now();
	Ticks ticks;
	for (size_t i = 0; i < 20; i++)
	{
		Tick tick{ now + std::chrono::seconds(30 * i), 1.0, 2.0, 1.0, ChangeFlag::ASK_AND_BID };
		ticks.push_back(tick);
	}

	Bars result = calculateBars(timeframe, ticks);
	for (const auto& bar : result) {
		printBar(bar);
	}

	EXPECT_EQ(result.size(), expectedCount);
}

INSTANTIATE_TEST_SUITE_P(
	BarsCalculationTests,
	BarsCalculationTest2,
	::testing::Values(
		TestCase{ Timeframe::MIN1, 10 },
		TestCase{ Timeframe::MIN5, 2 },
		TestCase{ Timeframe::MIN15, 1 }
	)
);

TEST_F(BarsCalculationTest, EmptyTicksReturnsEmptyBars) {
    Ticks emptyTicks;
    Timeframe timeframe = Timeframe::MIN1;

    Bars result = calculateBars(timeframe, emptyTicks);
    EXPECT_TRUE(result.empty());
}


