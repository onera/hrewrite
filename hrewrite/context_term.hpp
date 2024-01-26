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


#ifndef __HREWRITE_C_TERM_H__
#define __HREWRITE_C_TERM_H__

#include "hrewrite/utils.hpp"
#include "hrewrite/theory/core.hpp"
#include "hrewrite/exceptions/parsing.hpp"


namespace hrw {

  template<typename targ_ctx_theory, typename targ_registry> // targ_registry is set-like
  class context_term {
  public:
    using type = context_term<targ_ctx_theory, targ_registry>;
    using ctx_theory = targ_ctx_theory;
    using t_registry = targ_registry;

    static inline constexpr bool ensure_unique_v = t_registry::ensure_unique_v;
    static inline constexpr bool term_const_v = hrw::utils::is_registry_const_v<t_registry>;

    template<typename T> using tt_term_hash     = typename T::template t_hash<!ensure_unique_v>;
    template<typename T> using tt_term_eq       = typename T::template t_eq<!ensure_unique_v>;
    template<typename T> using tt_term_hash_ref = typename T::template t_hash_ref<!ensure_unique_v>;
    template<typename T> using tt_term_eq_ref   = typename T::template t_eq_ref<!ensure_unique_v>;


    template<typename T> using t_make_ref = typename t_registry::template make_ref<T, tt_term_hash<T>, tt_term_eq<T>>;

    // ensures consistency in the registry specification
    static_assert((!ensure_unique_v) || term_const_v);

    using theory_variable = typename ctx_theory::theory_variable;
    using theories_structured = typename ctx_theory::theories_structured;

    using t_variable      = typename theory_variable::t_term;
    template<typename ... ths> using make_term_full = tt_term_full<t_make_ref, ctx_theory, theory_variable::template tt_substitution, ths::template tt_term ...>;
    using t_term_full     = hrw::utils::tuple_convert_t<make_term_full, theories_structured>;
    using t_term_full_ref = typename t_term_full::reference;
    using t_substitution  = typename t_term_full::t_substitution;

    using t_term_hash     = tt_term_hash    <t_term_full>;
    using t_term_eq       = tt_term_eq      <t_term_full>;
    using t_term_hash_ref = tt_term_hash_ref<t_term_full>;
    using t_term_eq_ref   = tt_term_eq_ref  <t_term_full>;


    template<typename th> using th_factory = typename th::template factory<t_term_full>;
    using t_vfactory = th_factory<theory_variable>;
    using t_sfactories = hrw::utils::tuple_map_t<th_factory, theories_structured>; // all structured theories
    template<std::size_t N> using factory_selement = std::tuple_element<N, t_sfactories>;



    context_term() = default;
    context_term(const context_term&) = delete;


    //////////////////////////////////////////
    // term creation

    template<typename ... Args>
    t_term_full_ref create_vterm(Args ... args) {
      return this->m_registry.add(this->m_vfactory.create_term(args ...));
    }

    template<typename th, typename ... Args>
    t_term_full_ref create_sterm(const t_constructor_core<th> c, Args ... args) {
      static_assert(ctx_theory::template is_registered_stheory_v<th>);
      using tl_factory = th_factory<th>;
      t_sort_id s = ctx_theory::get_sort(c);

      if constexpr(has_spec_v<th>) { // the theory has a spec
        return this->m_registry.add(std::get<tl_factory>(this->m_sfactories).template create_term<typename ctx_theory::t_spec_sequence>(ctx_theory::get_spec(c), s, c.id(), args ...));
      } else {
        return this->m_registry.add(std::get<tl_factory>(this->m_sfactories).create_term(s, c.id(), args ...));
      }
    }

    template<typename th, typename ... Args>
    t_term_full_ref create_sterm_no_check(const t_constructor_core<th> c, Args ... args) {
      static_assert(ctx_theory::template is_registered_stheory_v<th>);
      using tl_factory = th_factory<th>;
      t_sort_id s = ctx_theory::get_sort(c);
      return this->m_registry.add(std::get<tl_factory>(this->m_sfactories).create_term(s, c.id(), args ...));
    }


    template<typename t_term>
    t_term_full_ref create_sterm_from_diff(const t_term& t, typename t_term::t_container&& c) {
      using th = typename t_term::t_theory;
      static_assert(ctx_theory::template is_registered_stheory_v<th>);
      using tl_factory = th_factory<th>;
      return this->m_registry.add(std::move(std::get<tl_factory>(this->m_sfactories).create_term_from_diff(t, std::move(c))));
    }


    //////////////////////////////////////////
    // useful wrapper on ctx_theory

