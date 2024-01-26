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

#ifndef __HREWRITE_TEST_HTERM_H__
#define __HREWRITE_TEST_HTERM_H__


#include "doctest/doctest.h"

#include "hrewrite/utils/type_traits.hpp"

#include "hrewrite/hterm.hpp"

#include <type_traits>
#include <tuple>
#include <string_view>
#include <iostream>


////////////////////////////////////////////////////////////////////////////////
//// WRAPPERS
////////////////////////////////////////////////////////////////////////////////

template<template<typename> typename t_make_ptr> struct make_ptr_wrapper {
  template<typename T> using type = t_make_ptr<T>;
};
template <template<typename> typename ... Ts> using make_make_ptr_list = std::tuple<make_ptr_wrapper<Ts>...>;

// parser variable
template<template<typename targ_alphabet, const targ_alphabet&> typename parser> struct parser_wrapper {
  template<typename targ_alphabet, const targ_alphabet& alphabet> using type = parser<targ_alphabet, alphabet>;
};
template<template<typename targ_alphabet, const targ_alphabet&> typename ... Ts> using make_parser_list = std::tuple<parser_wrapper<Ts>...>;

// theory_variable
template<template<template<typename tt, const tt&> typename P> typename t_th_variable> struct vth_wrapper {
  template<template<typename tt, const tt&> typename P> using type = t_th_variable<P>;
};
template<template<template<typename tt, const tt&> typename P> typename ... Ts> using make_vth_list = std::tuple<vth_wrapper<Ts>...>;

// theory structured
template<typename ... Ts> using make_sth_list = std::tuple<Ts...>;


// test function
template<template<typename config, typename ... Opts> typename func_class> struct t_func_class_wrapper {
  template<typename config, typename ... Opts> using type = func_class<config, Opts...>;
};
template<template<typename config, typename ... Opts> typename ... Ts> using t_func_class_wrapper_list = std::tuple<t_func_class_wrapper<Ts>...>;

////////////////////////////////////////////////////////////////////////////////
//// BASE STRUCTURES
////////////////////////////////////////////////////////////////////////////////

template<
    typename targ_alphabet,
    const targ_alphabet& arg_alphabet,
    typename targ_sort_resolver,
    typename targ_make_ptr_wrapper,
    typename targ_vparser_wrapper,
    typename targ_vth_wrapper,
    typename targ_sths> struct tt_config {
  using t_alphabet = targ_alphabet;
  static const t_alphabet& alphabet;
  using t_sort_resolver = targ_sort_resolver;
  using t_make_ptr = targ_make_ptr_wrapper;
  using t_vparser = targ_vparser_wrapper;
  using t_vth = targ_vth_wrapper;
  using t_sths = targ_sths;
};
template<
  typename t_alphabet, const t_alphabet& arg_alphabet, typename t_sort_resolver,
  typename make_ptr_wrapper, typename t_vparser_wrapper, typename t_vth_wrapper, typename t_sths>
const t_alphabet& tt_config<t_alphabet, arg_alphabet, t_sort_resolver, make_ptr_wrapper, t_vparser_wrapper, t_vth_wrapper, t_sths>::alphabet = arg_alphabet;


template<typename config> struct get_term_full {
  using vth_t = typename config::t_vth::template type<config::t_vparser::template type>::template type<typename config::t_alphabet, config::alphabet>;
  template<typename T> using t_make_ptr = typename config::t_make_ptr::template type<T>;
  template<typename> struct tmp;
  template<typename ... t_sths> struct tmp<std::tuple<t_sths...>> {
    using type = hrw::tt_term_full<
      t_make_ptr,
      typename config::t_sort_resolver,
      vth_t::template tt_substitution,
      t_sths::template tt_term...
    >;  
  };
  using type = typename tmp<typename config::t_sths>::type;
};


////////////////////////////////////////////////////////////////////////////////
//// GETTERS
////////////////////////////////////////////////////////////////////////////////

template<typename config> using get_term_full_t = typename get_term_full<config>::type;
template<typename config> using get_make_ptr_t = typename config::t_make_ptr::template type<get_term_full_t<config>>;

template<typename config> using get_vth_t = typename get_term_full<config>::vth_t;
template<typename config> using get_vfactory_t = typename get_term_full<config>::vth_t::template factory<get_term_full_t<config>>;
template<typename config> using get_vterm_t = typename get_term_full_t<config>::t_variable;
template<typename config> using get_substitution_t = typename get_term_full_t<config>::t_substitution;

template<typename sth, typename config> using get_sfactory_t = typename sth::template factory<get_term_full_t<config>>;
template<typename sth, typename config> using get_sterm_t = typename sth::template tt_term<typename get_term_full_t<config>::reference>;


template<typename config> constexpr unsigned get_pcomplexity_v = get_vth_t<config>::t_spec::complexity;
template<typename config>
bool is_valid_spec(std::string const& s) {
  return get_term_full_t<config>::t_variable::t_spec::t_parser_trigger(s);
}

template<typename sth, typename config> constexpr bool is_valid_sth_v = hrw::utils::tuple_contains_v<get_sterm_t<sth, config>, typename get_term_full_t<config>::t_tuple_sterm>;


////////////////////////////////////////////////////////////////////////////////
//// TEST GENERATION
////////////////////////////////////////////////////////////////////////////////

template<
    typename t_alphabet,
    const t_alphabet& alphabet,
    typename t_sort_resolver,
    typename make_make_ptr_list,
    typename t_vmake_parser_list,
    typename make_vth_list,
    typename t_sths_list,
    typename t_function_class_list> struct test_generation {
  using core_configs = hrw::utils::tuple_product_t< make_make_ptr_list, t_vmake_parser_list, make_vth_list, t_sths_list, t_function_class_list>;
  template<typename> struct t_exec_core_config;
  template< 
      typename make_ptr_wrapper,
      typename t_vparser_wrapper,
      typename t_vth_wrapper,
      typename t_sth_wrappers,
      typename t_function_class> struct t_exec_core_config<std::tuple<make_ptr_wrapper, t_vparser_wrapper, t_vth_wrapper, t_sth_wrappers, t_function_class>> {
    using config = tt_config<t_alphabet, alphabet, t_sort_resolver, make_ptr_wrapper, t_vparser_wrapper, t_vth_wrapper, t_sth_wrappers>;
    static void run() {
      using T = typename t_function_class::template type<config>;
      using t_make_ptr = get_make_ptr_t<config>;
      using t_vparser = typename config::t_vparser;
      using t_vth = typename config::t_vth;
      using t_sths = typename config::t_sths;

      std::string t_name = hrw::utils::type_name<T>();
      std::string_view t_core = std::string_view(t_name.data(), t_name.find("<"));

      std::cout << "running test " << t_core << "<" <<
      hrw::utils::type_name<t_make_ptr>() << ", " <<
      hrw::utils::type_name<t_vparser>() << ", " <<
      hrw::utils::type_name<t_vth>() << ", " <<
      hrw::utils::type_name<t_sths>() << ">" << std::endl;

      T().run();
    }
  };
  using execs = hrw::utils::tuple_map_t<t_exec_core_config, core_configs>;
  static void run() {
    std::apply([](auto ... args) {
      (args.run(), ...);
    }, execs());
  }
};


#endif // __HREWRITE_TEST_HTERM_H__

