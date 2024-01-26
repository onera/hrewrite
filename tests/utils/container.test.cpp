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
#if ENABLE_UTILS_CONTAINER

#include "doctest/doctest.h"
#include "tests/common/debug.hpp"

#include "hrewrite/utils/container.hpp"

#include "hrewrite/utils/type_traits.hpp"
#include "hrewrite/utils/print.hpp"
#include "hrewrite/utils/hash.hpp"

#include "hrewrite/exceptions/utils_core.hpp"

using namespace hrw::utils;

#include <sstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>




//////////////////////////////////////////
// basic structure

struct D {
  using t_content = int;
  D(t_content&& v): m_content(v) {}
  D(t_content const & v): m_content(v) {}

  t_content m_content;
};

struct D_hash {
  std::size_t operator()(D const & d) const { return std::hash<int>()(d.m_content); }
};

bool operator==(D const & d1, D const & d2) { return d1.m_content == d2.m_content; }
bool operator==(D const & d1, int const d2) { return d1.m_content == d2; }
std::ostream& operator<<(std::ostream& os, D const & d) {
  os << d.m_content;
  return os;
}



//////////////////////////////////////////
// recursive structure


template<template<typename T> typename make_ptr>
struct t_rec {
  using type = t_rec<make_ptr>;
  using ptr_struct = make_ptr<type>;
  using t_ptr = typename ptr_struct::ptr_type;
  using t_content = int;

  t_rec(t_content&& v): m_content(v), m_next(std::nullopt) {}
  t_rec(t_content const & v): m_content(v), m_next(std::nullopt) {}

  t_content m_content;
  std::optional<t_ptr> m_next;
};

template<typename T>
struct rec_hash {
  using type = rec_hash<T>;
  using value_type = T;
  using ptr_struct = typename value_type::ptr_struct;
  using H = hrw::utils::hash_combine<std::pair<
    hrw::utils::hash<int>,
    type
  >>;
  hrw::utils::hash_value operator()(const value_type& t) const {
    return hrw::utils::hash<int>()(t.m_content);
  }
};

template<template<typename T> typename make_ptr>
bool operator==(t_rec<make_ptr> const & r1, t_rec<make_ptr> const & r2) {
  return (r1.m_content == r2.m_content);
}
template<template<typename T> typename make_ptr>
bool operator==(t_rec<make_ptr> const & r, int const v) {
  return (r.m_content == v);
}
template<template<typename T> typename make_ptr>
std::ostream& operator<<(std::ostream& os, t_rec<make_ptr> const & r) {
  return os << r.m_content;
}

/////////////////////////////////////////////////////////////////////////////
// SINGLETON CONTAINER
/////////////////////////////////////////////////////////////////////////////


TEST_CASE("container_single") {
  OUTPUT("==================================================================");
  OUTPUT("= container single");

  container_single<D> c;

  CHECK_FALSE(c.has());

  try {
    c.push_back(D{0});
  } catch(hrw::exception::single_container_full const& e) {
    CHECK(false);  
  }
  
  CHECK(c.has());
  CHECK(c.get() == 0);

  c.clear();
  CHECK_FALSE(c.has());

  std::vector<D> v{D{1},D{2},D{3},D{4},D{5}};

  try {
    c.insert(c.end(), v.begin(), v.end());
    CHECK(false);  
  } catch(hrw::exception::single_container_full const& e) {}
  CHECK(c.has());
  CHECK(c.get() == 1);
}


/////////////////////////////////////////////////////////////////////////////
// REGISTERY STORE
/////////////////////////////////////////////////////////////////////////////

using t_int = std::vector<int>::size_type;

template<typename R> using t_set = std::unordered_set<typename R::value_type, typename R::ref_struct::value_hash>;
template<typename R> using t_map = std::unordered_map<typename R::value_type, bool, typename R::ref_struct::value_hash>;
template<typename R> using t_vector = std::vector<typename R::value_type>;

