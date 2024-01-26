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


#ifndef __HREWRITE_THEORY_CORE_H__
#define __HREWRITE_THEORY_CORE_H__

#include "hrewrite/utils.hpp"


namespace hrw {

  //////////////////////////////////////////
  // base types

  // the type of sort ids
  using t_sort_id = unsigned int;
  // the type of constructor ids
  using t_constructor_id = unsigned int;


  enum e_rw_status { NONE=0, SHALLOW=1, FULL=3 };
  inline std::ostream& operator<<(std::ostream& os, const e_rw_status& v) {
    switch(v) {
      case NONE   : os << "NONE"   ; break;
      case SHALLOW: os << "SHALLOW"; break;
      case FULL   : os << "FULL"   ; break;
    }
    return os;
  }

  //////////////////////////////////////////
  // constructor types

  template<typename th>
  class t_constructor_core {
  public:
    t_constructor_core(t_constructor_id idx): m_idx(idx) {}
    t_constructor_id id() const { return this->m_idx; }
  private:
    const t_constructor_id m_idx;
  };

  template<typename th, typename t_ctx_term>
  class t_constructor {
  public:
    t_constructor(const t_constructor_core<th> c, t_ctx_term& ctx): m_c(c), m_ctx(ctx) {}
    t_constructor_core<th> core() const { return this->m_c; }
    template<typename ... Args>
    typename t_ctx_term::t_term_full_ref operator()(Args ... args) { return this->m_ctx.create_sterm(this->m_c, args...); }
  private:
    const t_constructor_core<th> m_c;
    t_ctx_term& m_ctx;
  };

  using t_constructor_key = std::pair<std::size_t, t_constructor_id>;


  //////////////////////////////////////////
  // structuring templates

  namespace _detail_struct {
    template<typename T> using _spec = typename T::t_spec;
    using spec = hrw::utils::inner_type<_spec>;

    template<typename T> using _theory = typename T::t_theory;
    using theory = hrw::utils::inner_type<_theory>;

    template<typename th, typename t_term_full>
    using term = typename th::template tt_term<typename t_term_full::reference>;

    template<typename T> using _container = typename T::t_container;
    using container = hrw::utils::inner_type<_container>;

    template<typename T> using _iterator = typename T::t_iterator;
    using iterator = hrw::utils::inner_type<_iterator>;

    template<typename T> using _value = typename T::t_value;
    using value = hrw::utils::inner_type<_value>;

    template<typename T> using _t_id = typename T::t_id;
    using t_id = hrw::utils::inner_type<_t_id>;
  }

  template<typename T> using has_spec = _detail_struct::spec::template has<T>;
  template<typename T> constexpr bool has_spec_v = _detail_struct::spec::template has_v<T>;
  template<typename T> using get_spec_t = typename _detail_struct::spec::template get_t<T>;
  template<typename T> using get_spec_dflt_t = typename _detail_struct::spec::template dflt<void>::template get_t<T>;

  template<typename T> using has_theory = _detail_struct::theory::template has<T>;
  template<typename T> constexpr bool has_theory_v = _detail_struct::theory::template has_v<T>;
  template<typename T> using get_theory_t = typename _detail_struct::theory::template get_t<T>;

  template<typename T> using has_container = _detail_struct::container::template has<T>;
  template<typename T> constexpr bool has_container_v = _detail_struct::container::template has_v<T>;
  template<typename T> using get_container_t = typename _detail_struct::container::template get_t<T>;
  template<typename T> using get_iterator_t = typename _detail_struct::iterator::template get_t<T>;

  template<typename T> using has_value = _detail_struct::value::template has<T>;
  template<typename T> constexpr bool has_value_v = _detail_struct::value::template has_v<T>;
  template<typename T> using get_value_t = typename _detail_struct::value::template get_t<T>;

  template<typename T> constexpr bool has_t_id_v = _detail_struct::t_id::template has_v<T>;


  template<template<typename> typename tth, template<typename tt, const tt&> typename P>
  struct tth_wrap_to_parser {
    template <typename tt, const tt& v>
    using type = tth<P<tt, v>>;
  };

}


#endif // __HREWRITE_THEORY_CORE_H__

