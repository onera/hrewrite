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


#ifndef __HREWRITE_THEORY_VARIABLE_H__
#define __HREWRITE_THEORY_VARIABLE_H__


#include <unordered_map>
#include <vector>
#include <optional>
#include <iostream>


#include "hrewrite/utils.hpp"
#include "hrewrite/parsing/core.hpp"
#include "hrewrite/theory/core.hpp"


namespace hrw {
namespace theory {

    /////////////////////////////////////////////////////////////////////////////
    // VARIABLES IDENTIFIED BY ADDRESS
    /////////////////////////////////////////////////////////////////////////////

    template<typename targ_spec>
    class theory_variable_term_no_id {
    public:
      using t_spec = targ_spec;
      using type = theory_variable_term_no_id<t_spec>;

      theory_variable_term_no_id(t_spec&& spec): m_spec(spec) { } // std::cout << "variable&& = " << this->m_spec << std::endl; }
      theory_variable_term_no_id(const t_spec& spec): m_spec(spec) { } // std::cout << "variable& = " << this->m_spec << std::endl; }
      theory_variable_term_no_id(type const & t): m_spec(t.m_spec){ } // std::cout << "variable& = " << this->m_spec << std::endl; }

      const t_spec& get_spec() const { return this->m_spec; }

      template<typename t_context_print>
      void print(std::ostream& os, t_context_print& c) const {
        os << c.get_name_variable(reinterpret_cast<std::size_t>(this));
      }


      /////////////////////////////////////////
      // test API

      struct t_hash_core {
        using value_type = type;
        hrw::utils::hash_value operator()(const value_type& t) const {
          return hrw::utils::hash<std::size_t>()(std::size_t(&t));
        }
      };
      struct t_eq_core {
        using value_type = type;
        constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
          return (std::size_t(&(lhs)) == std::size_t(&(rhs)));
        }
      };

      template<bool deep> using t_hash = t_hash_core;
      template<bool deep> using t_eq = t_eq_core;


      /////////////////////////////////////////
      // mutable API

      type& operator=(type const & t) {
        this->m_spec = t.m_spec;
        return *this;
      }

      friend void swap(type& v1, type& v2) { using std::swap; swap(v1.m_spec, v2.m_spec); }

    private:
       const t_spec m_spec;
    };


    /////////////////////////////////////////////////////////////////////////////
    // VARIABLES IDENTIFIED BY ID: not thread safe for now
    /////////////////////////////////////////////////////////////////////////////

    template<typename targ_spec>
    class theory_variable_term_id {
    public:
      using t_spec = targ_spec;
      using type = theory_variable_term_id<t_spec>;
      using t_id = unsigned int;

      theory_variable_term_id(t_spec&& spec): m_spec(spec), m_id(type::counter++) { } // std::cout << "variable&& = " << this->m_spec << std::endl; }
      theory_variable_term_id(const t_spec& spec): m_spec(spec), m_id(type::counter++) { } // std::cout << "variable& = " << this->m_spec << std::endl; }
      theory_variable_term_id(type const & t): m_spec(t.m_spec), m_id(t.m_id) { } // std::cout << "variable& = " << this->m_spec << std::endl; }

      const t_spec& get_spec() const { return this->m_spec; }
      t_id get_id() const { return this->m_id; }

      template<typename t_context_print>
      void print(std::ostream& os, t_context_print& c) const {
        os << c.get_name_variable(static_cast<std::size_t>(this->get_id()));
      }

      /////////////////////////////////////////
      // test API

      struct t_hash_core {
        using value_type = type;
        hrw::utils::hash_value operator()(const value_type& t) const {
          return hrw::utils::hash<t_id>()(t.get_id());
        }
      };
      struct t_eq_core {
        using value_type = type;
        constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const { 
          return (lhs.get_id() == rhs.get_id());
        }
      };

      template<bool deep> using t_hash = t_hash_core;
      template<bool deep> using t_eq = t_eq_core;


      /////////////////////////////////////////
      // mutable API

      type& operator=(type const & t) {
        this->m_spec = t.m_spec;
        this->m_id = t.m_id;
        return *this;
      }

      friend void swap(type& v1, type& v2) {
        using std::swap;
        swap(v1.m_spec, v2.m_spec);
        swap(v1.m_id, v2.m_id);
      }

      static t_id get_counter() { return type::counter; }

    private:
       t_spec m_spec;
       t_id m_id;

       static t_id counter;
    };

