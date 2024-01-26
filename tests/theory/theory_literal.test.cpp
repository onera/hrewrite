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
#if ENABLE_THEORY_LITERAL

#include "doctest/doctest.h"

#include "hrewrite/theory/core.hpp"
#include "hrewrite/theory/theory_literal.hpp"

#include "tests/theory/common.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace hrw::theory;



//////////////////////////////////////////
// core structure (common for tests)

struct spec_wrapper {
  using type = unsigned int;
  static type create(const std::string& s) { return stoi(s); }
};

template<typename th>
struct tt_term {
  using type = tt_term<th>;
  using reference = type*;
  using t_content = typename th::template tt_term<type>;
  static constexpr bool is_const = false;

  using t_substitution = void*;
  template<typename termIt>
  struct match_context {
    match_context(const termIt arg_p_end, const termIt arg_t_end, t_substitution& arg_subst):
      m_p_end(arg_p_end), m_t_end(arg_t_end), m_subst(arg_subst) {}
    const termIt m_p_end;
    const termIt m_t_end;
    t_substitution& m_subst;
  };

  tt_term(t_content c): m_content(c) {}
  t_content m_content;
};

template<typename th>
using tt_factory = typename th::template factory<tt_term<th>>;


//////////////////////////////////////////
// construction of the theory

template <typename T>
using t_vector = std::vector<T>;

using t_theory_literal = tt_theory_literal<int>;
using t_term           = tt_term<t_theory_literal>;
using t_factory        = tt_factory<t_theory_literal>;

using t_term_inner   = typename t_theory_literal::template tt_term<t_term>;
using t_hash_shallow = typename t_term_inner::template t_hash<false>;
using t_eq_shallow   = typename t_term_inner::template t_eq<false>;
using t_hash_deep    = typename t_term_inner::template t_hash<true>;
using t_eq_deep      = typename t_term_inner::template t_eq<true>;

//////////////////////////////////////////
// construction of the term class


TEST_CASE("theory literal") {
  std::cout << "==================================================================\n";
  std::cout << "= theory literal\n";

  t_factory th;
  int value = 9001;
  t_term t = th.create_term(0, 0, value);

  CHECK(t.m_content.get_sort() == 0);
  CHECK(t.m_content.get_constructor() == 0);
  CHECK(t.m_content.get_value() == value);
}



TEST_CASE("theory literal unicity") {
  std::cout << "==================================================================\n";
  std::cout << "= theory literal unicity\n";

  t_factory th;
  int value = 9001;
  t_term t1 = th.create_term(0, 0, value);
  t_term t2 = th.create_term(0, 0, value);
  t_term t3 = th.create_term(0, 0, value+1);

  CHECK(t_eq_deep()(t1.m_content, t2.m_content));
  CHECK(t_hash_deep()(t1.m_content) == t_hash_deep()(t2.m_content));
  CHECK(!t_eq_deep()(t1.m_content, t3.m_content));

  CHECK(t_eq_shallow()(t1.m_content, t2.m_content));
  CHECK(t_hash_shallow()(t1.m_content) == t_hash_shallow()(t2.m_content));
  CHECK(!t_eq_shallow()(t1.m_content, t3.m_content));
}


#endif

