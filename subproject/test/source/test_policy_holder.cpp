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

#include "jkj/fp/policy.h"
#include <iostream>
#include <type_traits>

namespace test_policy_kind {
	struct a {};
	struct b {};
	struct c {};
	struct d {};
}

namespace a {
	struct policy1 {
		using policy_kind = test_policy_kind::a;
		int x;
		auto get_a() { return x; }
	};

	struct policy2 {
		using policy_kind = test_policy_kind::a;
		float x;
		auto get_a() { return x; }
	};
}

namespace b {
	struct policy1 {
		using policy_kind = test_policy_kind::b;
		int x;
		auto get_b() { return x; }
	};

	struct policy2 {
		using policy_kind = test_policy_kind::b;
		float x;
		auto get_b() { return x; }
	};
}

namespace c {
	struct policy1 {
		using policy_kind = test_policy_kind::c;
		int x;
		auto get_c() { return x; }
	};

	struct policy2 {
		using policy_kind = test_policy_kind::c;
		float x;
		auto get_c() { return x; }
	};

	struct policy3 {
		using policy_kind = test_policy_kind::c;
		auto get_c() { return 1.2345; }
	};
}

namespace d {
	struct policy1 {
		using policy_kind = test_policy_kind::d;
	};
}

int main()
{
	using namespace jkj::fp::detail::policy;
	bool success = true;

	std::cout << "[Policy holder test]\n";

	{
		auto policy_holder = make_policy_holder(
			make_default_list(
				make_default<test_policy_kind::a>(a::policy1{ 10 }),
				make_default_generator<test_policy_kind::b>([]() { return b::policy2{ 0.5f }; }),
				make_default<test_policy_kind::c>(c::policy2{ 0.1f })
			),
			a::policy1{ 4 }
		);

		success &= (policy_holder.get_a() == 4);
		success &= (policy_holder.get_b() == 0.5f);
		success &= (policy_holder.get_c() == 0.1f);
	}

	{
		auto policy_holder = make_policy_holder(
			make_default_list(
				make_default<test_policy_kind::a>(a::policy1{ 10 }),
				make_default_generator<test_policy_kind::b>([]() { return b::policy2{ 0.5f }; }),
				make_default<test_policy_kind::c>(c::policy2{ 0.1f })
			),
			b::policy1{ 100 }, a::policy2{ 0.3f }
		);

		success &= (policy_holder.get_a() == 0.3f);
		success &= (policy_holder.get_b() == 100);
		success &= (policy_holder.get_c() == 0.1f);
	}

	{
		auto policy_holder = make_policy_holder(
			make_default_list(
				make_default<test_policy_kind::c>(c::policy2{ 0.6f }),
				make_default<test_policy_kind::d>(d::policy1{})
			),
			c::policy3{}
		);

		success &= std::is_empty_v<decltype(policy_holder)>;
	}

	std::cout << std::endl;
	std::cout << "Done.\n\n\n";

	if (!success) {
		return -1;
	}
}