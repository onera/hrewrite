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


#ifndef __HREWRITE_HTERM_H__
#define __HREWRITE_HTERM_H__


#include <tuple>
#include <variant>
#include <utility>
#include <iostream>
#include <sstream>


#include "hrewrite/utils.hpp"
#include "hrewrite/exceptions/common.hpp"

#include "hrewrite/parsing.hpp"
#include "hrewrite/theory/core.hpp"
#include "hrewrite/theory/theory_variable.hpp"

#include "hrewrite/hterm_match.hpp"
#include "hrewrite/hterm_instantiate.hpp"

namespace hrw {


  /////////////////////////////////////////////////////////////////////////////
  // INTRODUCTION
  /////////////////////////////////////////////////////////////////////////////

  // static std::size_t nb_term = 0;

  namespace __detail__ {
    template<typename T, bool deep> struct t_term_hash_ref;
    template<typename T, bool deep> struct t_term_eq_ref;



    template<typename T> struct t_term_hash_ref<T, false> {
      using value_type = typename T::reference;
      using make_ptr_struct = typename T::make_ptr_struct;
      hrw::utils::hash_value operator()(const value_type& t) const {
        // std::cout << "t_term_hash_ref<T, false>" << std::endl;
        return std::hash<const T*>()(make_ptr_struct::to_ptr(t));
      }
    };

    template<typename T> struct t_term_hash_ref<T, true> {
      using value_type = typename T::reference;
      using make_ptr_struct = typename T::make_ptr_struct;
      template<bool b> using t_hash = typename T::template t_hash<b>;
      hrw::utils::hash_value operator()(const value_type& t) const {
        // std::cout << "t_term_hash_ref<T, true>" << std::endl;
        return t_hash<true>()(*make_ptr_struct::to_ptr(t));
      }
    };

    template<typename T> struct t_term_eq_ref<T, false> {
      using value_type = typename T::reference;
      using make_ptr_struct = typename T::make_ptr_struct;
      constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
        // std::cout << "t_term_eq_ref<T, false>" << std::endl;
        return make_ptr_struct::to_ptr(lhs) == make_ptr_struct::to_ptr(rhs);
      }
    };

