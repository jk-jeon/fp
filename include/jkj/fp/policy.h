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

#ifndef JKJ_HEADER_FP_POLICY
#define JKJ_HEADER_FP_POLICY

#include "decimal_fp.h"
#include "ieee754_format.h"
#include "detail/log.h"
#include "detail/dragonbox_cache.h"
#include "detail/util.h"
#include "detail/macros.h"

namespace jkj::fp {
	namespace detail {
		namespace policy {
			// The library will specify a list of accepted kinds of policies and their defaults,
			// and the user will pass a list of policies. The aim of helper classes/functions here
			// is to do the following:
			//   1. Check if the policy parameters given by the user are all valid; that means,
			//      each of them should be of the kinds specified by the library.
			//      If that's not the case, then the compilation fails.
			//   2. Check if multiple policy parameters for the same kind is specified by the user.
			//      If that's the case, then the compilation fails.
			//   3. Build a class deriving from all policies the user have given, and also from
			//      the default policies if the user did not specify one for some kinds.
			// A policy belongs to a certain kind if it is deriving from a base class.

			// Check if the given type is a policy of the given kind
			template <class Policy, class Kind, class = std::void_t<>>
			struct is_policy_of_kind : std::false_type {};

			template <class Policy, class Kind>
			struct is_policy_of_kind<Policy, Kind, std::void_t<typename Policy::policy_kind>> :
				std::integral_constant<bool, std::is_same_v<Kind, typename Policy::policy_kind>> {};

			template <class Policy, class Kind>
			static constexpr bool is_policy_of_kind_v = is_policy_of_kind<Policy, Kind>::value;

			// For a given kind, find a policy belonging to that kind.
			// Check if there are more than one such policies.
			enum class policy_found_info {
				not_found, unique, repeated
			};
			// This class records the reference to policy argument for the given
			// policy kind together with the enum to distinguish between:
			//   - there was no given policy for the policy kind;
			//     the class records the reference to a function object
			//     generating the default policy object (info == not_found).
			//   - there was a unique policy for the policy kind;
			//     the class records the reference to that unique policy (info == unique).
			//   - there were more than one policies for the policy kind;
			//     the class records the reference to one of those policies (info == repeated).
			template <class Policy, policy_found_info info>
			struct found_policy {
				static constexpr auto found_info = info;
				Policy&& policy;

				constexpr Policy&& get() {
					return static_cast<Policy&&>(policy);
				}
			};
			template <class DefaultPolicyGenerator>
			struct found_policy<DefaultPolicyGenerator, policy_found_info::not_found> {
				static constexpr auto found_info = policy_found_info::not_found;
				DefaultPolicyGenerator&& generator;

				constexpr decltype(auto) get() {
					return std::forward<DefaultPolicyGenerator>(generator)();
				}
			};
			// Passing a function object can be cumbersome, so
			// we might need to record the reference to an existing policy object instead.
			template <class DefaultPolicy>
			struct forward_generate_default {};
			template <class DefaultPolicy>
			struct found_policy<forward_generate_default<DefaultPolicy>, policy_found_info::not_found> {
				static constexpr auto found_info = policy_found_info::not_found;
				DefaultPolicy&& policy;

				constexpr DefaultPolicy&& get() {
					return static_cast<DefaultPolicy&&>(policy);
				}
			};

			// kind_default class records the reference to the default policy generator
			// (or to an existing policy object) for a given policy kind.
			// The member function get_policy finds policies of the given policy kind
			// from the list of given policies and returns a found_policy instance accordingly.
			template <class Kind, class DefaultPolicyGenerator>
			struct kind_default_base {
				template <class FoundPolicy>
				static constexpr FoundPolicy get_policy_impl(FoundPolicy&& fp) {
					return std::forward<FoundPolicy>(fp);
				}
				template <class FoundPolicy, class FirstPolicy, class... RemainingPolicies>
				static constexpr auto get_policy_impl(FoundPolicy&& fp,
					FirstPolicy&& first_policy, RemainingPolicies&&... remaining_policies)
				{
					// If we found a new policy of the given policy kind,
					if constexpr (is_policy_of_kind_v<remove_cvref_t<FirstPolicy>, Kind>)
					{
						// If this is the first instance,
						if constexpr (remove_cvref_t<FoundPolicy>::found_info
							== policy_found_info::not_found)
						{
							// We have a unique policy of the given policy kind.
							return get_policy_impl(
								found_policy<FirstPolicy, policy_found_info::unique>{
									std::forward<FirstPolicy>(first_policy)
								}, std::forward<RemainingPolicies>(remaining_policies)...);
						}
						// Otherwise,
						else {
							// We have multiple policies of the given policy kind.
							return get_policy_impl(
								found_policy<FirstPolicy, policy_found_info::repeated>{
									std::forward<FirstPolicy>(first_policy)
								}, std::forward<RemainingPolicies>(remaining_policies)...);
						}
					}
					// Otherwise,
					else {
						// Don't change the status.
						return get_policy_impl(std::forward<FoundPolicy>(fp),
							std::forward<RemainingPolicies>(remaining_policies)...);
					}
				}
			};