    template<typename targ_spec>
    typename theory_variable_term_id<targ_spec>::t_id theory_variable_term_id<targ_spec>::counter = 0;


    /////////////////////////////////////////////////////////////////////////////
    // GENERIC SUBSTITUTION
    /////////////////////////////////////////////////////////////////////////////

    namespace _detail {
      ///////////////////////
      // content cell
      template<typename targ_reference, typename T, typename termIts, typename=void> struct make_content_cell_impl;
      template<typename targ_reference, typename T, typename ... termIts> struct make_content_cell_impl<targ_reference, T, std::tuple<termIts ...>,
        std::enable_if_t<T::t_spec::complexity == hrw::utils::parsing_complexity::ELEMENT, void>> {
        using type = make_content_cell_impl<targ_reference, T, std::tuple<termIts ...>>;
        using reference = targ_reference;
        using t_content = std::optional<reference>;

        make_content_cell_impl(): m_content(std::nullopt) {};
        make_content_cell_impl(reference ref): m_content(ref) {};
        bool is_empty() const { return !this->m_content.has_value(); }
        template<typename t_container> bool retrieve(t_container& container) {
          if(this->m_content.has_value()) {
            container.push_back(this->m_content.value());
            return true;  
          } else {
            return false;
          }
        }
        template<typename t_container> bool retrieve(t_container& container) const {
          if(this->m_content.has_value()) {
            container.push_back(this->m_content.value());
            return true;  
          } else {
            return false;
          }
        }
        template<typename t_context_print>
        void print(std::ostream& os, t_context_print& c) const { this->m_content->print(os, c); }

        template<bool deep> struct t_hash {
          using value_type = type;
          using hash_content = typename value_type::reference::template t_hash<deep>;
          hrw::utils::hash_value operator()(const value_type& t) const {
            if(t.is_empty()) { return 0; }
            else {
              return hash_content()(t.m_content.value());
            }
          }
        };
        template<bool deep> struct t_eq {
          using value_type = type;
          using eq_content = typename value_type::reference::template t_eq<deep>;
          constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
            if(lhs.is_empty()) { return rhs.is_empty(); }
            else if(rhs.is_empty()) { return false; }
            else {
              return eq_content()(lhs.m_content.value(), rhs.m_content.value());
            }
          }
        };

        t_content m_content;
      };



      template<typename termIt> struct tt_inner {
        using t_content = termIt;
        tt_inner(termIt begin, termIt end): begin(begin), end(end) {}
        termIt begin, end;
      };

