// Copyright 2020 Junekey Jeon
//
// The contents of this file may be used under the terms of
// the Apache License v2.0 with LLVM Exceptions.
//
//    (See accompanying file LICENSE-Apache or copy at
//     https://llvm.org/foundation/relicensing/LICENSE.txt)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

#include "minmax_euclid.h"
#include <iostream>

int main()
{
	using jkj::fp::detail::bigint;
	using jkj::fp::detail::minmax_euclid;

	bool success = true;

	std::cout << "[Min-max Euclid algorithm test]\n";

	{
		bigint<64> a = 3;
		bigint<64> b = 7;
		bigint<64> N = 3;
		auto ret = minmax_euclid(a, b, N);
		if (ret.min != 2 || ret.max != 6 ||
			ret.argmin != 3 || ret.argmax != 2)
		{
			std::cout << "Test failed! (a = 3, b = 7, N = 3)\n";
			success = false;
		}
	}

	{
		bigint<64> a = 16;
		bigint<64> b = 6;
		bigint<64> N = 3;
		auto ret = minmax_euclid(a, b, N);
		if (ret.min != 0 || ret.max != 4 ||
			ret.argmin != 3 || ret.argmax != 1)
		{
			std::cout << "Test failed! (a = 16, b = 6, N = 4)\n";
			success = false;
		}
	}

	{
		bigint<64> a = 1234567;
		bigint<64> b = 1234567;
		bigint<64> N = 123456789;
		auto ret = minmax_euclid(a, b, N);
		if (ret.min != 0 || ret.max != 0)
		{
			std::cout << "Test failed! (a = 1234567, b = 1234567, N = 123456789)\n";
			success = false;
		}
	}

	{
		bigint<64> a = 13;
		bigint<64> b = 69;
		bigint<64> N = 40;
		auto ret = minmax_euclid(a, b, N);
		if (ret.min != 1 || ret.max != 67 ||
			ret.argmin != 16 || ret.argmax != 37)
		{
			std::cout << "Test failed! (a = 13, b = 69, N = 40)\n";
			success = false;
		}
	}

	std::cout << std::endl;
	std::cout << "Done.\n\n\n";

	if (!success) {
		return -1;
	}
}