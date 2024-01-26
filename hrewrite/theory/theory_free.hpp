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


#ifndef __HREWRITE_THEORY_FREE_H__
#define __HREWRITE_THEORY_FREE_H__

#include <vector>
#include <utility>
#include <iostream>
#include <functional>
#include <algorithm>
#include <optional>

#include "hrewrite/utils.hpp"
#include "hrewrite/parsing.hpp"
#include "hrewrite/theory/core.hpp"
#include "hrewrite/exceptions/theory_free.hpp"


namespace hrw {
namespace theory {

    template<typename targ_theory, typename t_term_full, template<typename...> typename tt_container>
    class tt_theory_free_term {
    public:
      using type = tt_theory_free_term<targ_theory, t_term_full, tt_container>;
      using t_term_full_ref = typename t_term_full::reference;
      using t_theory  = targ_theory;

      using t_container = tt_container<t_term_full_ref>;
      using t_iterator = typename t_container::iterator;
      using t_const_iterator = typename t_container::const_iterator;

      tt_theory_free_term(const t_sort_id sort, const t_constructor_id c, const t_container& subs) : m_sort(sort), m_c(c), m_subs(subs) {}
      tt_theory_free_term(const t_sort_id sort, const t_constructor_id c, t_container&& subs) : m_sort(sort), m_c(c), m_subs(subs) {}
      tt_theory_free_term(type const & t) : m_sort(t.m_sort), m_c(t.m_c), m_subs(t.m_subs) {}

      bool is_ground() const { return std::all_of(this->begin(), this->end(), [](auto it) { return it->is_ground(); }); }

      t_sort_id get_sort() const { return this->m_sort; }
      t_constructor_id get_constructor() const { return this->m_c; }


      t_iterator begin() { return this->m_subs.begin(); }
      t_iterator end()   { return this->m_subs.end(); }

      t_const_iterator begin() const { return this->m_subs.cbegin(); }
      t_const_iterator end()   const { return this->m_subs.cend(); }

      const t_container& get_subterms() const { return this->m_subs; }
      std::size_t size() const { return this->m_subs.size(); }

      // printing
      template<typename t_context_print>
      void print(std::ostream& os, t_context_print& c) const {
        if(this->m_subs.empty()) {
          os << (c.template get_name_constructor<t_theory>(this->m_c));
        } else {
          os << (c.template get_name_constructor<t_theory>(this->m_c)) << "(";
          auto it = this->m_subs.cbegin();
          do {
            (*it)->print(os, c);
            ++it;
            if(it != this->m_subs.cend()) {
              os << ", ";
            }
          } while(it != this->m_subs.cend());
          os << ")";
        }
      }



      /////////////////////////////////////////
      // rewrite API

      template<typename t_substitution, typename t_match>
      bool match_subterms(type& t, t_substitution& subst, t_match& m) {
        return t_theory::template factory<t_term_full>::match_term_subterms(*this, t, subst, m);
      }

      template<typename t_substitution, typename t_match>
      bool match(type& t, t_substitution& subst, t_match& m) {
        return t_theory::template factory<t_term_full>::match_term(*this, t, subst, m);
      }


      bool match_shallow(const type& t) const {
        return t_theory::template factory<t_term_full>::match_term_shallow(*this, t);
      }

      template<typename t_substitution, typename t_match>
      bool match_subterms(const type& t, t_substitution& subst, t_match& m) const {
        return t_theory::template factory<t_term_full>::match_term_subterms(*this, t, subst, m);
      }

      template<typename t_substitution, typename t_match>
      bool match(const type& t, t_substitution& subst, t_match& m) const {
        return t_theory::template factory<t_term_full>::match_term(*this, t, subst, m);
      }


      /////////////////////////////////////////
      // test API

      template<bool deep> struct t_hash {
        using value_type = type;
        using H = hrw::utils::hash_combine<std::tuple<
          // hrw::utils::hash<t_sort_id>,
          hrw::utils::hash<t_constructor_id>,
          hrw::utils::hash_combine<std::vector<typename t_term_full::template t_hash_ref<deep>>>
        >>;
        hrw::utils::hash_value operator()(const value_type& t) {
          // return H()(std::make_tuple(t.m_sort, t.m_c, t.m_subs));
          return H()(std::make_tuple(t.m_c, t.m_subs));
        }
      };
      template<bool deep> struct t_eq {
        using value_type = type;
        using t_eq_inner = typename t_term_full::template t_eq_ref<deep>;
        constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
          if((lhs.m_c == rhs.m_c) && (lhs.m_subs.size() == rhs.m_subs.size())) {
            t_eq_inner obj;
            for(unsigned int i = 0; i < lhs.m_subs.size(); ++i) {
              if(!obj(lhs.m_subs[i], rhs.m_subs[i])) {
                return false;
              }
            }
            return true;
          } else {
            return false;
          }
        }
      };