      template<typename targ_reference, typename T, typename ... termIts> struct make_content_cell_impl<targ_reference, T, std::tuple<termIts ...>,
        std::enable_if_t<T::t_spec::complexity >= hrw::utils::parsing_complexity::SEQUENCE, void>> {
        using type = make_content_cell_impl<targ_reference, T, std::tuple<termIts ...>>;
        using iterators = hrw::utils::tuple_toset_t<std::tuple<termIts ...>>;
        using t_content = hrw::utils::tuple_convert_t<std::variant, hrw::utils::tuple_add_t<std::monostate, hrw::utils::tuple_map_t<tt_inner, iterators>>>;

        make_content_cell_impl(): m_content() {};
        template<typename termIt, std::enable_if_t<hrw::utils::tuple_contains_v<termIt, iterators>, bool> = true>
        make_content_cell_impl(termIt begin, termIt end): m_content(tt_inner<termIt>(begin, end)) {};
        bool is_empty() const { return this->m_content.index() == 0; }
        template<typename t_container> bool retrieve(t_container& container) {
          // std::cout << "make_content_cell_impl: " << this->m_content.index() << std::endl;
          return VISIT_SINGLE(hrw::utils::visit_helper {
            [](const std::monostate&) { return false; },
            [&container](auto& inner) {
              // std::cout << "  content_cell::retrieve: is inner.begin == inner.end: " << std::boolalpha << (inner.begin == inner.end) << std::endl;
              container.insert(container.end(), inner.begin, inner.end);
              return true;
          } }, this->m_content);
        }
        template<typename t_container> bool retrieve(t_container& container) const {
          // std::cout << "make_content_cell_impl [const]: " << this->m_content.index() << std::endl;
          return VISIT_SINGLE(hrw::utils::visit_helper {
            [](const std::monostate&) { return false; },
            [&container](auto& inner) {
              // std::cout << "  content_cell::retrieve: is inner.begin == inner.end: " << std::boolalpha << (inner.begin == inner.end) << std::endl;
              container.insert(container.end(), inner.begin, inner.end);
              return true;
          } }, this->m_content);
        }
        template<typename t_context_print>
        void print(std::ostream& os, t_context_print& c) const {
          VISIT_SINGLE(hrw::utils::visit_helper {
            [](const std::monostate&) {},
            [&os, &c](auto& vcell) {
              os << "[ ";
              auto it = vcell.begin;
              while(it != vcell.end) {
                (*it)->print(os, c);
                ++it;
                if(it != vcell.end) {
                  os << ", ";
                }
              }
              os << " ]";
            } }, this->m_content);
        }

        template<bool deep> struct t_hash {
          using value_type = type;
          hrw::utils::hash_value operator()(const value_type& t) const {
            return VISIT_SINGLE(hrw::utils::visit_helper {
              [](const std::monostate&) { return utils::hash_value(0); },
              [](const auto& vcell) {
                using t_ptr = typename std::decay_t<decltype(vcell)>::t_content::value_type;
                using t_real = std::decay_t<decltype(*std::declval<t_ptr>())>;
                using t_hash_inner = typename t_real::template t_hash<deep>;
                utils::hash_value res(0);
                auto it = vcell.begin;
                while(it != vcell.end) {
                  res << t_hash_inner()(*(*it));
                  ++it;
                }
                return res;
              } }, t.m_content);
          }
        };

        template<bool deep> struct t_eq {
          using value_type = type;
          constexpr bool operator()( const value_type& lhs, const value_type& rhs ) const {
            return VISIT_SINGLE(hrw::utils::visit_helper {
              [](const std::monostate&, const std::monostate&) { return true; },
              [](const std::monostate&, const auto&) { return false; },
              [](const auto&, const std::monostate&) { return false; },
              [](const auto& lhs_cell, const auto& rhs_cell) {
                using t_ptr_left = typename std::decay_t<decltype(lhs_cell)>::t_content::value_type;
                using t_real_left = std::decay_t<decltype(*std::declval<t_ptr_left>())>;
                // using t_ptr_right = typename std::decay_t<decltype(rhs_cell)>::t_content::value_type;
                // using t_real_right = std::decay_t<decltype(*std::declval<t_ptr_right>())>;
                using t_eq_inner = typename t_real_left::template t_eq<deep>;

                auto it_left = lhs_cell.begin;
                auto it_right = rhs_cell.begin;
                while((it_left != lhs_cell.end) and (it_right != rhs_cell.end)) {
                  if(not t_eq_inner()(*(*it_left), *(*it_right))) { return false; }
                  ++it_left;
                  ++it_right;
                }
                return (it_left == lhs_cell.end) and (it_right == rhs_cell.end);
              } }, lhs.m_content, rhs.m_content);
          }
        };

        t_content m_content;
      };


      template<typename reference, typename T, typename ... termIts> using make_content_cell = make_content_cell_impl<reference, T, std::tuple<termIts...>>;

      ///////////////////////
      // content

