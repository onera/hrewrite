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
#if ENABLE_PARSING_MATCH

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
// matching implementation

/**
 * The four following declarations are used to construct the different tests, and in particular to help C++ with the types.
 * the "tests" arrays (test_sequence and test_automata) contain several tests to make, which have three parts:
 *  - the regexp specifying the language the automata can accept
 *  - a set of words that should be accepted by the automata (this set is implemented as std::vector of std::vector of SIGMA, i.e., std::string)
 *  - a set of words that should not be accepted by the automata (this set is implemented as std::vector of std::vector of SIGMA, i.e., std::string)
 */
using match_entry_t = std::tuple<std::string, std::vector<std::string>, std::vector<std::string>>;
using t_test = std::vector<match_entry_t>;
static inline match_entry_t mk_match_entry(std::string regexp, std::vector<std::string> accept, std::vector<std::string> reject) {
  return std::make_tuple(regexp, accept, reject);
}




static std::string_view test_tostring(const std::string& s, std::string::iterator begin, std::string::iterator end) { return std::string_view(s.c_str() + (begin - s.begin()), (end - s.begin())); }


template<typename t_parser>
bool check_core(t_parser& parser, std::string& s) {
  auto m = match<t_parser, std::string::iterator>(parser, s.begin(), s.end());
  auto i = s.begin();
  bool match_found = false;
  for(auto it = m.begin(); it != m.end(); ++it) {
    i = *it;
    match_found = true;
    OUTPUT("    found match \"", test_tostring(s, s.begin(), i), "\"");
  }
  OUTPUT("    result: match_found=",  match_found, ", end=",  (i == s.end()));
  return match_found && (i == s.end());
}


template<typename t_parser>
bool check_single(t_test& tests) {
  // using _w_state_print = data_wrapper<typename t_parser::t_state>;
  OUTPUT("TESTING check_test_match: ", type_name<t_parser>());
  for(auto& test: tests) {
    std::string& regexp = std::get<0>(test);
    if(t_parser::trigger(regexp)) {
      OUTPUT("Generating parser for regexp \"", regexp, "\"");
      t_parser parser(regexp);
      OUTPUT("parser: ", parser);
      OUTPUT(" starting state: ", wrap::print(parser.start()), ", is final ", parser.is_final(parser.start()));

      for(std::string& accept: std::get<1>(test)) {
        OUTPUT("Checking accept \"", test_tostring(accept, accept.begin(), accept.end()), "\"");
        bool check = check_core<t_parser>(parser, accept);
        CHECK(check);
      }
      for(std::string& reject: std::get<2>(test)) {
        OUTPUT("Checking reject \"", test_tostring(reject, reject.begin(), reject.end()), "\"");
        bool check = check_core<t_parser>(parser, reject);
        CHECK_FALSE(check);
      }
    }
  }
  return true;
}


template<typename T> struct check_all_t;
template<> struct check_all_t<std::tuple<>> { static bool function(t_test&) { return true; } };
template<typename t_parser, typename ... t_rest> struct check_all_t<std::tuple<t_parser, t_rest...>> {
  static bool function(t_test& tests) {
    using T = check_all_t<std::tuple<t_rest...>>;
    bool res = check_single<t_parser>(tests);
    res = res && T::function(tests);
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

template<typename targ_alphabet, const targ_alphabet & alphabet>
using tt_combo = combine_variant<targ_alphabet, alphabet, element, sequence, my_automata>;


using t_parsers = std::tuple<make<element>, make<sequence>, make<my_automata>, make<tt_combo>>;


static t_test tests = {
  mk_match_entry("b", { "a", "b" }, { "", "c", "aa" }),
  mk_match_entry("b b b b b", { "aaaaa" }, { "a", "abbb", "caaaa" }),
  mk_match_entry("b*", { "", "b", "aabaa" }, { "c", "aef" }),
  mk_match_entry("b(c*)a", {"aa", "ba", "aaa", "aaaa" }, { "a", "c", "abc" }),
};

/**
 * This implements the main test function which perform, for every test in the "tests" array:
 *   - generate the automata from the given regexp
 *   - for every word that should be accepted, check that the word is indeed accepted
 *   - for every word that should not be accepted, check that the word is indeed rejected
 */
TEST_CASE("parsing match") {
  std::cout << "==================================================================\n";
  std::cout << "= parsing match\n";

  check_all<t_parsers>(tests);
}


#endif

