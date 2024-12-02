#include <gtest/gtest.h>
#include <vector>
#include <ranges>
#include <iostream>

struct bar {

};

//template <typename T>
//auto vectorSlice(const std::vector<T>& data, int start, int end) -> std::ranges::subrange<const T*> {
//    if (start < 0 || start >= data.size() || end < 0 || end > data.size() || start > end) {
//        throw std::out_of_range("Invalid indices");
//    }
//
//    return data | std::views::drop(start) | std::views::take(end - start);
//}


int add(int a, int b) {
    //auto subrange_view  = vectorSlice(v, startIndex, endIndex);
    /*for (int i : subrange_view) {
		std::cout << i << std::endl;
	}*/
    return a + b;
}

TEST(MathFunctionsTest, AddTwoNumbers) {
    EXPECT_EQ(add(2, 3), 5);
}