      template<typename t_content_cell, typename T, typename=void> struct make_content;
      template<typename t_content_cell, typename T> struct make_content<t_content_cell, T, std::enable_if_t<has_t_id_v<T>, void>> {
        using type = make_content<t_content_cell, T, std::enable_if_t<has_t_id_v<T>, void>>;
        using t_variable = T;
        using t_content = std::vector<t_content_cell>;
        using iterator = typename t_content::iterator;
        using const_iterator = typename t_content::const_iterator;

        template<typename ... Args>
        void insert(const t_variable* v, Args&& ... args) {
          auto id = v->get_id();
          if(this->m_content.size() <= id) { this->m_content.resize(id+1); }
          this->m_content[v->get_id()] = std::move(t_content_cell(args...));
        }
        bool contains(const t_variable*v) const {
          auto id = v->get_id();
          return ((this->m_content.size() > id) && (!this->m_content[id].is_empty()));          
        }

        iterator get(const t_variable*v) {
          if(this->m_content.size() <= v->get_id()) {
            return this->m_content.end();
          } else {
            return this->m_content.begin() + v->get_id();  
          }
        }
        const_iterator get(const t_variable*v) const {
          if(this->m_content.size() <= v->get_id()) {
            return this->m_content.end();
          } else {
            return this->m_content.begin() + v->get_id();  
          }
        }

        iterator begin() { return this->m_content.begin(); }
        iterator end() { return this->m_content.end(); }
        const_iterator begin() const { return this->m_content.begin(); }
        const_iterator end() const { return this->m_content.end(); }
        void clear() { this->m_content.clear(); }

        static t_content_cell& get_value(const iterator it) { return *it; }
        static const t_content_cell& get_value(const const_iterator it) { return *it; }

        template<typename t_context_print>
        void print(std::ostream& os, t_context_print& c) const {
          bool empty = true;
          os << "{ ";
          for(unsigned int i = 0; i < this->m_content.size(); ++i) {
            if(!this->m_content[i].is_empty()) {
              if(not empty) { os << ", "; }
              os << c.get_name_variable(i) << ": "; this->m_content[i].print(os, c);
              empty = false;
            }
          }
          if(empty) { os << "}"; }
          else  { os << " }"; }
        }

        template<bool deep> struct t_hash {
          using value_type = type;
          using hash_content = utils::hash_combine<std::vector<typename t_content_cell::template t_hash<deep>>>;
          hrw::utils::hash_value operator()(const value_type& t) const {
            return hash_content()(t.m_content);
          }
        };

        template<bool deep> struct t_eq {
          using value_type = type;
          using eq_content = utils::eq_combine<std::vector<typename t_content_cell::template t_eq<deep>>>;
          constexpr bool operator()(const value_type& lhs, const value_type& rhs) const {
            return eq_content()(lhs.m_content, rhs.m_content);
          }
        };

        t_content m_content;
      };

