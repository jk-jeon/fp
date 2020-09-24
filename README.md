# `fp` Library
`fp` is a binary-to-decimal and decimal-to-binary conversion library for IEEE-754 floating-point numbers.

It aims to be a building block for higher-level libraries with high-performance parsing/formatting procedures between human-readable strings and IEEE-754-encoded floating-point numbers.

## Goal
`fp` itself does not aim to have fastest string-to-float/float-to-string conversions with the greatest flexibility, because that is a too demanding task. Rather, `fp` focuses on fast mathematical algorithms for converting between binary IEEE-754 floating-point numbers and thier decimal floating-point representations. Implementers of string-to-float/float-to-string procedures then can utilize those algorithms to implement actual functions for parsing/formatting with their own constraints and goals.

# Included Algorithms
Here is the list of algorithms that `fp` supports (or will support in a future).

## Dragonbox
Shortest-roundtrip binary-to-decimal conversion algorithm. To be prepared.

## Ryu-printf
Fixed-precision binary-to-decimal conversion algorithm. To be prepared.

## Dooly
Decimal-to-binary conversion algorithm with limited/unlimited precisions. To be prepared.

# Language Standard
The library is targetting C++17 and actively using its features (e.g., if constexpr).

# License
All code, except for those belong to third-party libraries, is licensed under either of

 * Apache License Version 2.0 with LLVM Exceptions ([LICENSE-Apache2-LLVM](LICENSE-Apache2-LLVM) or https://llvm.org/foundation/relicensing/LICENSE.txt) or
 * Boost Software License Version 1.0 ([LICENSE-Boost](LICENSE-Boost) or https://www.boost.org/LICENSE_1_0.txt).
