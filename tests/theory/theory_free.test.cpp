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
#if ENABLE_THEORY_FREE

#include "doctest/doctest.h"

#include "hrewrite/theory/core.hpp"
#include "hrewrite/theory/theory_free.hpp"


#include <iostream>
#include <string>
#include <vector>

#include "tests/theory/common.hpp"

using namespace hrw::theory;
using namespace hrw::tests;


//////////////////////////////////////////
// template instantiation

template <typename T> using t_vector = std::vector<T>;
using t_spec = int; // cannot be void, since we have reference to it
using t_theory_free  = tt_theory_free<t_spec, t_vector>;

using t_term         = tt_term_full<t_theory_free::template tt_term, false>;
using t_factory      = typename t_theory_free::template factory<t_term>;

using t_term_inner   = typename t_term::template tt_term<t_theory_free::template tt_term>;
using t_container    = typename t_term_inner::t_container;
using t_hash_shallow = typename t_term_inner::t_hash<false>;
using t_eq_shallow   = typename t_term_inner::t_eq<false>;
using t_hash_deep    = typename t_term_inner::t_hash<true>;
using t_eq_deep      = typename t_term_inner::t_eq<true>;


//////////////////////////////////////////
// tests

TEST_CASE("theory free") {
  std::cout << "==================================================================\n";
  std::cout << "= theory free\n";

  t_factory th;
  t_container v;
  t_term t = th.create_term(0, 0, v);

  CHECK(t.m_content.get_sort() == 0);
  CHECK(t.m_content.get_constructor() == 0);
  CHECK(t.m_content.begin() == t.m_content.end());
}





TEST_CASE("theory free unicity") {
  std::cout << "==================================================================\n";
  std::cout << "= theory free unicity\n";

  t_factory th;
  t_container v1;
  t_term t1 = th.create_term(0, 0, v1);
  t_term t2 = th.create_term(0, 0, v1);

  t_container v2{&t1, &t1};
  t_container v3{&t2, &t2};
  t_term t3 = th.create_term(0, 0, v2);
  t_term t4 = th.create_term(0, 1, v2);
  t_term t5 = th.create_term(0, 1, v3);

  CHECK(t_eq_deep()(t1.m_content, t2.m_content));
  CHECK(t_hash_deep()(t1.m_content) == t_hash_deep()(t2.m_content));

  CHECK(!t_eq_deep()(t1.m_content, t3.m_content));
  CHECK(!t_eq_deep()(t3.m_content, t4.m_content));

  CHECK(t_eq_deep()(t4.m_content, t5.m_content));
  CHECK(t_hash_deep()(t4.m_content) == t_hash_deep()(t5.m_content));

  CHECK(t_eq_shallow()(t1.m_content, t2.m_content));
  CHECK(t_hash_shallow()(t1.m_content) == t_hash_shallow()(t2.m_content));

  CHECK(!t_eq_shallow()(t1.m_content, t3.m_content));
  CHECK(!t_eq_shallow()(t3.m_content, t4.m_content));

  CHECK(!t_eq_shallow()(t4.m_content, t5.m_content));
}


#endif

