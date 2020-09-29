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

#ifndef JKJ_HEADER_FP_POLICY_HOLDER
#define JKJ_HEADER_FP_POLICY_HOLDER

#include <type_traits>

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

			// For a given kind, find a policy belonging to that kind.
			// Check if there are more than one such policies.
			enum class policy_found_info {
				not_found, unique, repeated
			};
			template <class Policy, policy_found_info info>
			struct found_policy_pair {
				using policy = Policy;
				static constexpr auto found_info = info;
			};

			template <class Base, class DefaultPolicy>
			struct base_default_pair {
				using base = Base;

				template <class FoundPolicyInfo>
				static constexpr FoundPolicyInfo get_policy_impl(FoundPolicyInfo) {
					return{};
				}
				template <class FoundPolicyInfo, class FirstPolicy, class... RemainingPolicies>
				static constexpr auto get_policy_impl(FoundPolicyInfo, FirstPolicy, RemainingPolicies... remainings) {
					if constexpr (std::is_base_of_v<Base, FirstPolicy>) {
						if constexpr (FoundPolicyInfo::found_info == policy_found_info::not_found) {
							return get_policy_impl(
								found_policy_pair<FirstPolicy, policy_found_info::unique>{},
								remainings...);
						}
						else {
							return get_policy_impl(
								found_policy_pair<FirstPolicy, policy_found_info::repeated>{},
								remainings...);
						}
					}
					else {
						return get_policy_impl(FoundPolicyInfo{},
							remainings...);
					}
				}

				template <class... Policies>
				static constexpr auto get_policy(Policies... policies) {
					return get_policy_impl(
						found_policy_pair<DefaultPolicy, policy_found_info::not_found>{},
						policies...);
				}
			};
			template <class... BaseDefaultPairs>
			struct base_default_pair_list {};

			// Check if a given policy belongs to one of the kinds specified by the library
			template <class Policy>
			constexpr bool check_policy_validity(Policy, base_default_pair_list<>)
			{
				return false;
			}
			template <class Policy, class FirstBaseDefaultPair, class... RemainingBaseDefaultPairs>
			constexpr bool check_policy_validity(Policy,
				base_default_pair_list<FirstBaseDefaultPair, RemainingBaseDefaultPairs...>)
			{
				return std::is_base_of_v<typename FirstBaseDefaultPair::base, Policy> ||
					check_policy_validity(Policy{}, base_default_pair_list< RemainingBaseDefaultPairs...>{});
			}

			template <class BaseDefaultPairList>
			constexpr bool check_policy_list_validity(BaseDefaultPairList) {
				return true;
			}

			template <class BaseDefaultPairList, class FirstPolicy, class... RemainingPolicies>
			constexpr bool check_policy_list_validity(BaseDefaultPairList,
				FirstPolicy, RemainingPolicies... remaining_policies)
			{
				return check_policy_validity(FirstPolicy{}, BaseDefaultPairList{}) &&
					check_policy_list_validity(BaseDefaultPairList{}, remaining_policies...);
			}

			// Build policy_holder
			template <bool repeated_, class... FoundPolicyPairs>
			struct found_policy_pair_list {
				static constexpr bool repeated = repeated_;
			};

			template <class... Policies>
			struct policy_holder : Policies... {};

			template <bool repeated, class... FoundPolicyPairs, class... Policies>
			constexpr auto make_policy_holder_impl(
				base_default_pair_list<>,
				found_policy_pair_list<repeated, FoundPolicyPairs...>,
				Policies...)
			{
				return found_policy_pair_list<repeated, FoundPolicyPairs...>{};
			}

			template <class FirstBaseDefaultPair, class... RemainingBaseDefaultPairs,
				bool repeated, class... FoundPolicyPairs, class... Policies>
				constexpr auto make_policy_holder_impl(
					base_default_pair_list<FirstBaseDefaultPair, RemainingBaseDefaultPairs...>,
					found_policy_pair_list<repeated, FoundPolicyPairs...>,
					Policies... policies)
			{
				using new_found_policy_pair = decltype(FirstBaseDefaultPair::get_policy(policies...));

				return make_policy_holder_impl(
					base_default_pair_list<RemainingBaseDefaultPairs...>{},
					found_policy_pair_list<
					repeated || new_found_policy_pair::found_info == policy_found_info::repeated,
					new_found_policy_pair, FoundPolicyPairs...
					>{}, policies...);
			}

			template <bool repeated, class... RawPolicies>
			constexpr auto convert_to_policy_holder(found_policy_pair_list<repeated>, RawPolicies...) {
				return policy_holder<RawPolicies...>{};
			}

			template <bool repeated, class FirstFoundPolicyPair, class... RemainingFoundPolicyPairs, class... RawPolicies>
			constexpr auto convert_to_policy_holder(
				found_policy_pair_list<repeated, FirstFoundPolicyPair, RemainingFoundPolicyPairs...>, RawPolicies... policies)
			{
				return convert_to_policy_holder(found_policy_pair_list<repeated, RemainingFoundPolicyPairs...>{},
					typename FirstFoundPolicyPair::policy{}, policies...);
			}

			template <class BaseDefaultPairList, class... Policies>
			constexpr auto make_policy_holder(BaseDefaultPairList, Policies... policies) {
				static_assert(check_policy_list_validity(BaseDefaultPairList{}, Policies{}...),
					"jkj::dragonbox: an invalid policy is specified");

				using policy_pair_list = decltype(make_policy_holder_impl(BaseDefaultPairList{},
					found_policy_pair_list<false>{}, policies...));

				static_assert(!policy_pair_list::repeated,
					"jkj::dragonbox: each policy should be specified at most once");

				return convert_to_policy_holder(policy_pair_list{});
			}
		}
	}
}

#endif