			template <class Kind, class DefaultPolicyGenerator>
			struct kind_default : kind_default_base<Kind, DefaultPolicyGenerator> {
				using policy_kind = Kind;
				DefaultPolicyGenerator&& generator;

				template <class... Policies>
				constexpr auto get_policy(Policies&&... policies) && {
					return kind_default_base<Kind, DefaultPolicyGenerator>::get_policy_impl(
						found_policy<DefaultPolicyGenerator, policy_found_info::not_found>{
							std::forward<DefaultPolicyGenerator>(generator)
						}, std::forward<Policies>(policies)...);
				}
			};

			template <class Kind, class DefaultPolicy>
			struct kind_default<Kind, forward_generate_default<DefaultPolicy>> :
				kind_default_base<Kind, forward_generate_default<DefaultPolicy>>
			{
				using policy_kind = Kind;
				DefaultPolicy&& policy;

				template <class... Policies>
				constexpr auto get_policy(Policies&&... policies) {
					return kind_default_base<Kind, forward_generate_default<DefaultPolicy>>::get_policy_impl(
						found_policy<forward_generate_default<DefaultPolicy>, policy_found_info::not_found>{
							std::forward<DefaultPolicy>(policy)
						}, std::forward<Policies>(policies)...);
				}
			};

			// Checks if a given policy belongs to one of the specified policy kinds.
			template <class Policy>
			constexpr bool check_policy_validity(typelist<Policy>, typelist<>)
			{
				return false;
			}
			template <class Policy, class FirstKindDefault, class... RemainingKindDefaults>
			constexpr bool check_policy_validity(typelist<Policy>,
				typelist<FirstKindDefault, RemainingKindDefaults...>)
			{
				return is_policy_of_kind_v<Policy, typename FirstKindDefault::policy_kind> ||
					check_policy_validity(typelist<Policy>{}, typelist<RemainingKindDefaults...>{});
			}

			// Checks if all the given policies are of specified policy kinds.
			template <class KindDefaultTypelist>
			constexpr bool check_policy_list_validity(typelist<>, KindDefaultTypelist) {
				return true;
			}
			template <class FirstPolicy, class... RemainingPolicies, class KindDefaultTypelist>
			constexpr bool check_policy_list_validity(
				typelist<FirstPolicy, RemainingPolicies...>, KindDefaultTypelist)
			{
				return check_policy_validity(typelist<FirstPolicy>{}, KindDefaultTypelist{}) &&
					check_policy_list_validity(typelist<RemainingPolicies...>{},
						KindDefaultTypelist{});
			}

			// This class aggregate all instances of found_policy.
			// It also records if there is any policy kind where multiple policies belong to.
			template <bool repeated_, class... FoundPolicies>
			struct found_policy_tuple : FoundPolicies... {
				static constexpr bool repeated = repeated_;
			};

			// This class aggregates all instances of kind_default.
			template <class... KindDefaults>
			struct kind_default_tuple : KindDefaults... {
				using kind_default_typelist = typelist<KindDefaults...>;
			};

