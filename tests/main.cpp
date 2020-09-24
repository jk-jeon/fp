#include "jkj/fp/ryu_printf.h"

namespace jkj::fp {
	namespace detail {
		extern void verify_log_computation();

		namespace ryu_printf {
			extern void generate_cache();
		}
	}
}

int count_decimal_digits(std::uint32_t n) {
	if (n >= 1'0000'0000) return 9;
	if (n >= 1'0000'000) return 8;
	if (n >= 1'0000'00) return 7;
	if (n >= 1'0000'0) return 6;
	if (n >= 1'0000) return 5;
	if (n >= 1'000) return 4;
	if (n >= 1'00) return 3;
	if (n >= 1'0) return 2;
	return 1;
}

#include <iostream>
#include <iomanip>

int main()
{
	using namespace jkj::fp::detail;
	//ryu_printf::generate_cache();
	//verify_log_computation();

	while (true) {
		float x;
		std::cin >> x;

		// Computes the first segment on construction
		// x should not be a nonzero finite number
		jkj::fp::ryu_printf rp{ x };

		int c = count_decimal_digits(rp.current_segment());
		int decimal_exponent = c - rp.current_segment_index() * 9 - 1;
		std::cout << "10^" << decimal_exponent << " * ";

		int first_digit_divisors[] = { 1, 10, 100, 1000, 10000, 100000,
			1000000, 10000000, 100000000 };

		auto q = rp.current_segment() / first_digit_divisors[c - 1];
		auto r = rp.current_segment() % first_digit_divisors[c - 1];

		if (c > 1) {
			std::cout << q << "." << std::setw(c - 1) << std::setfill('0') << r;
		}
		else {
			std::cout << q << ".";
		}

		while (rp.has_further_nonzero_segments()) {
			std::cout << std::setw(9) << std::setfill('0') << rp.compute_next_segment();
		}

		std::cout << std::endl;
	}
}