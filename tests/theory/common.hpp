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

#ifndef __HREWRITE_TEST_THEORY_H__
#define __HREWRITE_TEST_THEORY_H__

#include "doctest/doctest.h"
#include "tests/common/debug.hpp"


#include "hrewrite/theory/core.hpp"

#include <vector>
#include <functional>
#include <optional>

//////////////////////////////////////////
// term_full template

namespace hrw {
  namespace tests {

    template<typename t_term_full, bool deep> struct t_hash_ref_tmp;
    template<typename t_term_full, bool deep> struct t_eq_ref_tmp;

    template<typename t_term_full> struct t_hash_ref_tmp<t_term_full, false> {
      using value_type = typename t_term_full::reference;
      hrw::utils::hash_value operator()(const value_type& t) const {
        return std::hash<value_type>()(t);
      }
    };

    template<typename t_term_full> struct t_hash_ref_tmp<t_term_full, true> {
      using value_type = typename t_term_full::reference;
      template<bool deep> using t_hash = typename t_term_full::template t_hash<deep>;
      hrw::utils::hash_value operator()(const value_type& t) const {
        return t_hash<true>()(*t);
      }
    };

    template<typename t_term_full> struct t_eq_ref_tmp<t_term_full, false> {
      using value_type = typename t_term_full::reference;
      constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
        return lhs == rhs;
      }
    };

    template<typename t_term_full> struct t_eq_ref_tmp<t_term_full, true> {
      using value_type = typename t_term_full::reference;
      template<bool deep> using t_eq = typename t_term_full::template t_eq<deep>;
      constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
        return t_eq<true>()(*lhs, *rhs);
      }
    };


    template<template<typename> typename tt_term_inner, bool arg_is_const>
    struct tt_term_full {
      using type = tt_term_full<tt_term_inner, arg_is_const>;
      using reference = type*;

      template<bool deep> struct t_hash;
      template<bool deep> struct t_eq;
      template<bool deep> struct t_hash_ref;
      template<bool deep> struct t_eq_ref;

      template<template<typename> typename t_impl> using tt_term = t_impl<type>;
      using t_content = tt_term<tt_term_inner>;

      static constexpr bool is_const = arg_is_const;

      t_sort_id get_sort() const { return this->m_content.get_sort(); }
      t_constructor_id get_constructor() const { return this->m_content.get_constructor(); }

      using t_substitution = void*;

      std::optional<t_substitution> match(reference& t) { return t_match::match_term(this, t); }

      struct t_match {
        template<typename termIt1, typename termIt2>
        static bool match_term(termIt1 p_begin, termIt1 p_end, termIt2 t_begin, termIt2 t_end, t_substitution&) {
          // since we don't have variables, we just need a one to one match
          // TODO: this implementation does not work with commutative constructors
          while(p_begin != p_end) {
            if(t_begin == t_end) {
              return false;
            }
            reference t1 = *p_begin;
            reference t2 = *t_begin;
            if(t1->match(t2)) {
              ++p_begin;
              ++t_begin;
            } else {
              return false;
            }
          }
          return true;
        }

      };

      tt_term_full(t_content c): m_content(c) {}
      t_content m_content;

      template<bool deep> struct t_hash {
        using value_type = type;
        using hash_content = typename t_content::template t_hash<deep>;
        static_assert(std::is_same_v<typename value_type::t_content, typename hash_content::value_type>);
        hrw::utils::hash_value operator()(const value_type& t) const { return hash_content()(t.m_content); }
      };

      template<bool deep> struct t_eq {
        using value_type = type;
        using eq_content = typename t_content::template t_eq<deep>;
        constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
          return eq_content()(lhs.m_content, rhs.m_content);
        }
      };

      template<bool deep> struct t_hash_ref: public t_hash_ref_tmp<type, deep> {};
      template<bool deep> struct t_eq_ref: public t_eq_ref_tmp<type, deep> {};
    };


}}


//////////////////////////////////////////
// sorts and constructors declaration


#define sort_int    0
#define sort_double 1
#define sort_string 2

struct t_sigma_manager {
  using t_letter = unsigned int;
  using t_letter_set = hrw::utils::natset;
  static t_letter get_letter(const std::string& s) { return stoi(s); }
  bool is_subletter(t_letter l, t_letter r) const { return l == r; }
};

t_sigma_manager const & get_sigma_manager();

struct t_sort_resolver {
  static std::string s_names[3];
  static const std::string& get_sort_name(typename t_sigma_manager::t_letter l) { return t_sort_resolver::s_names[l]; }
};


#define c_zero 0
#define c_incr 1
#define c_plus 2
#define c_sum  3

#define c_int 4
#define c_double 5
#define c_string 6

#define c_string_from_int 7
#define c_string_from_double 8

#define c_epsilon 9
#define c_concact 10


//////////////////////////////////////////
// base test functions

using namespace hrw;

template<typename t_term>
void check_term(t_term& t, t_sort_id s, t_constructor_id c, std::vector<t_term*> diff) {
  CHECK(t.get_sort() == s);
  CHECK(t.get_constructor() == c);
  CHECK(t.match(*t));
  for(t_term* other: diff) {
    CHECK_FALSE(t.match(other));
  }
}

template<typename t_term> using f_create = std::function<t_term()>;
template<typename t_term>
void check_equality(f_create<t_term> f) {
  using t_hash        = typename t_term::t_content::t_hash;
  using t_eq          = typename t_term::t_content::t_eq;

  t_term t1 = f();
  t_term t2 = f();

  CHECK(t1.m_content == t2.m_content);
  CHECK(t_eq()(t1.m_content, t2.m_content));
  CHECK(t_hash()(t1.m_content) == t_hash()(t2.m_content));
}


#endif // __HREWRITE_TEST_THEORY_H__