			// Builds an instance of found_policy_tuple from the list of
			// policy kind & default pairs and the list of policies.
			// Note that the resulting object still holds only references.
			template <bool repeated, class... FoundPolicies, class... Policies>
			constexpr auto make_policy_holder_impl(
				kind_default_tuple<>&&,
				found_policy_tuple<repeated, FoundPolicies...>&& found,
				Policies&&...)
			{
				return std::move(found);
			}
			template <class FirstKindDefault, class... RemainingKindDefaults,
				bool repeated, class... FoundPolicies, class... Policies>
			constexpr auto make_policy_holder_impl(
				kind_default_tuple<FirstKindDefault, RemainingKindDefaults...>&& defaults,
				found_policy_tuple<repeated, FoundPolicies...>&& found,
				Policies&&... policies)
			{
				using new_found_policy_pair =
					decltype(static_cast<FirstKindDefault&&>(defaults).get_policy(std::forward<Policies>(policies)...));

				return make_policy_holder_impl(
					kind_default_tuple<RemainingKindDefaults...>{
						static_cast<RemainingKindDefaults&&>(defaults)...
					},
					found_policy_tuple<
						repeated || new_found_policy_pair::found_info == policy_found_info::repeated,
						new_found_policy_pair, FoundPolicies...
					>{
						static_cast<FirstKindDefault&&>(defaults).get_policy(std::forward<Policies>(policies)...),
						static_cast<FoundPolicies&&>(found)...
					}, std::forward<Policies>(policies)...);
			}

			// Our end-goal is to construct an instance of this class.
			template <class... Policies>
			struct JKJ_EMPTY_BASE policy_holder : Policies... {
				template <class... PolicyRefs>
				policy_holder(PolicyRefs&&... policies) : Policies{ std::forward<PolicyRefs>(policies) }... {}
			};

			// Convert an instance of found_policy_tuple into an instance of policy_holder,
			// which holds values rather than references. This is the place where
			// copy/move of policy parameters and/or invocation of default policy generator objects
			// might happen.
			template <bool repeated, class... RawPolicies>
			constexpr auto convert_to_policy_holder(
				found_policy_tuple<repeated>&&, RawPolicies&&... policies)
			{
				return policy_holder<remove_cvref_t<RawPolicies>...>{
					std::forward<RawPolicies>(policies)...
				};
			}
			template <bool repeated, class FirstFoundPolicy, class... RemainingFoundPolicies, class... RawPolicies>
			constexpr auto convert_to_policy_holder(
				found_policy_tuple<repeated, FirstFoundPolicy, RemainingFoundPolicies...>&& found,
				RawPolicies&&... policies)
			{
				return convert_to_policy_holder(
					found_policy_tuple<repeated, RemainingFoundPolicies...>{
						static_cast<RemainingFoundPolicies&&>(found)...
					},
					static_cast<FirstFoundPolicy&&>(found).get(),
					std::forward<RawPolicies>(policies)...);
			}

			// Builds an instance of policy_holder from the list of
			// policy kind & default pairs and the list of policies.
			template <class KindDefaultTuple, class... Policies>
			constexpr auto make_policy_holder(KindDefaultTuple&& defaults, Policies&&... policies)
			{
				// Ensure that all policies are of specified policy kinds.
				static_assert(check_policy_list_validity(
					typelist<remove_cvref_t<Policies>...>{},
					typename remove_cvref_t<KindDefaultTuple>::kind_default_typelist{}),
					"jkj::fp: an invalid policy is specified");

				// Ensure that for each policy kind, there is at most one policy
				// belonging to the kind.
				using found_policies = decltype(make_policy_holder_impl(
					std::forward<KindDefaultTuple>(defaults),
					found_policy_tuple<false>{},
					std::forward<Policies>(policies)...));
				static_assert(!found_policies::repeated,
					"jkj::fp: each policy kind should be specified at most once");

				// Call two of above functions to complete the job.
				return convert_to_policy_holder(make_policy_holder_impl(
					std::forward<KindDefaultTuple>(defaults),
					found_policy_tuple<false>{},
					std::forward<Policies>(policies)...));
			}

			// Makes an instance of kind_default
			// with a function object generating the default policy.
			template <class Kind, class DefaultGenerator>
			constexpr kind_default<Kind, DefaultGenerator>
				make_default_generator(DefaultGenerator&& generator)
			{
				return{ {}, std::forward<DefaultGenerator>(generator) };
			}

			// Makes an instance of kind_default
			// with an instance of the default policy object.
			template <class Kind, class DefaultPolicy>
			constexpr kind_default<Kind, forward_generate_default<DefaultPolicy>>
				make_default(DefaultPolicy&& policy)
			{
				return{ {}, std::forward<DefaultPolicy>(policy) };
			}

