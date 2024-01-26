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
#if ENABLE_UTILS_NATSET

#include "doctest/doctest.h"
#include "tests/common/debug.hpp"


#include "hrewrite/utils/natset.hpp"
#include "hrewrite/utils/type_traits.hpp"
#include "hrewrite/utils/print.hpp"

using namespace hrw::utils;



#include <unordered_set>
#include <random>
#include <climits>
#include <vector>
#include <algorithm>


using t_nat = _natset_detail::t_nat;

using my_set = std::unordered_set<t_nat>;

using t_data = data_wrapper<std::vector<t_nat>>;

template<typename T>
void print_container(const T& c) {
  t_data tmp(std::vector<t_nat>(c.begin(), c.end()));
  std::sort(tmp.m_content.begin(), tmp.m_content.end());
  OUTPUT("    ", tmp);
}

template<typename T, std::size_t max>
T create_natset() {
  if constexpr(T::kind == e_natset_kind::FREE) {
    return T();
  } else {
    return T(max);
  }
}


static std::vector<t_nat> insert_1;
static std::vector<t_nat> erase_1;
static std::vector<t_nat> insert_2;
static std::vector<t_nat> erase_2;


bool init_test(std::size_t limit, std::size_t nb_insert_1, std::size_t nb_erase_1, std::size_t nb_insert_2, std::size_t nb_erase_2) {
  std::mt19937 gen;
  std::uniform_int_distribution<> distrib(0, limit);

  insert_1.reserve(nb_insert_1);
  for(std::size_t i = 0; i < nb_insert_1; ++i) {
    t_nat val = distrib(gen);
    OUTPUT("  insert -1- ", val);
    insert_1.push_back(val);
  }

  erase_1.reserve(nb_erase_1);
  for(std::size_t i = 0; i < nb_erase_1; ++i) {
    t_nat val = distrib(gen);
    OUTPUT("  erase -1- ", val);
    erase_1.push_back(val);
  }

  insert_2.reserve(nb_insert_2);
  for(std::size_t i = 0; i < nb_insert_2; ++i) {
    t_nat val = distrib(gen);
    OUTPUT("  insert -2- ", val);
    insert_2.push_back(val);
  }

  erase_2.reserve(nb_erase_2);
  for(std::size_t i = 0; i < nb_erase_2; ++i) {
    t_nat val = distrib(gen);
    OUTPUT("  erase -2- ", val);
    erase_2.push_back(val);
  }

  OUTPUT("= insert_1:");
  print_container(insert_1);
  OUTPUT("= erase_1:");
  print_container(erase_1);
  OUTPUT("= insert_2:");
  print_container(insert_2);
  OUTPUT("= erase_2:");
  print_container(erase_2);

  OUTPUT("generation done");

  return true;
}


static constexpr std::size_t max_value = 440; // 20
static const bool dummy = init_test(max_value, 129, 129, 129, 129);
// static const bool dummy = init_test(max_value, 12, 12, 12, 12);


template<typename N, std::size_t max>
void check_natset_state(const N& s, const my_set& content) {
  // check contains
  for(t_nat i = 0; i < max; ++i) {
    // OUTPUT("    validates -1- ", i);
    CHECK_MESSAGE(s.contains(i) == (content.find(i) != content.end()), "validates -1- " , i);
  }
  // check iterator
  for(t_nat i: s) {
    // OUTPUT("    validates -2- ", i);
    CHECK_MESSAGE(s.contains(i), "validates -2- ", i);
    CHECK_MESSAGE(content.find(i) != content.end(), "validates -2- ", i);
  }
  // check contained values
  for(t_nat i: content) {
    // OUTPUT("    validates -3- ", i);
    auto it = std::find(s.begin(), s.end(), i);
    CHECK_MESSAGE(it != s.end(), "validates -3- ", i);
    CHECK_MESSAGE(s.contains(i), "validates -3- ", i);
  }
}


template<typename N, std::size_t max>
void check_natset_base_api() {
  auto s = create_natset<N, max>();
  my_set control;

  OUTPUT(" check_natset_base_api(", type_name<N>(), ", ", max, ")");

  OUTPUT("   add some elements -1-");
  for(auto val: insert_1) {
    if(val <= max) {
      OUTPUT("  insert -1- ", val);
      s.insert(val);
      control.insert(val);
      check_natset_state<N, max+10>(s, control);
    }
  }

  OUTPUT("   remove some elements -1-");
  for(auto val: erase_1) {
    if(val <= max) {
      OUTPUT("  remove -1- ", val);
      s.erase(val);
      control.erase(val);
      check_natset_state<N, max+10>(s, control);
    }
  }

  OUTPUT("   add some elements -2-");
  for(auto val: insert_2) {
    if(val <= max) {
      OUTPUT("  insert -2- ", val);
      s.insert(val);
      control.insert(val);
      check_natset_state<N, max+10>(s, control);
    }
  }

  OUTPUT("   remove some elements -2-");
  for(auto val: erase_2) {
    if(val <= max) {
      OUTPUT("  remove -2- ", val);
      s.erase(val);
      control.erase(val);
      check_natset_state<N, max+10>(s, control);
    }
  }
}


