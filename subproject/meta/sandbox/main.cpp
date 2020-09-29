#include "jkj/fp/to_chars/fixed_precision.h"

namespace jkj::fp {
	namespace detail {
		extern void verify_log_computation();

		namespace ryu_printf {
			extern void generate_cache();
		}
	}
}

#include <iostream>
#include <iomanip>
#include <string_view>


#include "ryu/ryu.h"

int main()
{
	using namespace jkj::fp::detail;
	//ryu_printf::generate_cache();
	//verify_log_computation();

#if 0
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
#else
	// Benchmark
#endif
}