			// Aggregate all policy kind & default pairs.
			// Note that all kind_default object holds references, not values, thus
			// the return value of this function MUST NOT bind to an lvalue.
			template <class... KindDefaults>
			constexpr kind_default_tuple<KindDefaults...> make_default_list(KindDefaults&&... defaults) {
				return{ std::forward<KindDefaults>(defaults)... };
			}
		}
	}

	// We want the list of policy kinds to be open-ended, so
	// we use tag types rather than enums here
	namespace policy_kind {
		// Determines between shortest-roundtrip/fixed-precision output.
		struct precision {};

		// Determines between scientific/fixed-point/general output.
		struct output_format {};

		// Propagates or ignores the sign.
		struct sign {};

		// Determines what to do with trailing zeros in the output.
		struct trailing_zero {};

		// Determines the rounding mode of binary IEEE-754 encoded data.
		struct binary_rounding {};

		// Determines the rounding mode of decimal conversion.
		struct decimal_rounding {};

		// Determines which cache table to use.
		struct cache {};

		// Determines what to do with invalid inputs.
		struct input_validation {};
	}

	namespace detail {
		// Forward declare the implementation class
		namespace dragonbox {
			template <class Float>
			struct impl;
		}

		namespace policy {
			// Sign policy
			namespace sign {
				struct ignore {
					using policy_kind = policy_kind::sign;
					using sign_policy = ignore;
					static constexpr bool return_has_sign = false;

					template <class Float, class DecimalFp>
					static constexpr void binary_to_decimal(ieee754_bits<Float>, DecimalFp&) noexcept {}

					template <class Float, class DecimalFp>
					static constexpr void decimal_to_binary(DecimalFp, ieee754_bits<Float>&) noexcept {}
				};

				struct propagate {
					using policy_kind = policy_kind::sign;
					using sign_policy = propagate;
					static constexpr bool return_has_sign = true;

					template <class Float, class DecimalFp>
					static constexpr void binary_to_decimal(ieee754_bits<Float> br, DecimalFp& fp) noexcept {
						fp.is_negative = br.is_negative();
					}
					template <class Float, class DecimalFp>
					static constexpr void decimal_to_binary(DecimalFp fp, ieee754_bits<Float>& br) noexcept {
						using carrier_uint = typename ieee754_bits<Float>::carrier_uint;
						using format_info = ieee754_format_info<ieee754_traits<Float>::format>;

						if constexpr (DecimalFp::is_signed) {
							br.u |= (fp.is_negative ?
								(carrier_uint(1) << (format_info::significand_bits + format_info::exponent_bits)) : 0);
						}
					}
				};
			}

			// Trailing zero policy
			namespace trailing_zero {
				struct allow {
					using policy_kind = policy_kind::trailing_zero;
					using trailing_zero_policy = allow;
					static constexpr bool report_trailing_zeros = false;

					template <class DecimalFp>
					static constexpr void on_trailing_zeros(DecimalFp&) noexcept {}

					template <class DecimalFp>
					static constexpr void no_trailing_zeros(DecimalFp&) noexcept {}
				};

				struct remove {
					using policy_kind = policy_kind::trailing_zero;
					using trailing_zero_policy = remove;
					static constexpr bool report_trailing_zeros = false;

					template <class DecimalFp>
					static constexpr void on_trailing_zeros(DecimalFp& fp) noexcept {
						fp.exponent +=
							dragonbox::impl<typename DecimalFp::float_type>::remove_trailing_zeros(fp.significand);
					}

					template <class DecimalFp>
					static constexpr void no_trailing_zeros(DecimalFp&) noexcept {}
				};

				struct report {
					using policy_kind = policy_kind::trailing_zero;
					using trailing_zero_policy = report;
					static constexpr bool report_trailing_zeros = true;

					template <class DecimalFp>
					static constexpr void on_trailing_zeros(DecimalFp& fp) noexcept {
						fp.may_have_trailing_zeros = true;
					}

					template <class DecimalFp>
					static constexpr void no_trailing_zeros(DecimalFp& fp) noexcept {
						fp.may_have_trailing_zeros = false;
					}
				};
			}

