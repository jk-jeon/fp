#include "jkj/fp/to_chars/fixed_precision.h"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <string_view>
#include <stdexcept>
#include <memory>
#include <vector>

#include "ryu/ryu.h"
#define FMT_HEADER_ONLY 1
#include "fmt/format.h"

int main()
{
	using namespace jkj::fp::detail;

	while (true) {
		double x;
		int precision;
		while (!(std::cin >> x))
		{
			std::cin.clear();
			while (std::cin.get() != '\n')
				continue;
			std::cout << "enter a new input: ";
		}
		while (!(std::cin >> precision))
		{
			std::cin.clear();
			while (std::cin.get() != '\n')
				continue;
			std::cout << "enter a new input: ";
		}

		char buffer[2048];
		std::string_view str{ buffer,
			std::size_t(jkj::fp::to_chars_fixed_precision_scientific(x, buffer, precision) - buffer) };

		std::cout << str << std::endl;
		std::cout << std::scientific << std::setprecision(precision) << x << std::endl;
	}
}