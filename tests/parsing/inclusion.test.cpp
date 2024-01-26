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


#include "tests/common/activation.hpp"
#if ENABLE_PARSING_INCLUSION

#include "doctest/doctest.h"
#include "tests/common/debug.hpp"


#include "hrewrite/parsing.hpp"
#include "hrewrite/utils.hpp"

using namespace hrw::utils;


#include <string>
#include <vector>
#include <utility>
#include <unordered_set>

//////////////////////////////////////////
// global alphabet class

struct t_smanager {
  using t_letter = char;
  using t_letter_set = std::unordered_set<t_letter>;
  t_letter get_letter(const std::string& s) const {
    // std::cout << "found atom \"" << s << "\"" << std::endl;

    if(s.size() != 1) {
      throw "ERROR: a string is not a valid symbol : \"" + s + "\"";
    } else {
      return s[0];
    }
  }
  bool is_subletter(const t_letter& s1, const t_letter& s2) const { return s1 <= s2; }
};

static t_smanager smanager;

template<template<typename tt, const tt& v> typename tt_parser>
using make = tt_parser<t_smanager, smanager>;


//////////////////////////////////////////
// inclusion implementation


using inclusion_entry_t = std::tuple<std::string, std::vector<std::string>, std::vector<std::string>>;
using t_test = std::vector<inclusion_entry_t>;
static inline inclusion_entry_t mk_inclusion_entry(std::string regexp, std::vector<std::string> accept, std::vector<std::string> reject) {
  return std::make_tuple(regexp, accept, reject);
}



template<typename t_parser1, typename t_parser2>
void check_core(std::string& exp1, std::string& exp2, bool result) {
  t_parser1 p1(exp1);
  t_parser2 p2(exp2);
  OUTPUT("parser1: ", p1);
  OUTPUT("parser2: ", p2);
  bool res = (inclusion<t_parser1, t_parser2>(p1, p2) == result);
  CHECK(res);
}

template<typename t_parser1, typename t_parser2>
void check_single(t_test& tests) {
  OUTPUT("TESTING check_single<", type_name<t_parser1>(),  ", ", type_name<t_parser2>(), ">");
  for(auto& test: tests) {
    std::string& exp1 = std::get<0>(test);
    if(t_parser1::trigger(exp1)) {
      OUTPUT("  => ", exp1);

      for(std::string& exp2: std::get<1>(test)) {
        if(t_parser2::trigger(exp2)) {
          OUTPUT("    vs (accept) ", exp2);
          check_core<t_parser1, t_parser2>(exp1, exp2, true);          
        }
      }

      for(std::string& exp2: std::get<2>(test)) {
        if(t_parser2::trigger(exp2)) {
          OUTPUT("    vs (reject) ", exp2);
          check_core<t_parser1, t_parser2>(exp1, exp2, false);
        }
      }

    }

  }
}


template<typename T1, typename T2> struct check_all_t;
template<typename ... Ts> struct check_all_t<std::tuple<>, std::tuple<Ts...>> { static void function(t_test&) {} };
template<typename T1, typename ... T1s, typename ... T2s> struct check_all_t<std::tuple<T1, T1s...>, std::tuple<T2s...>> {
    static void function(t_test& tests) {
        using T = check_all_t<std::tuple<T1s...>, std::tuple<T2s...>>;
        (check_single<T1, T2s>(tests), ...);
        T::function(tests);
    } };

template<typename TT1, typename TT2> void check_all(t_test& tests) {
  using T = check_all_t<TT1, TT2>;
   T::function(tests);
}

//////////////////////////////////////////
// instantiation of the test

template<typename t_alphabet, const t_alphabet& alphabet>
using my_automata = tt_automata<hrw::utils::natset, hrw::utils::natset>::template type<t_alphabet, alphabet>;

template<typename targ_alphabet, const targ_alphabet & alphabet>
using tt_combo = combine_variant<targ_alphabet, alphabet, element, sequence, my_automata>;


// using t_parsers = std::tuple<make<element>, make<sequence>, make<my_automata>, make<tt_combo>>;
using t_parsers = std::tuple<make<element>, make<sequence>, make<my_automata>>;


static t_test tests = {
  mk_inclusion_entry("b", { "b", "c" }, { "a", "b b" }),
  mk_inclusion_entry("b b b b b", { "b b b b b", "b*", "c*" }, { "b", "b b b b", "a*" }),
  mk_inclusion_entry("b*", { "b*", "b*|c*" }, { "b+", "a*" }),
  mk_inclusion_entry("b(c*)a | c", {"c*", "c c*" }, { "a", "c", "a b c" }),
};

/**
 * This implements the main test function which perform, for every test in the "tests" array:
 *   - generate the automata from the given regexp
 *   - for every word that should be accepted, check that the word is indeed accepted
 *   - for every word that should not be accepted, check that the word is indeed rejected
 */
TEST_CASE("parsing inclusion") {
  std::cout << "==================================================================\n";
  std::cout << "= parsing inclusion\n";

  check_all<t_parsers, t_parsers>(tests);
}


#endif

