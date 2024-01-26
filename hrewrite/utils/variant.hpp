/*
 * This file is part of the hrewrite library.
 * Copyright (c) 2021 ONERA.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

 // Author: Michael Lienhardt
 // Maintainer: Michael Lienhardt
 // email: michael.lienhardt@onera.fr

#ifndef __HREWRITE_VARIANT_H__
#define __HREWRITE_VARIANT_H__

#include <tuple>
#include <variant>

#include <type_traits>


namespace hrw {
  namespace utils {


    /////////////////////////////////////////////////////////////////////////////
    // VARIANT VISIT HELPER
    /////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////
    // visit helper
    // to be able to write std::visit(visit_helper {
    //   [](...) {...},
    //   ...,
    //   [](...) {...}
    // }, variant);
    template<typename ... Ts>
    struct visit_helper : Ts ... { using Ts::operator() ...; };
    template<class... Ts> visit_helper(Ts...) -> visit_helper<Ts...>;


    /////////////////////////////////////////////////////////////////////////////
    // VARIANT VISIT HELPER
    /////////////////////////////////////////////////////////////////////////////



    template<typename F, typename V> struct visit_get_result {
      template<typename A, typename=void> struct _inner {
        using type = A&&;
      };
      template<typename A> struct _inner<A, std::void_t<std::invoke_result_t<F&&, const A &>>> {
        using type = const A &;
      };
      using value_type = std::variant_alternative_t<0, std::remove_cv_t<std::remove_reference_t<V>>>;
      using type = std::invoke_result_t<F&&, typename _inner<value_type>::type>;
    };

    template<typename F, typename V> using visit_get_result_t = typename visit_get_result<F, V>::type;

    template<typename F, typename V, std::size_t I=0>
    // constexpr std::invoke_result_t<F&&, const std::variant_alternative_t<0, std::remove_cvref_t<V>>&> visit_single(F &&f, V && v) {
    constexpr visit_get_result_t<F,V> visit_single_impl(F &&f, V && v) {
      using V_core = std::remove_cv_t<std::remove_reference_t<V>>;
      if(v.index() == I) {
        return f(std::get<I>(std::forward<V>(v)));
      }
      if constexpr(I+1 < std::variant_size_v<V_core>) {
        return visit_single_impl<F,V,I+1>(std::forward<F>(f), std::forward<V>(v));
      } else {
        throw std::bad_variant_access{};
      }
    }

  }
}

#define VISIT_SINGLE std::visit
// #define VISIT_SINGLE hrw::utils::visit_single_impl


#endif // __HREWRITE_VARIANT_H__