			// Binary (input) rounding mode policy
			namespace binary_rounding {
				enum class tag_t {
					to_nearest,
					left_closed_directed,
					right_closed_directed
				};
				namespace interval_type {
					struct symmetric_boundary {
						static constexpr bool is_symmetric = true;
						bool is_closed;
						constexpr bool include_left_endpoint() const noexcept {
							return is_closed;
						}
						constexpr bool include_right_endpoint() const noexcept {
							return is_closed;
						}
					};
					struct asymmetric_boundary {
						static constexpr bool is_symmetric = false;
						bool is_left_closed;
						constexpr bool include_left_endpoint() const noexcept {
							return is_left_closed;
						}
						constexpr bool include_right_endpoint() const noexcept {
							return !is_left_closed;
						}
					};
					struct closed {
						static constexpr bool is_symmetric = true;
						static constexpr bool include_left_endpoint() noexcept {
							return true;
						}
						static constexpr bool include_right_endpoint() noexcept {
							return true;
						}
					};
					struct open {
						static constexpr bool is_symmetric = true;
						static constexpr bool include_left_endpoint() noexcept {
							return false;
						}
						static constexpr bool include_right_endpoint() noexcept {
							return false;
						}
					};
					struct left_closed_right_open {
						static constexpr bool is_symmetric = false;
						static constexpr bool include_left_endpoint() noexcept {
							return true;
						}
						static constexpr bool include_right_endpoint() noexcept {
							return false;
						}
					};
					struct right_closed_left_open {
						static constexpr bool is_symmetric = false;
						static constexpr bool include_left_endpoint() noexcept {
							return false;
						}
						static constexpr bool include_right_endpoint() noexcept {
							return true;
						}
					};
				}

				struct nearest_to_even {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_to_even;
					static constexpr auto tag = tag_t::to_nearest;

					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
						return f(nearest_to_even{});
					}

					template <class Float>
					static constexpr interval_type::symmetric_boundary
						interval_type_normal(ieee754_bits<Float> br) noexcept
					{
						return{ br.u % 2 == 0 };
					}
					template <class Float>
					static constexpr interval_type::closed
						interval_type_shorter(ieee754_bits<Float>) noexcept
					{
						return{};
					}
				};
				struct nearest_to_odd {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_to_odd;
					static constexpr auto tag = tag_t::to_nearest;

					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
						return f(nearest_to_odd{});
					}

					template <class Float>
					static constexpr interval_type::symmetric_boundary
						interval_type_normal(ieee754_bits<Float> br) noexcept
					{
						return{ br.u % 2 != 0 };
					}
					template <class Float>
					static constexpr interval_type::closed
						interval_type_shorter(ieee754_bits<Float>) noexcept
					{
						return{};
					}
				};
				struct nearest_toward_plus_infinity {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_toward_plus_infinity;
					static constexpr auto tag = tag_t::to_nearest;

					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
						return f(nearest_toward_plus_infinity{});
					}

					template <class Float>
					static constexpr interval_type::asymmetric_boundary
						interval_type_normal(ieee754_bits<Float> br) noexcept
					{
						return{ !br.is_negative() };
					}
					template <class Float>
					static constexpr interval_type::asymmetric_boundary
						interval_type_shorter(ieee754_bits<Float> br) noexcept
					{
						return{ !br.is_negative() };
					}
				};
				struct nearest_toward_minus_infinity {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_toward_minus_infinity;
					static constexpr auto tag = tag_t::to_nearest;

					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
						return f(nearest_toward_minus_infinity{});
					}

