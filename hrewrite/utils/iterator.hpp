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


#ifndef __HREWRITE_UTILS_ITERATOR_H__
#define __HREWRITE_UTILS_ITERATOR_H__

#include <iterator>
#include <optional>

#include "hrewrite/utils/variant.hpp"
#include "hrewrite/utils/type_traits.hpp"
#include "hrewrite/exceptions/utils_core.hpp"

namespace hrw {
  namespace utils {



    /////////////////////////////////////////////////////////////////////////////
    // ITERATOR WRAPPER
    /////////////////////////////////////////////////////////////////////////////


    template<typename Arg, typename ... termIts>
    class tt_iterator {
    public:
      using iterator_category = std::input_iterator_tag;
      using value_type = Arg;
      using difference_type = std::ptrdiff_t;
      using pointer = Arg*;
      using reference = Arg&;
      using type = tt_iterator<Arg, termIts...>;

      static_assert(std::conjunction_v<std::is_same<value_type, typename termIts::value_type>...>);

      tt_iterator() = default;
      tt_iterator(type& it) = default;
      tt_iterator(const type& it) = default;
      type& operator=(type& it) = default;
      type& operator=(const type& it) = default;

      template<typename T, typename=std::enable_if_t<tuple_contains_v<T, std::tuple<termIts...>>>> tt_iterator(T&& it): m_content(it) {}
      template<typename T, typename=std::enable_if_t<tuple_contains_v<T, std::tuple<termIts...>>>> tt_iterator(const T& it): m_content(it) {}

      type& operator++() {
        VISIT_SINGLE(increment_helper(), this->m_content);
        return *this;
      }

      Arg operator*() {
        return VISIT_SINGLE(star_helper(), this->m_content);
      }

      bool operator==(const tt_iterator<Arg, termIts...> it) const { return this->m_content == it.m_content; }
      bool operator!=(const tt_iterator<Arg, termIts...> it) const { return this->m_content != it.m_content; }

    private:
      using t_content = typename std::variant<termIts...>;
      t_content m_content;
      
      struct increment_helper {
        void operator()(std::monostate) { throw hrw::exception::iterator_increment(); }
        template<typename T> void operator()(T& it) { ++it; }
      };

      struct star_helper {
        Arg operator()(std::monostate) { throw hrw::exception::iterator_access(); }
        template<typename T> Arg operator()(T& it) { return *it; }
      };
    };



    /////////////////////////////////////////////////////////////////////////////
    // WRAP A SINGLETON IN AN ITERATOR
    /////////////////////////////////////////////////////////////////////////////


    template<typename T>
    class iterator_single_copy {
    public:
      using type = iterator_single_copy<T>;
      using iterator_category = std::input_iterator_tag;
      using value_type = T;
      using difference_type = std::ptrdiff_t;
      using pointer = T*;
      using reference = T&;
      using const_reference = T const &;

      explicit iterator_single_copy(T* ptr) = delete;
      explicit iterator_single_copy(T&& v): m_content(v) {}
      explicit iterator_single_copy(T const & v) : m_content(v) {}
      explicit iterator_single_copy()     : m_content() {}
      type& operator++() { this->m_content.reset(); return *this; }
      type operator+(int i) const {
        if((i == 0) && this->m_content.has_value()) {
          return type(this->m_content.value());
        } else {
          return type();
        }
      }
      bool operator==(type other) const { return this->m_content == other.m_content; }
      bool operator!=(type other) const { return !(*this == other); }
      reference operator*() { return this->m_content.value(); }
      const_reference operator*() const { return this->m_content.value(); }
    private:
      std::optional<T> m_content;
    };

    template<typename T>
    class iterator_single_no_copy {
    public:
      using type = iterator_single_no_copy<T>;
      using iterator_category = std::input_iterator_tag;
      using value_type = T;
      using difference_type = std::ptrdiff_t;
      using pointer = T*;
      using reference = T&;

      iterator_single_no_copy(T&& t) = delete;
      explicit iterator_single_no_copy(T* ptr) : m_content(ptr) {}
      explicit iterator_single_no_copy(T& v)   : m_content(&v) {}
      explicit iterator_single_no_copy()       : m_content(nullptr) {}
      type& operator++() { this->m_content = nullptr; return *this; }
      type operator+(int i) const { return type((i == 0)?(this->m_content):nullptr); }
      bool operator==(type other) const { return this->m_content == other.m_content; }
      bool operator!=(type other) const { return !(*this == other); }
      reference operator*() const { return *(this->m_content); }
    private:
      pointer m_content;
    };

    template<typename T, bool copy=false> using iterator_single = std::conditional_t<copy, iterator_single_copy<T>, iterator_single_no_copy<T>>;

  }
}


#endif // __HREWRITE_UTILS_ITERATOR_H__

