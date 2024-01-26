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
#if ENABLE_PARSING_TRIGGER

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
    OUTPUT("found atom \"", s, "\"");

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

template<template<typename tt, const tt& v> typename ... tt_parsers>
using make_combo = combine_variant<t_smanager, smanager, tt_parsers...>;


//////////////////////////////////////////
// matching implementation

using t_test = std::vector<std::string>;

template<typename T, const t_parser_trigger& f> struct check_core_t {
  static bool function(const t_test& t) {
    bool res = true;
    for(auto& s: t) {
      bool ok = T::trigger(s);
      bool tmp = (ok == f(s));
      CHECK(tmp);
      if(ok && tmp) { [[maybe_unused]] T tmp(s); }
      else { res = false; }
    }
    return res;
  }
};

template<typename T> struct check_all_t;
template<> struct check_all_t<std::tuple<>> { static bool function(t_test&) { return true; } };
template<typename T, typename ... t_rest> struct check_all_t<std::tuple<T, t_rest...>> {
  static bool function(t_test& test) {
    using TT = check_all_t<std::tuple<t_rest...>>;
    bool res = T::function(test);
    res = res && TT::function(test);
    return res;
  }
};

template<typename TT> bool check_all(t_test& tests) {
  using T = check_all_t<TT>;
  return T::function(tests);
}


//////////////////////////////////////////
// instantiation of the test

template<typename t_alphabet, const t_alphabet& alphabet>
using my_automata = tt_automata<hrw::utils::natset, hrw::utils::natset>::template type<t_alphabet, alphabet>;

t_parser_trigger trigger_element  = &(is_element<std::string>);
t_parser_trigger trigger_sequence = &(is_sequence<std::string>);
t_parser_trigger trigger_automata = &(is_regexp<std::string>);

using t_element  = check_core_t<make<element>, trigger_element>;
using t_sequence = check_core_t<make<sequence>, trigger_sequence>;
using t_automata = check_core_t<make<my_automata>, trigger_automata>;

using t_combo_1 = check_core_t<make_combo<element>, trigger_element>;
using t_combo_2 = check_core_t<make_combo<element, sequence>, trigger_sequence>;
using t_combo_3 = check_core_t<make_combo<element, sequence, my_automata>, trigger_automata>;

using t_parsers = std::tuple<t_element, t_sequence, t_automata, t_combo_1, t_combo_2, t_combo_3>;

static t_test test = {"a", "b", "b b b b b", "a b c d", "a*", "b(c*)a", "b| a*) i"};
static t_test test_bis = {"a[2]", "b[2,4]", "b*[3,4] b[0] b b+ b[4,5]*", "(a?)[1] b+ c* d[1,3]", "a[1,a]", "b[1](c*)[a", "b[1,2] | a*[2,1] i"};


TEST_CASE("parsing trigger") {
  std::cout << "==================================================================\n";
  std::cout << "= parsing trigger\n";

  // 1. check actual base trigger implementation
  CHECK      (trigger_element(test[0]));
  CHECK      (trigger_element(test[1]));
  CHECK_FALSE(trigger_element(test[2]));
  CHECK_FALSE(trigger_element(test[3]));
  CHECK_FALSE(trigger_element(test[4]));
  CHECK_FALSE(trigger_element(test[5]));
  CHECK_FALSE(trigger_element(test[6]));

  CHECK      (trigger_sequence(test[0]));
  CHECK      (trigger_sequence(test[1]));
  CHECK      (trigger_sequence(test[2]));
  CHECK      (trigger_sequence(test[3]));
  CHECK_FALSE(trigger_sequence(test[4]));
  CHECK_FALSE(trigger_sequence(test[5]));
  CHECK_FALSE(trigger_sequence(test[6]));

  CHECK      (trigger_automata(test[0]));
  CHECK      (trigger_automata(test[1]));
  CHECK      (trigger_automata(test[2]));
  CHECK      (trigger_automata(test[3]));
  CHECK      (trigger_automata(test[4]));
  CHECK      (trigger_automata(test[5]));
  CHECK_FALSE(trigger_automata(test[6]));

  OUTPUT("= \"", test_bis[0], "\"");
  CHECK      (trigger_automata(test_bis[0]));
  OUTPUT("= \"", test_bis[1], "\"");
  CHECK      (trigger_automata(test_bis[1]));
  OUTPUT("= \"", test_bis[2], "\"");
  CHECK      (trigger_automata(test_bis[2]));
  OUTPUT("= \"", test_bis[3], "\"");
  CHECK      (trigger_automata(test_bis[3]));
  OUTPUT("= \"", test_bis[4], "\"");
  CHECK_FALSE(trigger_automata(test_bis[4]));
  OUTPUT("= \"", test_bis[5], "\"");
  CHECK_FALSE(trigger_automata(test_bis[5]));
  OUTPUT("= \"", test_bis[6], "\"");
  CHECK_FALSE(trigger_automata(test_bis[6]));

  check_all<t_parsers>(test);
}


#endif