    template<typename T> struct t_term_eq_ref<T, true> {
      using value_type = typename T::reference;
      using make_ptr_struct = typename T::make_ptr_struct;
      template<bool b> using t_eq = typename T::template t_eq<b>;
      constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
        // std::cout << "t_term_eq_ref<T, true>" << std::endl;
        if(make_ptr_struct::to_ptr(lhs) == make_ptr_struct::to_ptr(rhs)) {
          return true;
        } else {
          return t_eq<true>()(*make_ptr_struct::to_ptr(lhs), *make_ptr_struct::to_ptr(rhs));
        }
      }
    };
  }


  /////////////////////////////////////////////////////////////////////////////
  // MAIN CLASS
  /////////////////////////////////////////////////////////////////////////////


  template<template<typename ...> typename t_make_ref, typename t_sort_resolver, template<typename, typename...> typename tt_substitution, template<typename> typename ... tt_sterms>
  class tt_term_full {         // type of term. We need to be able to access sort. All other manipulations require to use a visitor pattern
  public:
    template<bool deep> struct t_hash;
    template<bool deep> struct t_eq;

    using type = tt_term_full<t_make_ref, t_sort_resolver, tt_substitution, tt_sterms ...>;
    using make_ptr_struct = t_make_ref<type>;
    using reference = typename make_ptr_struct::ptr_type;
    using pointer_hash = typename make_ptr_struct::ptr_hash;

    template<bool deep> using t_hash_ref = __detail__::t_term_hash_ref<type, deep>;
    template<bool deep> using t_eq_ref = __detail__::t_term_eq_ref<type, deep>;

    static inline constexpr bool is_const_v = hrw::utils::is_make_ref_const<make_ptr_struct>::value;
    struct t_annex_data_const {};
    struct t_annex_data_no_const { t_annex_data_no_const(): status(e_rw_status::NONE) {} e_rw_status status; };
    struct t_annex_data: public std::conditional_t<is_const_v, t_annex_data_const, t_annex_data_no_const> {};
    t_annex_data& get_annex_data() { return this->m_annex_data; }
    const t_annex_data& get_annex_data() const { return this->m_annex_data; }

    template<template<typename> typename C> using tt_term = C<type>;
    using t_tuple_sterm = hrw::utils::tuple_toset_t<std::tuple<tt_term<tt_sterms>...>>;
    template<std::size_t idx> using sterm_element_t = std::tuple_element_t<idx, t_tuple_sterm>;
    template<typename t_term> static constexpr std::size_t sterm_index_v = hrw::utils::tuple_index_v<t_term, t_tuple_sterm>;

    // substitution construction
    struct t_substitution_iterator {
      template<typename T> using get_iterator = std::conditional_t<is_const_v, typename T::t_container::const_iterator, typename T::t_container::iterator>;
      using t_tuple_sterm_with_iterator = hrw::utils::tuple_elfilter_t<has_container, t_tuple_sterm>;

      using single_iterator = hrw::utils::iterator_single<reference, true>;
      using type = hrw::utils::tuple_toset_t<hrw::utils::tuple_add_t<
        single_iterator, // we need a single iterator on reference, which is const when is_const_v
        hrw::utils::tuple_map_t<get_iterator, t_tuple_sterm_with_iterator>
      >>;
    };
    using t_substitution_iterator_t = typename t_substitution_iterator::type;
    // using t_substitution_parameters_t = hrw::utils::tuple_concat_t<std::tuple<reference>, t_substitution_iterator_t>;
    using t_substitution_parameters_t = hrw::utils::tuple_concat_t<std::tuple<reference>, t_substitution_iterator_t>;
    using single_iterator = typename t_substitution_iterator::single_iterator;

    using t_substitution =  hrw::utils::tuple_convert_t<tt_substitution, t_substitution_parameters_t>;
    // struct t_substitution: public hrw::utils::tuple_convert_t<tt_substitution, t_substitution_parameters_t> {};


    using t_variable = typename t_substitution::t_variable;
    using t_tuple_all_term = hrw::utils::tuple_add_t<t_variable, t_tuple_sterm>;
    using t_content = hrw::utils::tuple_convert_t<std::variant, t_tuple_all_term>;
    t_content m_content; // PUBLIC FOR NOW


    using t_match = tt_term_full_match<type>;
    using t_instantiate = tt_term_full_instantiate<type>;

    ///////////////////////
    // constructors

    tt_term_full(const type& t): m_content(t.m_content), m_uid(0) {
      //std::cout << "constructor tt_term_full::copy : " << ((void*)&t) << " => " << ((void*)this) << std::endl;
      // nb_term += 1;
    }

    tt_term_full(type&& t): m_content(std::move(t.m_content)), m_uid(0) {
      //std::cout << "constructor tt_term_full::move : " << ((void*)&t) << " => " << ((void*)this) << std::endl;
      // nb_term += 1;
    }

    template<typename T, std::enable_if_t<hrw::utils::tuple_contains_v<T, t_tuple_all_term>, bool> = true> tt_term_full(T&& t): m_content(std::move(t)), m_uid(-1) {
      //std::cout << "constructor tt_term_full<" << hrw::utils::type_name<T>() << "&&>" << " => " << ((void*)this) << std::endl;
      // nb_term += 1;
    }
    template<typename T, std::enable_if_t<hrw::utils::tuple_contains_v<T, t_tuple_all_term>, bool> = true> tt_term_full(const T& t): m_content(t), m_uid(-1) {
      //std::cout << "constructor tt_term_full<const " << hrw::utils::type_name<T>() << "&>" << " => " << ((void*)this) << std::endl;
      // nb_term += 1;
    }


    ~tt_term_full() {
      // nb_term -= 1;
      // std::cout << "  HTERM: delete " << reinterpret_cast<void*>(this) << ": " << nb_term << std::endl;
    }

    ///////////////////////
    // base API

    bool is_structured() const { return (std::get_if<t_variable>(&(this->m_content)) == nullptr); }

    bool is_ground() const {
      is_ground_helper obj;
      return VISIT_SINGLE(obj, this->m_content);
    }

    t_sort_id get_sort() const {
      get_sort_helper obj;
      return VISIT_SINGLE(obj, this->m_content);
    }

    const std::string& get_spec() const {
      if(this->m_content.valueless_by_exception()) {
        std::ostringstream oss;
        oss << ((void*)this);
        throw hrw::exception::generic("ERROR!!!! :( " + oss.str());
      }
      get_spec_helper obj;
      return VISIT_SINGLE(obj, this->m_content);
    }

    t_constructor_id get_constructor() const {
      get_constructor_helper obj;
      return VISIT_SINGLE(obj, this->m_content);
    }

    t_constructor_key get_constructor_key() const {
      t_constructor_id c = this->get_constructor();
      return std::make_pair(this->m_content.index() - 1, c);
    }


    // generic visitor
    template<typename T>
    T const * get_if() const {
      using t_core = std::remove_cv_t<T>;
      return std::get_if<t_core>(&(this->m_content));
    }

    template<typename T>
    T * get_if() {
      using t_core = std::remove_cv_t<T>;
      return std::get_if<t_core>(&(this->m_content));
    }


    static constexpr std::size_t nb_alternative = std::variant_size_v<t_content>;
    std::size_t index() const { return this->m_content.index(); }


    template<typename t_visitor, typename t_result=hrw::utils::visit_get_result_t<t_visitor, t_content>>
    t_result visit(t_visitor&& visitor) {
      if constexpr(std::is_same_v<t_result, void>) {
        VISIT_SINGLE(visitor, this->m_content);
      } else {
        return VISIT_SINGLE(visitor, this->m_content); 
      }
    }

    template<typename t_visitor, typename t_result=hrw::utils::visit_get_result_t<t_visitor, t_content>>
    t_result visit(t_visitor&& visitor) const {
      if constexpr(std::is_same_v<t_result, void>) {
        VISIT_SINGLE(visitor, this->m_content);
      } else {
        return VISIT_SINGLE(visitor, this->m_content); 
      }
    }


    // printing
    template<typename t_context_print>
    void print(std::ostream& os, t_context_print& c) const {
      VISIT_SINGLE([&os,&c](auto& t) {t.print(os, c);}, this->m_content);
    }


    /////////////////////////////////////////
    // rewrite API

    ///////////////////////
    // match

    std::optional<t_substitution> match(reference&& t) = delete;
    std::optional<t_substitution> match(reference&& t) const = delete;


    std::optional<t_substitution> match(reference& t) {
      std::optional<t_substitution> res(std::in_place);
      if(not t_match().match_term(this, t, res.value())) {
        res.reset();
      }
      return res;
    }
    std::optional<t_substitution> match(reference& t) const {
      std::optional<t_substitution> res(std::in_place);
      if(not t_match().match_term(this, t, res.value())) {
        res.reset();
      }
      return res;
    }


    /////////////////////////////////////////
    // test API

    template<bool deep> struct t_hash {
      using value_type = type;
      template<typename ti_term> using t_hash_inner = typename ti_term::template t_hash<deep>;
      // using hash_content = hrw::utils::hash_combine<std::variant<t_hash_inner<const t_variable>, t_hash_inner<tt_term<tt_sterms>> ...>>;
      // using hash_content = hrw::utils::get_hash_t<typename value_type::t_content>;
      using hash_content = hrw::utils::hash_combine< hrw::utils::tuple_convert_t<std::variant, hrw::utils::tuple_map_t<t_hash_inner, t_tuple_all_term>> >;
      static_assert(std::is_same_v<typename value_type::t_content, typename hash_content::value_type>);
      hrw::utils::hash_value operator()(const value_type& t) const {
        // std::cout << "  t_hash<" << std::boolalpha << deep << ">" << std::endl;
        if constexpr(deep) {
          if(t.m_uid == 0) {
            t.m_uid = hash_content()(t.m_content);
          }
          return t.m_uid;
        } else {
          return hash_content()(t.m_content);
        }
      }
    };


    template<bool deep> struct t_eq {
      using value_type = type;
      template<typename ti_term> using t_eq_inner = typename ti_term::template t_eq<deep>;
      using eq_content = hrw::utils::eq_combine< hrw::utils::tuple_convert_t<std::variant, hrw::utils::tuple_map_t<t_eq_inner, t_tuple_all_term>> >;
      // MAYBE TODO: add a map to store the pairs we already tested, for the deep version
      constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
        // std::cout << "  t_eq<" << std::boolalpha << deep << ">" << std::endl;
        return eq_content()(lhs.m_content, rhs.m_content);
      }
    };




    /////////////////////////////////////////
    // mutable API

    type& operator=(type const & t) {
      this->m_content = t.m_content;
      this->m_uid = t.m_uid;
      if constexpr(!is_const_v) {
        this->m_annex_data.status = t.m_annex_data.status;
      }
      return *this;
    }

    friend void swap(type& t1, type& t2) {
      using std::swap;
      // std::cout << "  HTERM: calling swap on " << reinterpret_cast<std::size_t>(&t1) << " vs " << reinterpret_cast<std::size_t>(&t2) << std::endl;
      if constexpr(is_const_v) {
        std::cout << "ERROR: swap of constant term is not allowed" << std::endl;
        throw hrw::exception::generic("ERROR: swap of constant term is not allowed");
      } else {
        t1.m_content.swap(t2.m_content);
        std::swap(t1.m_uid, t2.m_uid);
        std::swap(t1.m_annex_data.status, t2.m_annex_data.status);
      }
    }

    // template<typename t_term>
    // t_term& get() { return std::get<t_term>(this->m_content);}



  private:
    mutable std::size_t m_uid;
    t_annex_data m_annex_data;

    ///////////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION
    ///////////////////////////////////////////////////////////////////////////////

    // class for is_ground
    struct is_ground_helper {
      bool operator()(const t_variable&) { return false; }
      template<typename T>
      bool operator()(const T& t) { return t.is_ground(); }
    };

    // class for get_sort
    struct get_sort_helper {
      t_sort_id operator()(const t_variable& v) {
        try {
          return v.get_spec().get_letter();
        } catch(const hrw::exception::generic& e) {
          std::string msg = "ERROR: variable do not have a sort (found \"";
          msg.append(e.what()).append("\")");
          throw hrw::exception::generic(msg);
        }
      }
      template<typename T>
      t_sort_id operator()(const T& t) { return t.get_sort(); }
    };

    // class for get_spec
    struct get_spec_helper {
      const std::string& operator()(const t_variable& v) { return v.get_spec().get_regexp(); }
      template<typename T>
      const std::string& operator()(const T& t) { return t_sort_resolver::get_sort_name(t.get_sort()); }
    };

    // class for get_constructor
    struct get_constructor_helper {
      t_constructor_id operator()(const t_variable&) { throw hrw::exception::generic("ERROR: variables do not have a constructor"); }
      template<typename T>
      t_constructor_id operator()(const T& t) { return t.get_constructor(); }
    };


  };

}


#endif // __HREWRITE_HTERM_H__

