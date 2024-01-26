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


#ifndef __HREWRITE_THEORY_LITERAL_H__
#define __HREWRITE_THEORY_LITERAL_H__

#include "hrewrite/utils.hpp"
#include "hrewrite/theory/core.hpp"


namespace hrw {
namespace theory {


    template<typename targ_theory>
    class tt_theory_literal_term {
    public:
      using type = tt_theory_literal_term<targ_theory>;
      using t_theory  = targ_theory;

      using t_value = typename t_theory::t_value;

      tt_theory_literal_term(const t_sort_id sort, const t_constructor_id c, const t_value& value): m_sort(sort), m_c(c), m_content(value) {}
      tt_theory_literal_term(type const & t): m_sort(t.m_sort), m_c(t.m_c), m_content(t.m_content) {}

      bool is_ground() const { return true; }

      t_sort_id get_sort() const { return this->m_sort; }
      t_constructor_id get_constructor() const { return this->m_c; }
      t_value& get_value() { return this->m_content; }
      const t_value& get_value() const { return this->m_content; }

      // printing
      template<typename t_context_print>
      void print(std::ostream& os, t_context_print& c) const {
        os << (c.template get_name_constructor<t_theory>(this->m_c)) << "[" << this->m_content << "]";
      }


      /////////////////////////////////////////
      // test API

      struct t_hash_core {
        using value_type = type;
        using H = hrw::utils::hash_combine<std::tuple<
          hrw::utils::hash<t_constructor_id>,
          typename t_theory::t_content_hash
        >>;
        hrw::utils::hash_value operator()(const value_type& t) {
          return H()(std::make_tuple(t.m_c, t.m_content));
        }
      };
      struct t_eq_core {
        using value_type = type;
        constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
          using t_eq = typename t_theory::t_content_eq;
          return ((lhs.m_c == rhs.m_c) && t_eq()(lhs.m_content, rhs.m_content));
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
        this->m_content = t.m_content;
        return *this;
      }

      friend void swap(type& t1, type& t2) {
        using std::swap;
        swap(t1.m_sort, t2.m_sort);
        swap(t1.m_c, t2.m_c);
        swap(t1.m_content, t2.m_content);
      }

    private:
      t_sort_id m_sort;
      t_constructor_id m_c;
      t_value m_content;
    };


    template<
      typename t_data,
      typename targ_hash=hrw::utils::hash<t_data>,
      typename targ_eq=std::equal_to<t_data>
      >
    class tt_theory_literal {
    public:
      using type = tt_theory_literal<t_data, targ_hash, targ_eq>;
      using t_value = t_data;
      using t_content_hash = targ_hash;
      using t_content_eq = targ_eq;

      // template to construct terms
      template<typename t_term_full>
      using tt_term = tt_theory_literal_term<type>;

      // term factory
      template<typename t_term_full>
      class factory {
      public:
        factory() = default;

        using t_term = tt_term<t_term_full>;

        ////////////////////////////////
        // term management

        t_term_full create_term(const t_sort_id s, const t_constructor_id c, const t_data& value) {
          return t_term_full(t_term(s, c, value));
          // return this->m_reg_term.create(this->m_reg_constructor, std::make_pair(c, value));
        }
        t_term_full create_term(const t_sort_id s, const t_constructor_id c, t_data&& value) {
          return t_term_full(t_term(s, c, std::forward<t_data>(value)));
          // return this->m_reg_term.create(this->m_reg_constructor, std::make_pair(c, std::forward<t_data>(value)));
        }

      };
    };


    template<
      typename t_data,
      typename targ_hash=hrw::utils::hash<t_data>,
      typename targ_eq=std::equal_to<t_data>
      >
    struct tp_theory_literal {
      template<typename tt, const tt&>
      using type = tt_theory_literal<t_data, targ_hash, targ_eq>;
    };

  }
}


#endif // __HREWRITE_THEORY_LITERAL_H__