      /////////////////////////////////////////
      // mutable API

      type& operator=(type const & t) {
        this->m_sort = t.m_sort;
        this->m_c = t.m_c;
        this->m_subs = t.m_subs;
        return *this;
      }

      friend void swap(type& t1, type& t2) {
        using std::swap;
        swap(t1.m_sort, t2.m_sort);
        swap(t1.m_c, t2.m_c);
        swap(t1.m_subs, t2.m_subs);
      }

    private:
      t_sort_id m_sort;
      t_constructor_id m_c;
      t_container m_subs;
    };



    template<typename targ_spec, template<typename...> typename targ_container>
    class tt_theory_free {
    public:
      using type = tt_theory_free<targ_spec, targ_container>;
      using t_spec = targ_spec;

      // template to construct terms
      template<typename t_term_full>
      using tt_term = tt_theory_free_term<type, t_term_full, targ_container>;

      // term factory
      template<typename t_term_full>
      class factory {
      public:
        factory() = default;

        using t_term = tt_term<t_term_full>;
        using t_container = typename t_term::t_container;
        using t_iterator = typename t_term::t_iterator;
        using t_const_iterator = typename t_term::t_const_iterator;

        ////////////////////////////////
        // term management

        // check
        template<typename t_spec_sequence>
        t_term_full create_term(const t_spec& spec, const t_sort_id s, const t_constructor_id c, t_container& subs) {
          std::string regexp = "";
          for(const auto& st: subs) { regexp = regexp + " " + (st->get_spec()); }
          t_spec_sequence check(regexp);
          if(hrw::utils::inclusion(check, spec)) {
            return t_term_full(t_term(s, c, subs));
          } else {
            throw hrw::exception::th_free_construct<typename t_term_full::reference>(c, spec.get_regexp(), regexp);
          }
        }
        template<typename t_spec_sequence>
        t_term_full create_term(const t_spec& spec, const t_sort_id s, const t_constructor_id c, t_container&& subs) {
          std::string regexp = "";
          for(const auto& st: subs) { regexp = regexp + " " + (st->get_spec()); }
          t_spec_sequence check(regexp);
          if(hrw::utils::inclusion(check, spec)) {
            return t_term_full(t_term(s, c, std::move(subs)));
          } else {
            throw hrw::exception::th_free_construct<typename t_term_full::reference>(c, spec.get_regexp(), regexp);
          }
        }

        // no check
        t_term_full create_term(const t_sort_id s, const t_constructor_id c, t_container& subs) {
          return t_term_full(t_term(s, c, subs));
        }
        t_term_full create_term(const t_sort_id s, const t_constructor_id c, t_container&& subs) {
          return t_term_full(t_term(s, c, std::move(subs)));
        }

        t_term_full create_term_from_diff(const t_term t, t_container&& subs) {
          return t_term_full(t_term(t.get_sort(), t.get_constructor(), std::forward<t_container>(subs)));
        }


        ////////////////////////////////
        // term matching
        using t_substitution = typename t_term_full::t_substitution;
        using t_match =  typename t_term_full::t_match;

        template<typename T, std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, t_term>, bool> = true>
        static bool match_term_shallow(T& pattern, T& t) {
          // return (pattern.get_sort() == t.get_sort()) && (pattern.get_constructor() == t.get_constructor());
          return (pattern.get_constructor() == t.get_constructor());
        }

        template<typename T, typename t_match, std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, t_term>, bool> = true>
        static bool match_term_subterms(T& pattern, T& t, t_substitution& subst, t_match& m) {
          auto p_begin = pattern.begin();
          auto p_end   = pattern.end();
          auto t_begin = t.begin();
          auto t_end   = t.end();

          return m.match_term(p_begin, p_end, t_begin, t_end, subst);          
        }


        template<typename T, typename t_match, std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, t_term>, bool> = true>
        static bool match_term(T& pattern, T& t, t_substitution& subst, t_match& m) {
          using c_type = typename type::template factory<t_term_full>;
          if(c_type::match_term_shallow(pattern, t)) {
            return c_type::match_term_subterms(pattern, t, subst, m);
          } else {
            return false;
          }
        }
      };
    };


    template<template<typename...> typename targ_container, template<typename tt, const tt&> typename P>
    struct tp_theory_free {
      template <typename tt, const tt& v>
      using type = tt_theory_free<P<tt, v>, targ_container>;
    };

  }
}


#endif // __HREWRITE_THEORY_FREE_H__

