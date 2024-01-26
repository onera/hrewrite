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
#if ENABLE_THEORY_VARIABLE

#include "doctest/doctest.h"

#include "hrewrite/theory/core.hpp"
#include "hrewrite/theory/theory_variable.hpp"

#include "tests/theory/common.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace hrw::theory;



//////////////////////////////////////////
// core structure (common for tests)

struct spec {
  spec(const std::string& s): m_content(stoi(s)) {}
  unsigned int m_content;
  bool operator==(const spec& s) const { return this->m_content == s.m_content; }
};

template<typename th>
struct tt_term {
  using type = tt_term<th>;
  using reference = type*;
  using t_content = typename th::t_term; // different from other tests, because of specifiy nature of variables
  static constexpr bool is_const = false;

  tt_term(t_content c): m_content(c) {}
  t_content m_content;
};

template<typename th>
using tt_factory = typename th::template factory<tt_term<th>>;


//////////////////////////////////////////
// construction of the theory

using t_theory_variable = tt_theory_variable_map<spec>;
using t_term = tt_term<t_theory_variable>;
using t_factory = tt_factory<t_theory_variable>;

using t_term_inner   = typename t_theory_variable::t_term;
using t_hash_shallow = typename t_term_inner::template t_hash<false>;
using t_eq_shallow   = typename t_term_inner::template t_eq<false>;
using t_hash_deep    = typename t_term_inner::template t_hash<true>;
using t_eq_deep      = typename t_term_inner::template t_eq<true>;


static std::vector<typename std::string> tests_id = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};


TEST_CASE("theory variable") {
  std::cout << "==================================================================\n";
  std::cout << "= theory variable\n";
  t_factory th;
  std::vector<t_term> vs;
  for(auto s: tests_id) {
  	std::cout << "==========================================" << std::endl;
  	t_term t = th.create_term(s);
    CHECK(t.m_content.get_spec() == spec(s));
  	// std::cout << "my_term => " << t.m_content.get_spec() << std::endl;
    vs.push_back(t);
    for(const t_term& t2: vs) {
      CHECK(!t_eq_shallow()(t.m_content, t2.m_content));
      CHECK(!t_eq_deep()(t.m_content, t2.m_content));
    }
  }


  unsigned int i = 0;
  for(const t_term& t: vs) {
    CHECK(t.m_content.get_spec() == spec(tests_id[i]));
    for(unsigned int j = 0; j < tests_id.size(); ++j) {
      CHECK((t_eq_shallow()(t.m_content, vs[j].m_content)) == (i == j));
      CHECK((t_eq_deep()(t.m_content, vs[j].m_content)) == (i == j));
    }
    ++i;
  }
}


#endif

