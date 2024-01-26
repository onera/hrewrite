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


#ifndef __HREWRITE_C_THEORY_H__
#define __HREWRITE_C_THEORY_H__

#include "hrewrite/utils.hpp"
#include "hrewrite/parsing.hpp"
#include "hrewrite/theory/core.hpp"

#include "hrewrite/context_sort.hpp"
#include "hrewrite/context_constructor.hpp"


namespace hrw {

  template<
    typename targ_id,                      // to identify the context
    typename targ_context_sort,
    template<typename CS, const CS&> typename targ_theory_variable, // parameter is sort manager used for specification generation
    template<typename CS, const CS&> typename ... targ_ths          // parameter is sort manager used for specification generation
  >
  class context_theory {
  public:
    static_assert(sizeof...(targ_ths) != 0, "ERROR: a theory_context needs at least one theory for structured terms");

    using type = context_theory<targ_id, targ_context_sort, targ_theory_variable, targ_ths...>;
    using t_context_sort = targ_context_sort;

  private:
    static t_context_sort m_ctx_sort; // the sort manager, that is used for all theories to parse their specifications (when this is necessary)
  public:

    // GCC PARSING BUG
    template<template<typename CS, const CS&> typename th> using tth_theory = th<targ_context_sort, m_ctx_sort>;
    using theory_variable = tth_theory<targ_theory_variable>;
    using theories_structured = std::tuple<tth_theory<targ_ths> ...>;
    // using theory_variable = targ_theory_variable<targ_context_sort, m_ctx_sort>;
    // using theories_structured = std::tuple<targ_ths<targ_context_sort, m_ctx_sort> ...>;

    using p_element = hrw::utils::element<targ_context_sort, m_ctx_sort>;
    using p_sequence = hrw::utils::sequence<targ_context_sort, m_ctx_sort>;
    template<typename natset, typename natset_core>
    using p_automata = hrw::utils::automata<natset, natset_core, targ_context_sort, m_ctx_sort>;



    using t_specs = hrw::utils::tuple_map_t<
      get_spec_t, hrw::utils::tuple_elfilter_t<has_spec, 
        hrw::utils::tuple_add_t<theory_variable, theories_structured> >>;
    using t_spec_sequence = hrw::utils::tuple_convert_t<hrw::utils::combine_sequence_from_existing, t_specs>;

    template<std::size_t idx> using theory_element_t = std::tuple_element_t<idx, theories_structured>;
    template<typename th> static constexpr std::size_t theory_index_v = hrw::utils::tuple_index_v<th, theories_structured>;
    template<typename th> static constexpr bool is_registered_stheory_v = hrw::utils::tuple_contains_v<th, theories_structured>;

    template<typename th> using th_ctx_constructor = context_constructor<get_spec_dflt_t<th>>;
    using t_ctx_constructors = typename hrw::utils::tuple_toset<typename hrw::utils::tuple_map<th_ctx_constructor, theories_structured>::type>::type;

  private:
    static t_ctx_constructors m_ctx_constructor;
  public:


    //////////////////////////////////////////
    // sorts
    static t_sort_id add_sort(const std::string& name) { return m_ctx_sort.add_sort(name); }
    static t_sort_id add_sort(std::string&& name) { return m_ctx_sort.add_sort(std::forward<std::string>(name)); }
    static void add_subsort(const t_sort_id subsort, const t_sort_id supsort) { return m_ctx_sort.add_subsort(subsort, supsort); }
    static void add_subsort(const std::string& subsort, const std::string& supsort) { return m_ctx_sort.add_subsort(subsort, supsort); }

    static bool contains_sort(const std::string& sort) { return m_ctx_sort.contains(sort); }
    static bool contains_sort(const t_sort_id sort) { return m_ctx_sort.contains(sort); }

    static bool is_subsort(const t_sort_id subsort, const t_sort_id supsort) { return m_ctx_sort.is_subsort(subsort, supsort); }
    static bool is_subsort(const std::string& subsort, const std::string& supsort) { return m_ctx_sort.is_subsort(subsort, supsort); }