      template<typename t_content_cell, typename T> struct make_content<t_content_cell, T, std::enable_if_t<not has_t_id_v<T>, void>> {
        using type = make_content<t_content_cell, T, std::enable_if_t<not has_t_id_v<T>, void>>;
        using t_variable = T;
        using t_content = std::unordered_map<const t_variable*, t_content_cell>;
        using iterator = typename t_content::iterator;
        using const_iterator = typename t_content::const_iterator;
        template<typename ... Args>
        void insert(const t_variable* v, Args&& ... args) {
          if(this->contains(v)) { this->m_content.erase(v); }
          this->m_content.emplace(v, t_content_cell(args...));
        }
        bool contains(const t_variable* v) const {
          return (this->m_content.find(v) != this->m_content.end());
        }

        iterator get(const t_variable*v) { return this->m_content.find(v); }
        const_iterator get(const t_variable*v) const { return this->m_content.find(v); }

        iterator begin() { return this->m_content.begin(); }
        iterator end() { return this->m_content.end(); }
        const_iterator begin() const { return this->m_content.begin(); }
        const_iterator end() const { return this->m_content.end(); }
        void clear() { this->m_content.clear(); }

        static t_content_cell& get_value(const iterator it) { return it->second; }
        static const t_content_cell& get_value(const const_iterator it) { return it->second; }

        template<typename t_context_print>
        void print(std::ostream& os, t_context_print& c) const {
          bool empty = true;
          os << "{ ";
          for(auto& cell: this->m_content) {
            if(not empty) { os << ", "; }
            cell.first->print(os, c); os << ": "; cell.second.print(os, c);
            empty = false;
          }
          if(empty) { os << "}"; }
          else  { os << " }"; }
        }

        template<bool deep> struct t_hash {
          using value_type = type;
          hrw::utils::hash_value operator()(const value_type& t) const {
            return VISIT_SINGLE(hrw::utils::visit_helper {
              [](const std::monostate&) { return utils::hash_value(0); },
              [](const auto& vcell) {
                using t_ptr = typename std::decay_t<decltype(vcell)>::t_content::value_type;
                using t_real = std::decay_t<decltype(*std::declval<t_ptr>())>;
                using t_hash_inner = typename t_real::template t_hash<deep>;
                utils::hash_value res(0);
                auto it = vcell.begin;
                while(it != vcell.end) {
                  res << t_hash_inner()(*(*it));
                  ++it;
                }
                return res;
              } }, t.m_content);
          }
        };

        template<bool deep> struct t_eq {
          using value_type = type;
          constexpr bool operator()(const value_type& lhs, const value_type& rhs) const {
            return VISIT_SINGLE(hrw::utils::visit_helper {
              [](const std::monostate&, const std::monostate&) { return true; },
              [](const std::monostate&, const auto&) { return false; },
              [](const auto&, const std::monostate&) { return false; },
              [](const auto& lhs_cell, const auto& rhs_cell) {
                using t_ptr_left = typename std::decay_t<decltype(lhs_cell)>::t_content::value_type;
                using t_real_left = std::decay_t<decltype(*std::declval<t_ptr_left>())>;
                // using t_ptr_right = typename std::decay_t<decltype(rhs_cell)>::t_content::value_type;
                // using t_real_right = std::decay_t<decltype(*std::declval<t_ptr_right>())>;
                using t_eq_inner = typename t_real_left::template t_eq<deep>;

                auto it_left = lhs_cell.begin;
                auto it_right = rhs_cell.begin;
                while((it_left != lhs_cell.end) and (it_right != rhs_cell.end)) {
                  if(not t_eq_inner()(*(*it_left), *(*it_right))) { return false; }
                }
                return (it_left == lhs_cell.end) and (it_right == rhs_cell.end);
              } }, lhs.m_content, rhs.m_content);
          }
        };

        t_content m_content;
      };
    }


    template<typename targ_variable, typename targ_reference, typename ... termIts>
    class make_substitution {
    public:
      using type = make_substitution<targ_variable, targ_reference, termIts...>;
      using t_iterators = std::tuple<termIts ...>;
      using t_variable = targ_variable;
      using reference = targ_reference;
      using t_content_cell = _detail::make_content_cell<reference, t_variable, termIts ...>;
      using t_content = _detail::make_content<t_content_cell, t_variable>;
      using iterator = typename t_content::iterator;
      using const_iterator = typename t_content::const_iterator;


      ///////////////////////
      // base API

      template<typename ... Args>
      void insert(const t_variable* v, Args&& ... args) { this->m_content.insert(v, args...); }

      bool contains(const t_variable* v) const { return this->m_content.contains(v); }

      typename t_content_cell::t_content &get(const t_variable* v) { return t_content::get_value(this->m_content.get(v)).m_content; }

      template<typename t_container>
      bool retrieve(const t_variable* v, t_container& container) const {
        auto it = this->m_content.get(v);
        if(it != this->m_content.m_content.end()) {
          return t_content::get_value(it).retrieve(container);
        }
        return false;
      }

      iterator begin() { return this->m_content.begin(); }
      iterator end() { return this->m_content.end(); }
      const_iterator begin() const { return this->m_content.begin(); }
      const_iterator end() const { return this->m_content.end(); }
      void clear() { this->m_content.clear(); }


      template<bool deep> struct t_hash {
        using value_type = type;
        using hash_content = typename value_type::t_content::template t_hash<deep>;
        hrw::utils::hash_value operator()(const value_type& t) const {
          return hash_content()(t.m_content);
        }
      };
      template<bool deep> struct t_eq {
        using value_type = type;
        using eq_content = typename value_type::t_content::template t_eq<deep>;
        constexpr bool operator()(const value_type& lhs, const value_type& rhs) const {
          return eq_content()(lhs.m_content, rhs.m_content);
        }
      };

      ///////////////////////
      // print

      template<typename t_context_print>
      void print(std::ostream& os, t_context_print& c) const { this->m_content.print(os, c); }

    private:
      t_content m_content;
    };




    /////////////////////////////////////////////////////////////////////////////
    // THEORIES
    /////////////////////////////////////////////////////////////////////////////



    template<typename targ_spec, template<typename> typename targ_variable>
    struct tt_theory_variable {
      using t_spec = targ_spec;
      using t_term = targ_variable<t_spec>;

      template<typename t_term_full>
      class factory {
      public:
        factory() = default;

        t_term_full create_term(const std::string& spec) {
          return t_term_full(t_term(t_spec(spec)));
        }
        t_term_full create_term(std::string&& spec) {
          return t_term_full(t_term(t_spec(std::move(spec))));
        }
      };

      template<typename reference, typename ... termIts> using tt_substitution = make_substitution<t_term, reference, termIts...>;
    };


    template<typename targ_spec> using tt_theory_variable_map = tt_theory_variable<targ_spec, theory_variable_term_no_id>;
    template<typename targ_spec> using tt_theory_variable_vector = tt_theory_variable<targ_spec, theory_variable_term_id>;



    template<template<typename tt, const tt&> typename P>
    using tp_theory_variable_map = tth_wrap_to_parser<tt_theory_variable_map, P>;

    template<template<typename tt, const tt&> typename P>
    using tp_theory_variable_vector = tth_wrap_to_parser<tt_theory_variable_vector, P>;

  }
}


#endif // __HREWRITE_THEORY_VARIABLE_H__

