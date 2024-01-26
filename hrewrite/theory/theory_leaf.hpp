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


#ifndef __HREWRITE_THEORY_LEAF_H__
#define __HREWRITE_THEORY_LEAF_H__

#include <functional>

#include "hrewrite/utils.hpp"
#include "hrewrite/theory/core.hpp"


namespace hrw {
namespace theory {


    template<typename targ_theory>
    class tt_theory_leaf_term {
    public:
      using type = tt_theory_leaf_term<targ_theory>;
      using t_theory  = targ_theory;

      tt_theory_leaf_term(const t_sort_id sort, const t_constructor_id c): m_sort(sort), m_c(c) {}
      tt_theory_leaf_term(type const & t): m_sort(t.m_sort), m_c(t.m_c) {}

      bool is_ground() const { return true; }

      t_sort_id get_sort() const { return this->m_sort; }
      t_constructor_id get_constructor() const { return this->m_c; }

      // printing
      template<typename t_context_print>
      void print(std::ostream& os, t_context_print& c) const {
        os << (c.template get_name_constructor<t_theory>(this->m_c));
      }


      /////////////////////////////////////////
      // test API

      struct t_hash_core {
        using value_type = type;
        // using H = hrw::utils::hash_combine<std::pair<
        //   hrw::utils::hash<t_sort_id>,
        //   hrw::utils::hash<t_constructor_id>
        // >>;
        using H = hrw::utils::hash<t_constructor_id>;
        hrw::utils::hash_value operator()(const value_type& t) {
          // return H()(std::make_pair(t.m_sort, t.m_c));
          return H()(t.m_c);
        }
      };
      struct t_eq_core {
        using value_type = type;
        constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
          return (lhs.m_c == rhs.m_c);
        }
      };

      template<bool deep> using t_hash = t_hash_core;
      template<bool deep> using t_eq = t_eq_core;


      /////////////////////////////////////////
      // rewrite API

      bool match_shallow(const type& t) const {
        return t_eq_core()(*this, t);
      }

      template<typename t_substitution, typename t_match>
      bool match_subterms(const type&, t_substitution&, t_match&) const {
        return true;
      }

      template<typename t_substitution, typename t_match>
      bool match(const type& t, t_substitution&, t_match&) const {
        return this->match_shallow(t);
      }


      /////////////////////////////////////////
      // mutable API

      type& operator=(type const & t) {
        this->m_sort = t.m_sort;
        this->m_c = t.m_c;
        return *this;
      }

      friend void swap(type& t1, type& t2) {
        using std::swap;
        swap(t1.m_sort, t2.m_sort);
        swap(t1.m_c, t2.m_c);
      }


    private:
      t_sort_id m_sort;
      t_constructor_id m_c;
    };


    class t_theory_leaf {
    public:
      using type = t_theory_leaf;

      // template to construct terms
      template<typename t_term_full>
      using tt_term = tt_theory_leaf_term<type>;

      // term factory
      template<typename t_term_full>
      class factory {
      public:
        factory() = default;

        using t_term = tt_term<t_term_full>;


        ////////////////////////////////
        // term management

        t_term_full create_term(const t_sort_id s, const t_constructor_id c) {
          return t_term_full(t_term(s, c));
        }

      };
    };

    struct tp_theory_leaf {
      template<typename tt, const tt&>
      using type = t_theory_leaf;
    };

  }
}


#endif // __HREWRITE_THEORY_LEAF_H__