    static const std::string& get_sort_name(const t_sort_id sort) { return m_ctx_sort.get_name(sort); }
    static const typename t_context_sort::natset& get_subsorts(const t_sort_id sort) { return m_ctx_sort.get_subsorts(sort); }
    static const typename t_context_sort::natset& get_supsorts(const t_sort_id sort) { return m_ctx_sort.get_supsorts(sort); }
    static const typename t_context_sort::natset& get_subsorts(const std::string& sort) { return m_ctx_sort.get_subsorts(sort); }
    static const typename t_context_sort::natset& get_supsorts(const std::string& sort) { return m_ctx_sort.get_supsorts(sort); }
    static t_sort_id get_sort_id(const std::string& sort) { return m_ctx_sort.get_letter(sort); }

    //////////////////////////////////////////
    // constructors

    template<typename th, typename ... Args>
    static t_constructor_core<th> add_constructor(const t_sort_id sort, Args ... args) {
      static_assert(is_registered_stheory_v<th>);
      return t_constructor_core<th>(std::get<th_ctx_constructor<th>>(m_ctx_constructor).add_constructor(sort, args ...));
    }
    template<typename th, typename ... Args>
    static t_constructor_core<th> add_constructor(const std::string& sort, Args ... args) {
      static_assert(is_registered_stheory_v<th>);
      return t_constructor_core<th>(std::get<th_ctx_constructor<th>>(m_ctx_constructor).add_constructor(m_ctx_sort.get_letter(sort), args ...));
    }

    template<typename th>
    static bool contains_constructor(const t_constructor_core<th> c) {
      static_assert(is_registered_stheory_v<th>);
      return std::get<th_ctx_constructor<th>>(m_ctx_constructor).contains(c.id());
    }

    template<typename th>
    static t_sort_id get_sort(const t_constructor_core<th> c) {
      static_assert(is_registered_stheory_v<th>);
      return std::get<th_ctx_constructor<th>>(m_ctx_constructor).get_sort(c.id());
    }

    template<typename th>
    static t_constructor_key get_key(const t_constructor_core<th> c) {
      static_assert(is_registered_stheory_v<th>);
      return std::make_pair(theory_index_v<th>, c.id());
    }

    template<typename th>
    static const std::string& get_name(const t_constructor_core<th> c) {
      static_assert(is_registered_stheory_v<th>);
      return std::get<th_ctx_constructor<th>>(m_ctx_constructor).get_name(c.id());
    }

    template<typename th>
    static const typename th::t_spec& get_spec(const t_constructor_core<th> c) {
      static_assert(is_registered_stheory_v<th>);
      static_assert(has_spec_v<th>);
      return std::get<th_ctx_constructor<th>>(m_ctx_constructor).get_spec(c.id());
    }


    //////////////////////////////////////////
    // clear
    static void clear() {
      m_ctx_sort.clear();
      std::apply([](auto& ... ctx_constructor) {
        (ctx_constructor.clear(), ...);
      }, m_ctx_constructor);

    }

  };


  /////////////////////////////////////////////////////////////////////////////
  // IMPLEMENTATION
  /////////////////////////////////////////////////////////////////////////////


  template<
    typename id, // to identify the context
    typename targ_context_sort,
    template<typename CS, const CS&> typename theory_variable,
    template<typename CS, const CS&> typename ... ths 
  >
  targ_context_sort context_theory<id, targ_context_sort, theory_variable, ths ...>::m_ctx_sort;

  template<
    typename id, // to identify the context
    typename targ_context_sort,
    template<typename CS, const CS&> typename theory_variable,
    template<typename CS, const CS&> typename ... ths 
  >
  typename context_theory<id, targ_context_sort, theory_variable, ths ...>::t_ctx_constructors context_theory<id, targ_context_sort, theory_variable, ths ...>::m_ctx_constructor;

}


#endif // __HREWRITE_C_THEORY_H__