					template <class Float>
					static constexpr interval_type::asymmetric_boundary
						interval_type_normal(ieee754_bits<Float> br) noexcept
					{
						return{ br.is_negative() };
					}
					template <class Float>
					static constexpr interval_type::asymmetric_boundary
						interval_type_shorter(ieee754_bits<Float> br) noexcept
					{
						return{ br.is_negative() };
					}
				};
				struct nearest_toward_zero {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_toward_zero;
					static constexpr auto tag = tag_t::to_nearest;

					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
						return f(nearest_toward_zero{});
					}
					template <class Float>
					static constexpr interval_type::right_closed_left_open
						interval_type_normal(ieee754_bits<Float>) noexcept
					{
						return{};
					}
					template <class Float>
					static constexpr interval_type::right_closed_left_open
						interval_type_shorter(ieee754_bits<Float>) noexcept
					{
						return{};
					}
				};
				struct nearest_away_from_zero {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_away_from_zero;
					static constexpr auto tag = tag_t::to_nearest;

					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
						return f(nearest_away_from_zero{});
					}
					template <class Float>
					static constexpr interval_type::left_closed_right_open
						interval_type_normal(ieee754_bits<Float>) noexcept
					{
						return{};
					}
					template <class Float>
					static constexpr interval_type::left_closed_right_open
						interval_type_shorter(ieee754_bits<Float>) noexcept
					{
						return{};
					}
				};

				namespace detail {
					struct nearest_always_closed {
						static constexpr auto tag = tag_t::to_nearest;

						template <class Float>
						static constexpr interval_type::closed
							interval_type_normal(ieee754_bits<Float>) noexcept
						{
							return{};
						}
						template <class Float>
						static constexpr interval_type::closed
							interval_type_shorter(ieee754_bits<Float>) noexcept
						{
							return{};
						}
					};
					struct nearest_always_open {
						static constexpr auto tag = tag_t::to_nearest;

						template <class Float>
						static constexpr interval_type::open
							interval_type_normal(ieee754_bits<Float>) noexcept
						{
							return{};
						}
						template <class Float>
						static constexpr interval_type::open
							interval_type_shorter(ieee754_bits<Float>) noexcept
						{
							return{};
						}
					};
				}

				struct nearest_to_even_static_boundary {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_to_even_static_boundary;
					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
						if (br.u % 2 == 0) {
							return f(detail::nearest_always_closed{});
						}
						else {
							return f(detail::nearest_always_open{});
						}
					}
				};
				struct nearest_to_odd_static_boundary {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_to_odd_static_boundary;
					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
						if (br.u % 2 == 0) {
							return f(detail::nearest_always_open{});
						}
						else {
							return f(detail::nearest_always_closed{});
						}
					}
				};
				struct nearest_toward_plus_infinity_static_boundary {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_toward_plus_infinity_static_boundary;
					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
						if (br.is_negative()) {
							return f(nearest_toward_zero{});
						}
						else {
							return f(nearest_away_from_zero{});
						}
					}
				};
				struct nearest_toward_minus_infinity_static_boundary {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = nearest_toward_minus_infinity_static_boundary;
					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
						if (br.is_negative()) {
							return f(nearest_away_from_zero{});
						}
						else {
							return f(nearest_toward_zero{});
						}
					}
				};

				namespace detail {
					struct left_closed_directed {
						static constexpr auto tag = tag_t::left_closed_directed;

						template <class Float>
						static constexpr interval_type::left_closed_right_open
							interval_type_normal(ieee754_bits<Float>) noexcept
						{
							return{};
						}
					};
					struct right_closed_directed {
						static constexpr auto tag = tag_t::right_closed_directed;

						template <class Float>
						static constexpr interval_type::right_closed_left_open
							interval_type_normal(ieee754_bits<Float>) noexcept
						{
							return{};
						}
					};
				}

				struct toward_plus_infinity {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = toward_plus_infinity;
					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
						if (br.is_negative()) {
							return f(detail::left_closed_directed{});
						}
						else {
							return f(detail::right_closed_directed{});
						}
					}
				};
				struct toward_minus_infinity {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = toward_minus_infinity;
					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
						if (br.is_negative()) {
							return f(detail::right_closed_directed{});
						}
						else {
							return f(detail::left_closed_directed{});
						}
					}
				};
				struct toward_zero {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = toward_zero;
					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
						return f(detail::left_closed_directed{});
					}
				};
				struct away_from_zero {
					using policy_kind = policy_kind::binary_rounding;
					using binary_rounding_policy = away_from_zero;
					template <class Float, class Func>
					static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
						return f(detail::right_closed_directed{});
					}
				};
			}

			// Decimal (output) rounding mode policy
			namespace decimal_rounding {
				enum class tag_t {
					do_not_care,
					to_even,
					to_odd,
					away_from_zero,
					toward_zero
				};

				struct do_not_care {
					using policy_kind = policy_kind::decimal_rounding;
					using decimal_rounding_policy = do_not_care;
					static constexpr auto tag = tag_t::do_not_care;

					template <class DecimalFp>
					static constexpr void break_rounding_tie(DecimalFp&) noexcept {}
				};

				struct to_even {
					using policy_kind = policy_kind::decimal_rounding;
					using decimal_rounding_policy = to_even;
					static constexpr auto tag = tag_t::to_even;

					template <class DecimalFp>
					static constexpr void break_rounding_tie(DecimalFp& fp) noexcept
					{
						fp.significand = fp.significand % 2 == 0 ?
							fp.significand : fp.significand - 1;
					}
				};

				struct to_odd {
					using policy_kind = policy_kind::decimal_rounding;
					using decimal_rounding_policy = to_odd;
					static constexpr auto tag = tag_t::to_odd;

					template <class DecimalFp>
					static constexpr void break_rounding_tie(DecimalFp& fp) noexcept
					{
						fp.significand = fp.significand % 2 != 0 ?
							fp.significand : fp.significand - 1;
					}
				};

				struct away_from_zero {
					using policy_kind = policy_kind::decimal_rounding;
					using decimal_rounding_policy = away_from_zero;
					static constexpr auto tag = tag_t::away_from_zero;

					template <class DecimalFp>
					static constexpr void break_rounding_tie(DecimalFp& fp) noexcept {}
				};

				struct toward_zero {
					using policy_kind = policy_kind::decimal_rounding;
					using decimal_rounding_policy = toward_zero;
					static constexpr auto tag = tag_t::toward_zero;

					template <class DecimalFp>
					static constexpr void break_rounding_tie(DecimalFp& fp) noexcept
					{
						--fp.significand;
					}
				};
			}

			// Cache policy
			namespace cache {
				struct fast {
					using policy_kind = policy_kind::cache;
					using cache_policy = fast;
					template <ieee754_format format>
					static constexpr typename dragonbox::cache_holder<format>::cache_entry_type
						get_cache(int k) noexcept
					{
						assert(k >= dragonbox::cache_holder<format>::min_k &&
							k <= dragonbox::cache_holder<format>::max_k);
						return dragonbox::cache_holder<format>::cache[
							std::size_t(k - dragonbox::cache_holder<format>::min_k)];
					}
				};

				struct compact {
					using policy_kind = policy_kind::cache;
					using cache_policy = compact;
					template <ieee754_format format>
					static constexpr typename dragonbox::cache_holder<format>::cache_entry_type
						get_cache(int k) noexcept
					{
						assert(k >= dragonbox::cache_holder<format>::min_k &&
							k <= dragonbox::cache_holder<format>::max_k);

						if constexpr (format == ieee754_format::binary64)
						{
							// Compute base index
							auto cache_index = (k - dragonbox::cache_holder<format>::min_k) /
								dragonbox::compressed_cache_detail::compression_ratio;
							auto kb = cache_index * dragonbox::compressed_cache_detail::compression_ratio
								+ dragonbox::cache_holder<format>::min_k;
							auto offset = k - kb;

							// Get base cache
							auto base_cache = dragonbox::compressed_cache_detail::cache.table[cache_index];

							if (offset == 0) {
								return base_cache;
							}
							else {
								// Compute the required amount of bit-shift
								auto alpha = log::floor_log2_pow10(kb + offset) - log::floor_log2_pow10(kb) - offset;
								assert(alpha > 0 && alpha < 64);

								// Try to recover the real cache
								auto pow5 = dragonbox::compressed_cache_detail::pow5.table[offset];
								auto recovered_cache = wuint::umul128(base_cache.high(), pow5);
								auto middle_low = wuint::umul128(base_cache.low() - (kb < 0 ? 1 : 0), pow5);

								recovered_cache += middle_low.high();

								auto high_to_middle = recovered_cache.high() << (64 - alpha);
								auto middle_to_low = recovered_cache.low() << (64 - alpha);

								recovered_cache = wuint::uint128{
									(recovered_cache.low() >> alpha) | high_to_middle,
									((middle_low.low() >> alpha) | middle_to_low)
								};

								if (kb < 0) {
									recovered_cache += 1;
								}

								// Get error
								auto error_idx = (k - dragonbox::cache_holder<format>::min_k) / 16;
								auto error = (dragonbox::compressed_cache_detail::errors[error_idx] >>
									((k - dragonbox::cache_holder<format>::min_k) % 16) * 2) & 0x3;

								// Add the error back
								assert(recovered_cache.low() + error >= recovered_cache.low());
								recovered_cache = {
									recovered_cache.high(),
									recovered_cache.low() + error
								};

								return recovered_cache;
							}
						}
						else
						{
							return dragonbox::cache_holder<format>::cache[
								std::size_t(k - dragonbox::cache_holder<format>::min_k)];
						}
					}
				};
			}

			namespace input_validation {
				struct assert_finite {
					using policy_kind = policy_kind::input_validation;
					using input_validation_policy = assert_finite;
					template <class Float>
					static void validate_input([[maybe_unused]] ieee754_bits<Float> br) noexcept
					{
						assert(br.is_finite());
					}
				};

				struct do_nothing {
					using policy_kind = policy_kind::input_validation;
					using input_validation_policy = do_nothing;
					template <class Float>
					static void validate_input(ieee754_bits<Float>) noexcept {}
				};
			}
		}
	}

	namespace policy {
		namespace sign {
			static constexpr auto ignore = detail::policy::sign::ignore{};
			static constexpr auto propagate = detail::policy::sign::propagate{};
		}

		namespace trailing_zero {
			static constexpr auto allow = detail::policy::trailing_zero::allow{};
			static constexpr auto remove = detail::policy::trailing_zero::remove{};
			static constexpr auto report = detail::policy::trailing_zero::report{};
		}

		namespace binary_rounding {
			static constexpr auto nearest_to_even =
				detail::policy::binary_rounding::nearest_to_even{};
			static constexpr auto nearest_to_odd =
				detail::policy::binary_rounding::nearest_to_odd{};
			static constexpr auto nearest_toward_plus_infinity =
				detail::policy::binary_rounding::nearest_toward_plus_infinity{};
			static constexpr auto nearest_toward_minus_infinity =
				detail::policy::binary_rounding::nearest_toward_minus_infinity{};
			static constexpr auto nearest_toward_zero =
				detail::policy::binary_rounding::nearest_toward_zero{};
			static constexpr auto nearest_away_from_zero =
				detail::policy::binary_rounding::nearest_away_from_zero{};

			static constexpr auto nearest_to_even_static_boundary =
				detail::policy::binary_rounding::nearest_to_even_static_boundary{};
			static constexpr auto nearest_to_odd_static_boundary =
				detail::policy::binary_rounding::nearest_to_odd_static_boundary{};
			static constexpr auto nearest_toward_plus_infinity_static_boundary =
				detail::policy::binary_rounding::nearest_toward_plus_infinity_static_boundary{};
			static constexpr auto nearest_toward_minus_infinity_static_boundary =
				detail::policy::binary_rounding::nearest_toward_minus_infinity_static_boundary{};

			static constexpr auto toward_plus_infinity =
				detail::policy::binary_rounding::toward_plus_infinity{};
			static constexpr auto toward_minus_infinity =
				detail::policy::binary_rounding::toward_minus_infinity{};
			static constexpr auto toward_zero =
				detail::policy::binary_rounding::toward_zero{};
			static constexpr auto away_from_zero =
				detail::policy::binary_rounding::away_from_zero{};
		}

		namespace decimal_rounding {
			static constexpr auto do_not_care =
				detail::policy::decimal_rounding::do_not_care{};
			static constexpr auto to_even =
				detail::policy::decimal_rounding::to_even{};
			static constexpr auto to_odd =
				detail::policy::decimal_rounding::to_odd{};
			static constexpr auto away_from_zero =
				detail::policy::decimal_rounding::away_from_zero{};
			static constexpr auto toward_zero =
				detail::policy::decimal_rounding::toward_zero{};
		}

		namespace cache {
			static constexpr auto fast = detail::policy::cache::fast{};
			static constexpr auto compact = detail::policy::cache::compact{};
		}

		namespace input_validation {
			static constexpr auto assert_finite =
				detail::policy::input_validation::assert_finite{};
			static constexpr auto do_nothing =
				detail::policy::input_validation::do_nothing{};
		}
	}
}

#include "detail/undef_macros.h"
#endif