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

#ifndef __HREWRITE_TEST_HREWRITE_H__
#define __HREWRITE_TEST_HREWRITE_H__


#include "doctest/doctest.h"

#include "hrewrite/utils/type_traits.hpp"

#include "hrewrite/context_theory.hpp"
#include "hrewrite/context_term.hpp"
#include "hrewrite/context_rw.hpp"

#include <type_traits>
#include <tuple>
#include <string>
#include <string_view>
#include <regex>
#include <iostream>



////////////////////////////////////////////////////////////////////////////////
//// WRAPPERS
////////////////////////////////////////////////////////////////////////////////

// term registeries
template <typename ... Ts> using term_registry_list = std::tuple<Ts...>;

// maps
template<template<typename ... Args> typename map> struct t_map_wrapper {
  template<typename ... Args> using type = map<Args...>;
};
template<template<typename ... Args> typename ... Ts> using map_list = std::tuple<t_map_wrapper<Ts>...>;

// sort context
template <typename ... Ts> using sort_context_param_list = std::tuple<Ts...>;


// parsers
template<template<typename targ_alphabet, const targ_alphabet&> typename parser> struct t_parser_wrapper {
  template<typename targ_alphabet, const targ_alphabet& alphabet> using type = parser<targ_alphabet, alphabet>;
};
template<template<typename targ_alphabet, const targ_alphabet&> typename ... Ts> using parser_list = std::tuple<t_parser_wrapper<Ts>...>;


// theory variable
template<template<template<typename tt, const tt&> typename P> typename t_th> struct t_vth_wrapper {
  template<template<typename tt, const tt&> typename P> using type = t_th<P>;
};
template<template<template<typename tt, const tt&> typename P> typename ... Ts> using variable_theory_list = std::tuple<t_vth_wrapper<Ts>...>;

// theory structured
template<template<typename tt, const tt&> typename t_th> struct t_sth_wrapper {
  template<typename tt, const tt& v> using type = t_th<tt, v>;
};
template<template<typename tt, const tt&> typename ... Ts> using structured_theory_list = std::tuple<t_sth_wrapper<Ts>...>;

// test function
template<template<typename ... Ts> typename func_class> struct t_func_class_wrapper {
  template<typename ... Ts> using type = func_class<Ts...>;
};
template<template<typename ...> typename ... Ts> using function_class_list = std::tuple<t_func_class_wrapper<Ts>...>;


////////////////////////////////////////////////////////////////////////////////
//// BASE STRUCTURES
////////////////////////////////////////////////////////////////////////////////

template<
    std::size_t arg_algebra_id,
    typename targ_term_registry,
    typename targ_map,
    typename targ_sort_context_param,
    typename targ_parser,
    typename targ_variable_theory,
    typename targ_structured_theory_list> struct tt_config {
  static inline constexpr std::size_t algebra_id = arg_algebra_id;
  using t_term_registry = targ_term_registry;
  using t_map = targ_map;
  using t_sort_context_param = targ_sort_context_param;
  using t_parser = targ_parser;
  using t_variable_theory = targ_variable_theory;
  using t_structured_theory_list = targ_structured_theory_list;
};

template<std::size_t id> struct wrap_id { static inline constexpr std::size_t value = id; };

template<typename config> struct get_hrewrite {
  using t_sort_context = hrw::context_sort<typename config::t_sort_context_param>;
  template<typename> struct tmp;
  template<typename ... t_sths> struct tmp<std::tuple<t_sths...>> {
    template<typename tt, const tt& v> using tt_vth = typename config::t_variable_theory::template type<config::t_parser::template type>::template type<tt, v>;
    using type = hrw::context_theory<wrap_id<config::algebra_id>,
      t_sort_context,
      tt_vth,
      t_sths::template type...
    >;
  };
  using ctx_th = typename tmp<typename config::t_structured_theory_list>::type;
  using t_ctx_tm = hrw::context_term<ctx_th, typename config::t_term_registry>;

  using t_term_full = typename t_ctx_tm::t_term_full;
  using t_term_full_ref = typename t_term_full::reference;
  using t_variable = typename t_term_full::t_variable;
  using t_substitution = typename t_term_full::t_substitution;

  using t_ctx_rw = hrw::context_rw<t_ctx_tm, config::t_map::template type>;

  using t_print = hrw::t_hterm_print<ctx_th, t_term_full>;

};


////////////////////////////////////////////////////////////////////////////////
//// GETTERS
////////////////////////////////////////////////////////////////////////////////

template<typename config> using get_ctx_th          = typename get_hrewrite<config>::ctx_th         ;
template<typename config> using get_ctx_tm_t        = typename get_hrewrite<config>::t_ctx_tm       ;

template<typename config> using get_term_full_t     = typename get_hrewrite<config>::t_term_full    ;
template<typename config> using get_term_full_ref_t = typename get_hrewrite<config>::t_term_full_ref;
template<typename config> using get_variable_t      = typename get_hrewrite<config>::t_variable     ;
template<typename config> using get_substitution_t  = typename get_hrewrite<config>::t_substitution ;

template<typename config> using get_ctx_rw_t        = typename get_hrewrite<config>::t_ctx_rw       ;

template<typename config> using get_print_t         = typename get_hrewrite<config>::t_print        ;