template<typename N, std::size_t max>
void check_natset_set_api() {
  auto s1 = create_natset<N, max>();
  auto s2 = create_natset<N, max>();
  auto s3 = create_natset<N, max>();
  auto s4 = create_natset<N, max>();

  my_set c1(insert_1.begin(), insert_1.end()),
         c2(erase_1.begin(), erase_1.end()),
         c3(insert_2.begin(), insert_2.end()),
         c4(erase_2.begin(), erase_2.end());

  for(t_nat val: insert_1) { if(val <= max) { s1.insert(val); } }
  for(t_nat val: erase_1)  { if(val <= max) { s2.insert(val); } }
  for(t_nat val: insert_2) { if(val <= max) { s3.insert(val); } }
  for(t_nat val: erase_2)  { if(val <= max) { s4.insert(val); } }

  OUTPUT(" check_natset_set_api(", type_name<N>(), ", ", max, ")");

  auto s = create_natset<N, max>();
  my_set control;

  // side effect free
  for(t_nat val: c1) { if(val <= max) { control.insert(val); } }
  for(t_nat val: c2) { if(val <= max) { control.insert(val); } }
  OUTPUT("   = cup 1");
  OUTPUT("    ", s1.cup(s2));
  print_container(control);
  check_natset_state<N, max+10>(s1.cup(s2), control);
  control.clear();

  for(t_nat val: c1) { if(val <= max) { control.insert(val); } }
  for(t_nat val: c3) { if(val <= max) { control.insert(val); } }
  OUTPUT("   = cup 2");
  OUTPUT("    ", s3.cup(s1));
  print_container(control);
  check_natset_state<N, max+10>(s3.cup(s1), control);
  control.clear();

  for(t_nat val: c1) { if((val <= max) && (c2.find(val) != c2.end())) { control.insert(val); } }
  OUTPUT("   = cap 1");
  OUTPUT("    ", s1.cap(s2));
  print_container(control);
  check_natset_state<N, max+10>(s1.cap(s2), control);
  control.clear();

  for(t_nat val: c1) { if((val <= max) && (c3.find(val) != c3.end())) { control.insert(val); } }
  OUTPUT("   = cap 2");
  OUTPUT("    ", s3.cap(s1));
  print_container(control);
  check_natset_state<N, max+10>(s3.cap(s1), control);
  control.clear();

  // side effect
  for(t_nat val: c1) { if(val <= max) { control.insert(val); } }
  for(t_nat val: c2) { if(val <= max) { control.insert(val); } }
  s.add(s1);
  s.cup_update(s2);
  OUTPUT("   = cup_update 1");
  OUTPUT("    ", s);
  print_container(control);
  check_natset_state<N, max+10>(s, control);
  control.clear();
  s.clear();

  OUTPUT("    ", s);

  for(t_nat val: c1) { if(val <= max) { control.insert(val); } }
  for(t_nat val: c3) { if(val <= max) { control.insert(val); } }
  s.add(s3);
  s.cup_update(s1);
  OUTPUT("   = cup_update 2");
  OUTPUT("    ", s);
  print_container(control);
  check_natset_state<N, max+10>(s, control);
  control.clear();
  s.clear();

  for(t_nat val: c1) { if((val <= max) && (c2.find(val) != c2.end())) { control.insert(val); } }
  s.add(s1);
  s.cap_update(s2);
  OUTPUT("   = cap_update 1");
  OUTPUT("    ", s);
  print_container(control);
  check_natset_state<N, max+10>(s, control);
  control.clear();
  s.clear();

  for(t_nat val: c1) { if((val <= max) && (c3.find(val) != c3.end())) { control.insert(val); } }
  s.add(s3);
  s.cap_update(s1);
  OUTPUT("   = cap_update 2");
  OUTPUT("    ", s);
  print_container(control);
  check_natset_state<N, max+10>(s, control);
  control.clear();
  s.clear();

}


TEST_CASE("natset") {

  SUBCASE("natset_static") {
    check_natset_base_api<natset_static<8>, 7>();
    check_natset_set_api<natset_static<8>, 7>();
    check_natset_base_api<natset_static<120>, 119>();
    check_natset_set_api<natset_static<120>, 119>();
  }


  SUBCASE("natset_extensible") {
    check_natset_base_api<natset_extensible<>, max_value>();
    check_natset_set_api<natset_extensible<>, max_value>();
  }

  SUBCASE("tt_natset<120>") {
    check_natset_base_api<tt_natset<natset_static<8>>, 7>();
    check_natset_set_api<tt_natset<natset_static<8>>, 7>();
    check_natset_base_api<tt_natset<natset_static<120>>, 119>();
    check_natset_set_api<tt_natset<natset_static<120>>, 119>();
  }

  SUBCASE("tt_natset<natset_extensible>") {
    check_natset_base_api<tt_natset<natset_extensible<>>, max_value>();
    check_natset_set_api<tt_natset<natset_extensible<>>, max_value>();
  }
}


#endif