template<typename R>
void check_registry_store_add(t_vector<R>& t, R& reg) {

  using t_reg = R;
  using value_type = typename t_reg::value_type;
  using t_ptr = typename t_reg::t_ptr;
  using t_vector_r = std::vector<t_ptr>;

  t_vector_r r;
  t_set<R> s(reg.begin(), reg.end());
  for(auto& d: t) { s.erase(d); }

  t_vector<R> tmp1(reg.begin(), reg.end());
  std::stringstream oss1;
  oss1 << wrap::print(data_wrapper<t_vector<R>&>(tmp1));
  OUTPUT(" before add: ", oss1.str());

  for(auto& d: t) { r.push_back(reg.add(value_type(d))); }

  t_vector<R> tmp2(reg.begin(), reg.end());
  std::stringstream oss2;
  oss2 << wrap::print(data_wrapper<t_vector<R>&>(tmp2));
  OUTPUT(" after add: ", oss2.str());


  for(t_int i = 0; i < r.size(); ++i) { CHECK((*(r[i])) == t[i]); }

  for(t_int i = 0; i < r.size(); ++i) {
    for(t_int j = 0; j < r.size(); ++j) {
      if(t[i] == t[j]) {
        CHECK_EQ(r[i], r[j]);
      } else {
        CHECK_NE(r[i], r[j]);
      }
    }
  }

  t_map<R> map;
  for(auto& d: t) { map[d] = false; }
  for(auto& d: reg) {
    if(s.find(d) == s.end()) {
      CHECK_NE(map.find(d), map.end());
      map[d] = true;
    }
  }

  for(auto v: map) {
    CHECK(v.second);
  }

}

template<typename R>
void check_registry_store() {
  OUTPUT(" check_registry_store(", type_name<R>(), ")");

  using t_reg = R;
  using value_type = typename t_reg::value_type;
  // using t_ptr = typename t_reg::t_ptr;
  // using t_vector_r = std::vector<t_ptr>;

  // 1. first test
  t_vector<R> t{value_type{0}, value_type{1}, value_type{0}};
  t_reg reg;
  check_registry_store_add(t, reg);

  t_vector<R> tmp1(reg.begin(), reg.end());
  std::stringstream oss1;
  oss1 << wrap::print(data_wrapper<t_vector<R>&>(tmp1));
  OUTPUT(" end: ", oss1.str());
}

template<typename Rs> struct check_registry_store_all;
template<typename ... Rs> struct check_registry_store_all<std::tuple<Rs ...>> {
  static void run() {
    ((check_registry_store<Rs>()), ...);  
  }
};


template<typename T> using get_et = typename T::element_type;
using manip_et = inner_type<get_et>;

template<typename Rs> struct check_registry_store_is_const;
template<> struct check_registry_store_is_const<std::tuple<>> { static void run() {} };
template<typename R, typename ... Rs> struct check_registry_store_is_const<std::tuple<R, Rs ...>> {
  static void run() {
    static_assert(is_registry_const_v<R>);
    check_registry_store_is_const<std::tuple<Rs ...>>::run();
  }
};



using t_reg_makes = std::tuple<
  registry_unique<std::unordered_set, true, false>,
  registry_unique<std::unordered_set, true, true>,
  registry_unique<std::unordered_set, false, false>,
  registry_unique<std::unordered_set, false, true>
>;
template<typename TM> using make_d = typename TM::template make<D, D_hash>;
using td_regs = tuple_map_t<make_d, t_reg_makes>;

template<template<typename T, typename H, typename E, template<typename> typename A> typename make_ref>
struct make_t_rec {
  template<typename T> using tmp = make_ref<T, rec_hash<T>, std::equal_to<T>, std::allocator>;
  using type = t_rec<tmp>;
  using type_hash = rec_hash<type>;
};
template<template<typename T, typename H, typename E, template<typename> typename A> typename make_ref>
using make_t_rec_t = typename make_t_rec<make_ref>::type;
template<template<typename T, typename H, typename E, template<typename> typename A> typename make_ref>
using make_t_rec_hash = typename make_t_rec<make_ref>::type_hash;
template<typename TM> using make_rec = typename TM::template make<make_t_rec_t<TM::template make_ref>, make_t_rec_hash<TM::template make_ref>>;
using trec_regs = tuple_map_t<make_rec, t_reg_makes>;


TEST_CASE("registry_unique") {
  OUTPUT("==================================================================");
  OUTPUT("= registry unique");

  check_registry_store_is_const<t_reg_makes>::run();
  check_registry_store_all<td_regs>::run();
  check_registry_store_all<trec_regs>::run();
}


/////////////////////////////////////////////////////////////////////////////
// REGISTERY NO STORE
/////////////////////////////////////////////////////////////////////////////


#endif