template<template<typename tt, const tt&> typename sth, typename config>
constexpr bool is_valid_sth_v = hrw::utils::tuple_contains_v<t_sth_wrapper<sth>, typename config::t_structured_theory_list>;

template<template<typename tt, const tt&> typename sth, typename config>
using get_stheory_t = typename get_ctx_th<config>::template theory_element_t< hrw::utils::tuple_index_v<t_sth_wrapper<sth>, typename config::t_structured_theory_list> >;

template<template<typename tt, const tt&> typename sth, typename config>
using get_sterm_t = typename get_stheory_t<sth, config>::template tt_term<get_term_full_ref_t<config>>;

template<typename config> constexpr unsigned get_p_complexity_v = get_variable_t<config>::t_spec::complexity;
template<typename config>
bool is_valid_spec(std::string const& s) {
  return get_term_full_t<config>::t_variable::t_spec::t_parser_trigger(s);
}



////////////////////////////////////////////////////////////////////////////////
//// TEST GENERATION
////////////////////////////////////////////////////////////////////////////////

struct A {
  using t_letter = int;
  using t_letter_set = int;
  t_letter get_letter(std::string const &) { return 1; }
  bool is_subletter(t_letter, t_letter) { return true; }
};
static inline A a;

template<typename T> void TEST() { std::cout << "TEST = " << __PRETTY_FUNCTION__ << std::endl;}

template<
    typename t_term_registry_list,
    typename t_map_list,
    typename t_sort_context_param_list,
    typename t_parser_list,
    typename t_variable_theory_list,
    typename t_structured_theory_list_list,
    typename t_function_class_list> struct test_generation {
  using core_configs = hrw::utils::tuple_product_t<
    t_term_registry_list, t_map_list, t_sort_context_param_list, t_parser_list, t_variable_theory_list,
    t_structured_theory_list_list, t_function_class_list>;
  template<std::size_t, typename> struct t_exec_core_config;
  template<
      std::size_t algebra_id, 
      typename t_term_registry,
      typename t_map,
      typename t_sort_context_param,
      typename t_parser,
      typename t_variable_theory,
      typename t_structured_theory_list,
      typename t_function_class> struct t_exec_core_config<algebra_id, std::tuple<
        t_term_registry, t_map, t_sort_context_param, t_parser, t_variable_theory, t_structured_theory_list, t_function_class>> {
    using config = tt_config<algebra_id, t_term_registry, t_map, t_sort_context_param, t_parser, t_variable_theory, t_structured_theory_list>;

    template<typename sth_wrapper> using F = typename sth_wrapper::template type<A, a>;
    using sh_list = hrw::utils::tuple_map_t<F, t_structured_theory_list>;

    static void run() {
      using T = typename t_function_class::template type<config>;
      std::regex reg_1(", A, a");
      std::regex reg_2(" A, a");
      std::regex reg_3("A, a");

      auto clean_str = [&](std::string const & s) { return std::regex_replace(std::regex_replace(std::regex_replace(s, reg_1, ""), reg_2, ""), reg_3, ""); };

      std::string t_name_str = hrw::utils::type_name<T>();
      std::string_view t_name = std::string_view(t_name_str.c_str(), t_name_str.find("<"));

      std::string map_name_str = hrw::utils::type_name<typename t_map::template type<int, int>>();
      std::string_view map_name = std::string_view(map_name_str.data(), map_name_str.find("<"));

      std::string parser_name = hrw::utils::type_name<typename t_parser::template type<A, a>>();
      parser_name = clean_str(parser_name);

      std::string vth_name = hrw::utils::type_name<typename t_variable_theory::template type<t_parser::template type>::template type<A, a>>();
      std::string_view vth_name_view = std::string_view(vth_name.data() + vth_name.rfind(" "));
      vth_name_view = std::string_view(vth_name_view.data(), vth_name_view.size() - 1);
      vth_name = clean_str(std::string(vth_name_view));

      std::string sh_list_name = hrw::utils::type_name<sh_list>();
      sh_list_name = clean_str(sh_list_name);


      std::cout << "running test " << t_name << "<" <<
      algebra_id << ", " <<
      hrw::utils::type_name<t_term_registry>() << ", " <<
      map_name << ", " <<
      hrw::utils::type_name<t_sort_context_param>() << ", " <<
      parser_name << ", " <<
      // hrw::utils::type_name<t_variable_theory>() << ", " <<
      vth_name << ", " <<
      sh_list_name  << ">" << std::endl;

      T().run();
    }
  };

  using init = std::tuple<>;
  template<typename A, typename CC> struct Acc_S;
  template<typename CC> struct Acc_S<std::tuple<>, CC> {
    using type = std::tuple<t_exec_core_config<0, CC>>;
  };
  template<typename CC, typename T, typename ... Ts> struct Acc_S<std::tuple<T, Ts...>, CC> {
    using type = std::tuple<t_exec_core_config<T::config::algebra_id+1, CC>, T, Ts...>;
  };
  template<typename A, typename CC> using Acc = typename Acc_S<A, CC>::type;

  using execs = hrw::utils::tuple_fold_left_t<Acc, init, core_configs>;
  static void run() {
    std::apply([](auto ... args) {
      (args.run(), ...);
    }, execs());
  }
};


#endif // __HREWRITE_TEST_HREWRITE_H__

