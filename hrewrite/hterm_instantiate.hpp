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


#ifndef __HREWRITE_HTERM_INSTANTIATE_H__
#define __HREWRITE_HTERM_INSTANTIATE_H__


#include "hrewrite/utils/variant.hpp"

#include "hrewrite/hterm_print.hpp"
#include <iostream>

namespace hrw {


  ///////////////////////////////////////////////////////////////////////////////
  // ELEMENT IMPLEMENTATION
  ///////////////////////////////////////////////////////////////////////////////

  template<typename tt_term_full>
  class tt_term_full_instantiate_element {
  public:
    using type = tt_term_full_instantiate_element<tt_term_full>;
    using t_variable  = typename tt_term_full::t_variable;
    using t_substitution  = typename tt_term_full::t_substitution;
    using reference = typename tt_term_full::reference;

    static constexpr bool is_const_v = tt_term_full::is_const_v;

    template<typename t_ctx_term>
    static reference instantiate(t_ctx_term& rweng, reference t, t_substitution& subst) {
      instantiate_helper<t_ctx_term> obj(rweng, t, subst);
      return VISIT_SINGLE(obj, t->m_content);
    }

    template<typename t_ctx_term, typename t_container>
    static void instantiate(t_ctx_term& rweng, reference t, t_substitution& subst, t_container& container) {
      reference res = type::template instantiate<t_ctx_term>(rweng, t, subst);
      container.push_back(res);
    }

  private:

    template<typename t_ctx_term>
    struct instantiate_helper{
      using t_print = t_hterm_print<typename t_ctx_term::ctx_theory, tt_term_full>;

      instantiate_helper(t_ctx_term& rweng, reference t, t_substitution& subst):
        m_rweng(rweng), m_t(t), m_subst(subst) {}

      template<typename T>
      reference operator()(T& t) {
        using t_term_core = std::remove_cv_t<T>; // the type stored in the t_content variant
        // using t_term_cv = std::conditional_t<is_const_v, std::add_const_t<t_term_core>, t_term_core>; // the type as it is used (if is_const_v, then the iterators are const)
        if constexpr(std::is_same_v<t_term_core, t_variable>) {
          bool contains = this->m_subst.contains(&t);
          if(contains) {
            return this->m_subst.get(&t).value();  
          } else {
            return this->m_t;
          }
        } else if constexpr (has_container_v<t_term_core>) {
          using t_container_other = get_container_t<T>;
          t_container_other container;
          for(reference s: t) {
            instantiate_helper<t_ctx_term> obj(this->m_rweng, s, this->m_subst);
            container.push_back(VISIT_SINGLE(obj, s->m_content));
          }
          return this->m_rweng.template create_sterm_from_diff<T>(t, std::move(container));
        } else {
          return this->m_t;
        }
      }

    private:
      t_ctx_term& m_rweng;
      reference m_t;
      t_substitution& m_subst;
    };

  };


  ///////////////////////////////////////////////////////////////////////////////
  // LIST IMPLEMENTATION
  ///////////////////////////////////////////////////////////////////////////////

  template<typename tt_term_full>
  class tt_term_full_instantiate_sequence {
  public:
    using type = tt_term_full_instantiate_sequence<tt_term_full>;
    using t_variable  = typename tt_term_full::t_variable;
    using t_substitution  = typename tt_term_full::t_substitution;
    using reference = typename tt_term_full::reference;

    static constexpr bool is_const_v = tt_term_full::is_const_v;

    template<typename t_ctx_term>
    static reference instantiate(t_ctx_term& rweng, reference t, t_substitution& subst) {
      using t_container = hrw::utils::container_single<reference>;
      t_container container;
      type::template instantiate<t_ctx_term, t_container>(rweng, t, subst, container);
      return container.get();
    }

    template<typename t_ctx_term, typename t_container>
    static void instantiate(t_ctx_term& rweng, reference t, t_substitution& subst, t_container& container) {
      instantiate_helper<t_ctx_term, t_container> obj(rweng, t, subst, container);
      VISIT_SINGLE(obj, t->m_content);
    }

  private:

    template<typename t_ctx_term, typename t_container>
    struct instantiate_helper{
      instantiate_helper(t_ctx_term& rweng, reference t, t_substitution& subst, t_container& container):
        m_rweng(rweng), m_t(t), m_subst(subst), m_container(container) {}

      template<typename T>
      void operator()(T& t) {
        using t_term_core = std::remove_cv_t<T>; // the type stored in the t_content variant
        // using t_term_cv = std::conditional_t<is_const_v, std::add_const_t<t_term_core>, t_term_core>; // the type as it is used (if is_const_v, then the iterators are const)
        if constexpr(std::is_same_v<t_term_core, t_variable>) {
          if(!this->m_subst.retrieve(&t, this->m_container)) {
            this->m_container.push_back(this->m_t);
          }
        } else if constexpr (has_container_v<t_term_core>) {
          using t_container_other = get_container_t<T>;
          t_container_other container;
          for(reference s: t) {
            instantiate_helper<t_ctx_term, t_container_other> obj(this->m_rweng, s, this->m_subst, container);
            VISIT_SINGLE(obj, s->m_content);
          }
          this->m_container.push_back(this->m_rweng.template create_sterm_from_diff<T>(t, std::move(container)));
        } else {
          this->m_container.push_back(this->m_t);
        }
      }

    private:
      t_ctx_term& m_rweng;
      reference m_t;
      t_substitution& m_subst;
      t_container& m_container;
    };

  };


  ///////////////////////////////////////////////////////////////////////////////
  // COMBINER
  ///////////////////////////////////////////////////////////////////////////////

  template<typename tt_term_full, typename=void> struct tt_term_full_instantiate;

  template<typename tt_term_full> struct tt_term_full_instantiate<tt_term_full, std::enable_if_t<
    tt_term_full::t_variable::t_spec::complexity == hrw::utils::parsing_complexity::ELEMENT, void>>:
      public tt_term_full_instantiate_element<tt_term_full> {};

  template<typename tt_term_full> struct tt_term_full_instantiate<tt_term_full, std::enable_if_t<
    tt_term_full::t_variable::t_spec::complexity >= hrw::utils::parsing_complexity::SEQUENCE, void>>:
      public tt_term_full_instantiate_sequence<tt_term_full> {};

}


#endif // __HREWRITE_HTERM_INSTANTIATE_H__

