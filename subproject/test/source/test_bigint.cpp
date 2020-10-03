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

#include "bigint.h"
#include <iostream>

int main()
{
	using jkj::fp::detail::bigint;

	bool success = true;

	std::cout << "[Big integer test]\n";

	// log2p1
	{
		if (jkj::fp::detail::log2p1(0x003f'3ebc'1040'9782) != 54)
		{
			std::cout << "jkj::fp::detail::log2p1 has failed.\n";
			success = false;
		}
		if (log2p1(bigint<256>::power_of_2(250)) != 251 ||
			log2p1(bigint<1024>{ 0 }) != 0)
		{
			std::cout << "jkj::fp::detail::log2p1 has failed.\n";
			success = false;
		}
	}

	// Computation of array_size
	{
		if (bigint<176>::array_size != 3 || bigint<1066>::array_size != 17)
		{
			std::cout << "Array-size computation has failed.\n";
			success = false;
		}
	}

	// power_of_5
	{
		auto five_to_the_178 = bigint<500>::power_of_5(178);
		if (five_to_the_178.leading_one_pos.element_pos != 6 ||
			five_to_the_178.leading_one_pos.bit_pos != 30 ||
			five_to_the_178.elements[6] != 0x0000'0000'277b'efc0 ||
			five_to_the_178.elements[5] != 0x6c18'6b6a'ce82'2204 ||
			five_to_the_178.elements[4] != 0xdabe'9929'c4d8'2a83 ||
			five_to_the_178.elements[3] != 0xd133'4193'f0d4'07ee ||
			five_to_the_178.elements[2] != 0xc235'0a2a'c8c8'5f25 ||
			five_to_the_178.elements[1] != 0x0789'4115'14b6'66c0 ||
			five_to_the_178.elements[0] != 0x1192'6668'4c87'fb59)
		{
			std::cout << "power_of_5 computation has failed.\n";
			success = false;
		}
	}

	// power_of_2
	{
		auto two_to_the_2645 = bigint<2646>::power_of_2(2645);
		if (two_to_the_2645.leading_one_pos.element_pos != 41 ||
			two_to_the_2645.leading_one_pos.bit_pos != 22 ||
			two_to_the_2645.elements[41] != (std::uint64_t(1) << 21))
		{
			std::cout << "power_of_2 computation has failed.\n";
			success = false;
		}
		for (std::size_t i = 0; i < two_to_the_2645.array_size; ++i) {
			if (i != two_to_the_2645.leading_one_pos.element_pos &&
				two_to_the_2645.elements[i] != 0)
			{
				std::cout << "power_of_2 computation has failed.\n";
				success = false;
				break;
			}
		}
	}

	// multiply_2_until
	{
		bigint<120> a;
		a.leading_one_pos.element_pos = 1;
		a.leading_one_pos.bit_pos = 37;
		a.elements[0] = 0x00eb'8e49'432a'32cb;
		a.elements[1] = 0x19'766c'e413;

		bigint<120> b;
		b.leading_one_pos.element_pos = 1;
		b.leading_one_pos.bit_pos = 7;
		b.elements[0] = 0x6755'1281'12da'3953;
		b.elements[1] = 0x57;

		auto r = b.multiply_2_until(a);
		if (r != 31 ||
			b.leading_one_pos.element_pos != 1 ||
			b.leading_one_pos.bit_pos != 38 ||
			b.elements[1] != 0x2b'b3aa'8940 ||
			b.elements[0] != 0x896d'1ca9'8000'0000)
		{
			std::cout << "multiply_2_until computation has failed.\n";
			success = false;
		}
	}

	// multiply_5
	{
		bigint<120> a;
		a.leading_one_pos.element_pos = 1;
		a.leading_one_pos.bit_pos = 37;
		a.elements[0] = 0x00eb'8e49'432a'32cb;
		a.elements[1] = 0x19'766c'e413;

		a.multiply_5();
		if (a.leading_one_pos.element_pos != 1 ||
			a.leading_one_pos.bit_pos != 39 ||
			a.elements[1] != 0x7f'5020'745f ||
			a.elements[0] != 0x0499'c76e'4fd2'fdf7)
		{
			std::cout << "multiply_5 computation has failed.\n";
			success = false;
		}
	}

	// multiply_2
	{
		bigint<128> a;
		a.leading_one_pos.element_pos = 1;
		a.leading_one_pos.bit_pos = 62;
		a.elements[0] = 0x3333'3333'3333'3333;
		a.elements[1] = 0x3333'3333'3333'3333;

		a.multiply_2();
		if (a.leading_one_pos.element_pos != 1 ||
			a.leading_one_pos.bit_pos != 63 ||
			a.elements[1] != 0x6666'6666'6666'6666 ||
			a.elements[0] != 0x6666'6666'6666'6666)
		{
			std::cout << "multiply_2 computation has failed.\n";
			success = false;
		}
		a.multiply_2();
		if (a.leading_one_pos.element_pos != 1 ||
			a.leading_one_pos.bit_pos != 64 ||
			a.elements[1] != 0xcccc'cccc'cccc'cccc ||
			a.elements[0] != 0xcccc'cccc'cccc'cccc)
		{
			std::cout << "multiply_2 computation has failed.\n";
			success = false;
		}
	}

	// Comparisons
	{
		bigint<256> a;
		a.leading_one_pos.element_pos = 1;
		a.leading_one_pos.bit_pos = 64;
		a.elements[1] = 0x8123'4567'89ab'cdef;
		a.elements[0] = 0x1234'5678'9abc'def0;

		bigint<256> b;
		b.leading_one_pos.element_pos = 1;
		b.leading_one_pos.bit_pos = 61;
		b.elements[1] = 0x1234'5678'9abc'def0;
		b.elements[0] = 0x8123'4567'89ab'cdef;

		bigint<256>::element_type c = 0x4567'89ab'cdef'1234;

		if (!(a > b) ||
			!(a >= b) ||
			!(a >= a) ||
			!(b < a) ||
			!(b <= a) ||
			!(b <= b) ||
			!(a == a) ||
			!(a != b) ||
			!(a > c) ||
			!(a >= c) ||
			(b < c) ||
			(b <= c) ||
			(c > a) ||
			(c >= a) ||
			!(c < b) ||
			!(c <= b) ||
			(a == c) ||
			!(b != c) ||
			(b == c) ||
			!(a != c))
		{
			std::cout << "one of the comparisons has failed.\n";
			success = false;
		}
	}

	// Addition
	{
		bigint<256> a;
		a.leading_one_pos.element_pos = 2;
		a.leading_one_pos.bit_pos = 60;
		a.elements[3] = 0;
		a.elements[2] = 0x0f12'eefc'bcde'1523;
		a.elements[1] = 0x0f12'eefc'bcde'1523;
		a.elements[0] = 0x0f12'eefc'bcde'1523;

		auto b = a + 0xffff'eeee'dddd'cccc;
		if (b.leading_one_pos.element_pos != 2 ||
			b.leading_one_pos.bit_pos != 60 ||
			b.elements[3] != 0 ||
			b.elements[2] != 0x0f12'eefc'bcde'1523 ||
			b.elements[1] != 0x0f12'eefc'bcde'1524 ||
			b.elements[0] != 0x0f12'ddeb'9abb'e1ef)
		{
			std::cout << "Addition has failed.\n";
			success = false;
		}

		a += b;
		if (a.leading_one_pos.element_pos != 2 ||
			a.leading_one_pos.bit_pos != 61 ||
			a.elements[3] != 0 ||
			a.elements[2] != 0x1e25'ddf9'79bc'2a46 ||
			a.elements[1] != 0x1e25'ddf9'79bc'2a47 ||
			a.elements[0] != 0x1e25'cce8'5799'f712)
		{
			std::cout << "Addition has failed.\n";
			success = false;
		}

		b = a + a;
		if (b.leading_one_pos.element_pos != 2 ||
			b.leading_one_pos.bit_pos != 62 ||
			b.elements[3] != 0 ||
			b.elements[2] != 0x3c4b'bbf2'f378'548c ||
			b.elements[1] != 0x3c4b'bbf2'f378'548e ||
			b.elements[0] != 0x3c4b'99d0'af33'ee24)
		{
			std::cout << "Addition has failed.\n";
			success = false;
		}

		b += b;
		if (b.leading_one_pos.element_pos != 2 ||
			b.leading_one_pos.bit_pos != 63 ||
			b.elements[3] != 0 ||
			b.elements[2] != 0x7897'77e5'e6f0'a918 ||
			b.elements[1] != 0x7897'77e5'e6f0'a91c ||
			b.elements[0] != 0x7897'33a1'5e67'dc48)
		{
			std::cout << "Addition has failed.\n";
			success = false;
		}

		b = b + b;
		if (b.leading_one_pos.element_pos != 2 ||
			b.leading_one_pos.bit_pos != 64 ||
			b.elements[3] != 0 ||
			b.elements[2] != 0xf12e'efcb'cde1'5230 ||
			b.elements[1] != 0xf12e'efcb'cde1'5238 ||
			b.elements[0] != 0xf12e'6742'bccf'b890)
		{
			std::cout << "Addition has failed.\n";
			success = false;
		}

		b = (b += b);
		if (b.leading_one_pos.element_pos != 3 ||
			b.leading_one_pos.bit_pos != 1 ||
			b.elements[3] != 1 ||
			b.elements[2] != 0xe25d'df97'9bc2'a461 ||
			b.elements[1] != 0xe25d'df97'9bc2'a471 ||
			b.elements[0] != 0xe25c'ce85'799f'7120)
		{
			std::cout << "Addition has failed.\n";
			success = false;
		}

		++b;
		if (b.leading_one_pos.element_pos != 3 ||
			b.leading_one_pos.bit_pos != 1 ||
			b.elements[3] != 1 ||
			b.elements[2] != 0xe25d'df97'9bc2'a461 ||
			b.elements[1] != 0xe25d'df97'9bc2'a471 ||
			b.elements[0] != 0xe25c'ce85'799f'7121)
		{
			std::cout << "Addition has failed.\n";
			success = false;
		}
	}

	// Subtraction
	{
		bigint<128> a;
		a.leading_one_pos.element_pos = 1;
		a.leading_one_pos.bit_pos = 61;
		a.elements[1] = 0x1111'0000'1111'0000;
		a.elements[0] = 0x0000'1111'0000'1111;

		bigint<128> b;
		b.leading_one_pos.element_pos = 1;
		b.leading_one_pos.bit_pos = 29;
		b.elements[1] = 0x0000'0000'1111'0000;
		b.elements[0] = 0x0000'1111'0000'1111;

		auto c = a - b;
		if (c.leading_one_pos.element_pos != 1 ||
			c.leading_one_pos.bit_pos != 61 ||
			c.elements[1] != 0x1111'0000'0000'0000 ||
			c.elements[0] != 0x0000'0000'0000'0000)
		{
			std::cout << "Subtraction has failed.\n";
			success = false;
		}

		a -= c;
		if (a != b)
		{
			std::cout << "Subtraction has failed.\n";
			success = false;
		}

		b -= b;
		if (!b.is_zero())
		{
			std::cout << "Subtraction has failed.\n";
			success = false;
		}

		--c;
		if (c.leading_one_pos.element_pos != 1 ||
			c.leading_one_pos.bit_pos != 61 ||
			c.elements[1] != 0x1110'ffff'ffff'ffff ||
			c.elements[0] != 0xffff'ffff'ffff'ffff)
		{
			std::cout << "Subtraction has failed.\n";
			success = false;
		}

		c -= 0x0000'0000'ffff'ffff;
		if (c.leading_one_pos.element_pos != 1 ||
			c.leading_one_pos.bit_pos != 61 ||
			c.elements[1] != 0x1110'ffff'ffff'ffff ||
			c.elements[0] != 0xffff'ffff'0000'0000)
		{
			std::cout << "Subtraction has failed.\n";
			success = false;
		}

		a = c - 0xffff'ffff'0000'0001;
		if (a.leading_one_pos.element_pos != 1 ||
			a.leading_one_pos.bit_pos != 61 ||
			a.elements[1] != 0x1110'ffff'ffff'fffe ||
			a.elements[0] != 0xffff'ffff'ffff'ffff)
		{
			std::cout << "Subtraction has failed.\n";
			success = false;
		}
	}

	// Multiplication
	{
		bigint<256> a = 0xffff'ffff'ffff'ffff;
		a *= 0xffff'ffff'ffff'ffff;

		if (a.leading_one_pos.element_pos != 1 ||
			a.leading_one_pos.bit_pos != 64 ||
			a.elements[1] != 0xffff'ffff'ffff'fffe ||
			a.elements[0] != 0x0000'0000'0000'0001)
		{
			std::cout << "Multiplication has failed.\n";
			success = false;
		}

		a *= a;
		if (a.leading_one_pos.element_pos != 3 ||
			a.leading_one_pos.bit_pos != 64 ||
			a.elements[3] != 0xffff'ffff'ffff'fffc ||
			a.elements[2] != 0x0000'0000'0000'0005 ||
			a.elements[1] != 0xffff'ffff'ffff'fffc ||
			a.elements[0] != 0x0000'0000'0000'0001)
		{
			std::cout << "Multiplication has failed.\n";
			success = false;
		}
	}

	// Division
	{
		bigint<256> a;
		a.leading_one_pos.element_pos = 3;
		a.leading_one_pos.bit_pos = 64;
		a.elements[3] = 0xfedc'ba98'7654'3210;
		a.elements[2] = 0xfedc'ba98'7654'3210;
		a.elements[1] = 0xfedc'ba98'7654'3210;
		a.elements[0] = 0xfedc'ba98'7654'3210;

		bigint<256> b;
		b.leading_one_pos.element_pos = 2;
		b.leading_one_pos.bit_pos = 32;
		b.elements[3] = 0;
		b.elements[2] = 0x0000'0000'8765'4321;
		b.elements[1] = 0x1234'5678'1234'5678;
		b.elements[0] = 0x1234'5678'1234'5678;

		auto q = a.long_division(b);
		if (q.leading_one_pos.element_pos != 1 ||
			q.leading_one_pos.bit_pos != 33 ||
			q.elements[3] != 0 ||
			q.elements[2] != 0 ||
			q.elements[1] != 0x1'e1e1'e1e1 ||
			q.elements[0] != 0x9d0a'c1a0'fed3'2f62)
		{
			std::cout << "Division has failed.\n";
			success = false;
		}
		if (a.leading_one_pos.element_pos != 2 ||
			a.leading_one_pos.bit_pos != 31 ||
			a.elements[3] != 0 ||
			a.elements[2] != 0x0000'0000'726e'404a ||
			a.elements[1] != 0x906f'6884'2a2b'4684 ||
			a.elements[0] != 0x3556'235b'8d83'1020)
		{
			std::cout << "Division has failed.\n";
			success = false;
		}
	}

	// Bit shifts
	{
		bigint<256> a = 0x1234'5678'1234'5678;

		auto b = a << 160;
		if (b.leading_one_pos.element_pos != 3 ||
			b.leading_one_pos.bit_pos != 29 ||
			b.elements[3] != 0x0000'0000'1234'5678 ||
			b.elements[2] != 0x1234'5678'0000'0000 ||
			b.elements[1] != 0 ||
			b.elements[0] != 0)
		{
			std::cout << "Left-shift has failed.\n";
			success = false;
		}

		b >>= 176;
		if (b.leading_one_pos.element_pos != 0 ||
			b.leading_one_pos.bit_pos != 45 ||
			b.elements[3] != 0 ||
			b.elements[2] != 0 ||
			b.elements[1] != 0 ||
			b.elements[0] != 0x0000'1234'5678'1234)
		{
			std::cout << "Right-shift has failed.\n";
			success = false;
		}
	}

	// lower_bits_of
	{
		bigint<256> a;
		a.leading_one_pos.element_pos = 3;
		a.leading_one_pos.bit_pos = 60;
		a.elements[3] = 0x0000'ffff'0000'ffff;
		a.elements[2] = 0xaaaa'0000'aaaa'0000;
		a.elements[1] = 0xcd00'cd00'cd00'cd00;
		a.elements[0] = 0x1234'5678'1234'5678;

		auto b = lower_bits_of(a, 128 + 16 + 10);
		if (b.leading_one_pos.element_pos != 2 ||
			b.leading_one_pos.bit_pos != 26 ||
			b.elements[3] != 0 ||
			b.elements[2] != 0x0000'0000'02aa'0000 ||
			b.elements[1] != 0xcd00'cd00'cd00'cd00 ||
			b.elements[0] != 0x1234'5678'1234'5678)
		{
			std::cout << "lower_bits_of computation has failed.\n";
			success = false;
		}
	}

	// factor_out_2
	{
		bigint<256> a;
		a.leading_one_pos.element_pos = 3;
		a.leading_one_pos.bit_pos = 60;
		a.elements[3] = 0x0000'ffff'0000'ffff;
		a.elements[2] = 0xaaaa'0000'aaaa'0000;
		a.elements[1] = 0;
		a.elements[0] = 0;

		auto r = a.factor_out_2();
		if (r != 128 + 16 + 1 ||
			a.leading_one_pos.element_pos != 1 ||
			a.leading_one_pos.bit_pos != 43 ||
			a.elements[3] != 0 ||
			a.elements[2] != 0 ||
			a.elements[1] != 0x0000'0000'7fff'8000 ||
			a.elements[0] != 0x7fff'd555'0000'5555)
		{
			std::cout << "factor_out_2 computation has failed.\n";
			success = false;
		}
	}

	// count_factor_of_2
	{
		bigint<256> a;
		a.leading_one_pos.element_pos = 3;
		a.leading_one_pos.bit_pos = 60;
		a.elements[3] = 0x0000'ffff'0000'ffff;
		a.elements[2] = 0xaaaa'0000'aaaa'0000;
		a.elements[1] = 0;
		a.elements[0] = 0;

		auto r = a.count_factor_of_2();
		if (r != 128 + 16 + 1)
		{
			std::cout << "count_factor_of_2 computation has failed.\n";
			success = false;
		}
	}

	std::cout << std::endl;
	std::cout << "Done.\n\n\n";

	if (!success) {
		return -1;
	}
}