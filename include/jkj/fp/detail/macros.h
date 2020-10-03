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

////////////////////////////////////////////////////////////
//////////// !!!! This header leaks macros !!!! ////////////
////////////////////////////////////////////////////////////

// This header MUST be included AFTER all the other fp headers are included,
// because some of the fp headers include undef_macros.h, which
// undef's all the macros exported by this header.

#ifndef JKJ_HEADER_FP_MACROS
#define JKJ_HEADER_FP_MACROS

#if defined(__GNUC__) || defined(__clang__)
#define JKJ_SAFEBUFFERS
#define JKJ_FORCEINLINE __attribute__((always_inline))
#define JKJ_EMPTY_BASE
#elif defined(_MSC_VER)
#define JKJ_SAFEBUFFERS __declspec(safebuffers)	// Suppresses needless buffer overrun check.
#define JKJ_FORCEINLINE __forceinline
#define JKJ_EMPTY_BASE __declspec(empty_bases)
#else
#define JKJ_SAFEBUFFERS
#define JKJ_FORCEINLINE inline
#define JKJ_EMPTY_BASE
#endif

#endif