    // the static methods for managing constructors
    template<typename th, typename ... Args>
    t_constructor<th, type> add_constructor(const t_sort_id sort, Args ... args) {
      return t_constructor(ctx_theory::template add_constructor<th>(sort, args...), *this);
    }
    template<typename th, typename ... Args>
    t_constructor<th, type> add_constructor(const std::string& sort, Args ... args) {
      return t_constructor(ctx_theory::template add_constructor<th>(sort, args...), *this);
    }

    template<typename th>
    static bool contains_constructor(const t_constructor<th, type> c) {
      return ctx_theory::template contains_constructor<th>(c.core());
    }

    template<typename th>
    static t_sort_id get_sort(const t_constructor_core<th> c) {
      return ctx_theory::template get_sort<th>(c.core());
    }

    template<typename th>
    static t_constructor_key get_key(const t_constructor_core<th> c) {
      return ctx_theory::template get_key<th>(c.core());
    }

    template<typename th>
    static const std::string& get_name(const t_constructor_core<th> c) {
      return ctx_theory::template get_name<th>(c.core());
    }


    //////////////////////////////////////////
    // instantiation

    const std::string& get_spec(t_term_full_ref t) { return t->template get_spec<ctx_theory>(); }

    template<typename T>
    static bool is_instance_of(t_term_full_ref t, const T& spec) {
      if constexpr(std::is_same_v<T, t_sort_id>) {
        typename ctx_theory::p_element spec_real(ctx_theory::get_sort_name(spec));
        return t->visit(is_instance_of_helper<typename ctx_theory::p_element>{spec_real});
      } else if constexpr(std::is_convertible_v<T, std::string>) {
        if(hrw::utils::is_element(spec)) {
          typename ctx_theory::p_element spec_real(spec);
          return t->visit(is_instance_of_helper<typename ctx_theory::p_element>{spec_real});
        } else if(hrw::utils::is_sequence(spec)) {
          typename ctx_theory::p_sequence spec_real(spec);
          return t->visit(is_instance_of_helper<typename ctx_theory::p_sequence>{spec_real});
        } else if(hrw::utils::is_regexp(spec)) {
          using p_automata = typename ctx_theory::template p_automata<hrw::utils::natset, hrw::utils::natset>;
          p_automata spec_real(spec);
          return t->visit(is_instance_of_helper<p_automata>{spec_real});
        } else {

          throw hrw::exception::spec_invalid(spec);
        }
      } else { // suppose a parser
        return t->visit(is_instance_of_helper{spec});
      }
    }

    t_term_full_ref instantiate(t_term_full_ref pattern, t_substitution& subst) {
      // using t_print = t_hterm_print<typename type::ctx_theory, t_term_full>;
      // t_print ctx_print;
      // std::cout << "context_term::instantiate(" << ctx_print.print(pattern) << ", " << ctx_print.print(subst) << ")" << std::endl << std::flush;
      return t_term_full::t_instantiate::template instantiate<type>(*this, pattern, subst);
    }


    //////////////////////////////////////////
    // useful methods

    bool contains(const t_term_full_ref t) const { return this->m_registry.contains(*t); }
    void clear() { this->m_registry.clear(); }


    using term_make_ref = typename t_registry::template make_ref<t_term_full, t_term_hash, t_term_eq>;

  private:
    using term_registry = typename t_registry::template make<t_term_full, t_term_hash, t_term_eq>;
    static_assert(std::is_same_v<t_term_full_ref, typename term_registry::t_ptr>);

    //////////////////////////////////////////
    // term registry
    term_registry m_registry;

    //////////////////////////////////////////
    // variable and structured term factories
    t_vfactory m_vfactory;
    t_sfactories m_sfactories;

    template<typename t_spec>
    struct is_instance_of_helper {
      const t_spec& m_spec;
      is_instance_of_helper(const t_spec& spec): m_spec(spec) {}
      bool operator()(const t_variable& v) const {
        using t_spec_main = typename t_variable::t_spec;
        using t_inclusion = hrw::utils::t_inclusion<t_spec_main, t_spec>;
        return t_inclusion::check(v.get_spec(), this->m_spec);
      }
      template<typename T>
      bool operator()(const T& t) const {
        using t_spec_main = typename ctx_theory::p_element;
        using t_inclusion = hrw::utils::t_inclusion<t_spec_main, t_spec>;
        t_spec_main spec_main(ctx_theory::get_sort_name(t.get_sort()));
        return t_inclusion::check(spec_main, this->m_spec);
      }
    };
  };


}


#endif // __HREWRITE_C_TERM_H__

