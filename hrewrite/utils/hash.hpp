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

#ifndef __HREWRITE_UTILS_HASH_H__
#define __HREWRITE_UTILS_HASH_H__

#include <vector>
#include <tuple>
#include <variant>
#include <unordered_set>
#include <unordered_map>


#include "hrewrite/utils/type_traits.hpp"
#include "hrewrite/utils/variant.hpp"

namespace hrw {
  namespace utils {

    //////////////////////////////////////////
    // hash value with combine operator

    class hash_value {
    public:
      hash_value(std::size_t s): m_content(s) {}
      hash_value(const hash_value& s) = default;
      hash_value(hash_value&& s)      = default;
      operator size_t() const { return this->m_content; }

      hash_value& operator<<(std::size_t s) {
        this->m_content ^= s + 0x9e3779b9 + (this->m_content<<6) + (this->m_content>>2);
        return *this;
      }
      hash_value& operator<<(const hash_value& v) { return (*this) << v.m_content; }
    private:
      std::size_t m_content;
    };



    //////////////////////////////////////////
    // wrapper on hash including hashed type

    template<typename T, typename H=std::hash<T>>
    struct hash {
      using value_type = T;
      hash_value operator()(const value_type& v) const { return hash_value(H()(v)); }
    };


    //////////////////////////////////////////
    // combiner of hash on std structures

    template<typename T, typename ... Args> struct hash_combine;


    template<typename T> using get_value_type = typename T::value_type;
    template<typename T> using get_value_type_keep_const = typename std::conditional<std::is_const<T>::value, typename std::add_const<get_value_type<T>>::type, get_value_type<T>>::type;

    // hash for pair
    template<typename L, typename R>
    struct hash_combine<std::pair<L, R>> {
      using value_type = std::pair<get_value_type_keep_const<L>, get_value_type_keep_const<R>>;
      hash_value operator()(const value_type& v) const {
        hash_value res = L()(v.first);
        res << R()(v.second);
        return res;
      }
    };

    // hash for vector
    template<typename T>
    struct hash_combine<std::vector<T>> {
      using value_type = std::vector<get_value_type<T>>;
      hash_value operator()(const value_type& v) const {
        hash_value res{0};
        for(get_value_type<T> el: v) {
          res << T()(el);
        }
        return res;
      }
    };

    // hash for tuple
    template<typename ... Hs>
    struct hash_combine<std::tuple<Hs ...>> {
      using value_type = std::tuple<get_value_type_keep_const<Hs> ...>;
      hash_value operator()(const value_type& v) const {
        return std::apply([](get_value_type<Hs> ... args) {
          return (hash_value(0) << ... << (typename std::remove_const<Hs>::type()(args)));
        }, v);
      }
    };

    // hash for variant
    template<typename ... Hs>
    struct hash_combine<std::variant<Hs ...>> {
      static_assert(typelist_isset<get_value_type<Hs> ...>::value);
      using value_type = std::variant<get_value_type_keep_const<Hs> ...>;
      using t_hash_tuple = std::tuple<Hs...>;
      using type = hash_combine<std::variant<Hs ...>>;
      hash_value operator()(const value_type& v) const {
        return type::_inner(v);
      }
    private:
      template<std::size_t I=0>
      static hash_value _inner(const value_type & v) {
        using t_hash = std::tuple_element_t<I, t_hash_tuple>;
        if(v.index() == I) {
          return hash_value(t_hash()(std::get<I>(v)));
        }
        if constexpr(I+1 < std::variant_size_v<value_type>) {
          return type::_inner<I+1>(v);
        } else {
          throw std::bad_variant_access{};
        }
      }
    };


    // hash for unordered_set
    template<typename Key, typename key_hash, typename key_equal, typename Alloc>
    struct hash_combine<std::unordered_set<Key, key_hash, key_equal, Alloc>> {
      using value_type = std::unordered_set<Key, key_hash, key_equal, Alloc>;
      hash_value operator()(const value_type& v) const {
        hash_value res(0);
        for(auto & el: v) {
          res << key_hash()(el);
        }
        return res;
      }
    };


    // hash for unordered_map
    template<typename Key, typename T, typename key_hash, typename key_equal, typename Alloc, typename val_hash>
    struct hash_combine<std::unordered_map<Key, T, key_hash, key_equal, Alloc>, val_hash> {
      using value_type = std::unordered_map<Key, T, key_hash, key_equal, Alloc>;
      static_assert(std::is_same_v<T, typename val_hash::value_type>);
      hash_value operator()(const value_type& v) const {
        hash_value res(0);
        for(auto el: v) {
          res << key_hash()(el.first) << val_hash()(el.second);
        }
        return res;
      }
    };


    //////////////////////////////////////////
    // combiner of hash on std structures

    template<typename T, typename ... Args> struct eq_combine;

    // eq for pair
    template<typename L, typename R>
    struct eq_combine<std::pair<L, R>> {
      using value_type = std::pair<get_value_type_keep_const<L>, get_value_type_keep_const<R>>;
      bool operator()(const value_type& lhs, const value_type& rhs) const {
        return L()(lhs.first, rhs.first) && R()(lhs.second, rhs.second);
      }
    };

    // eq for vector
    template<typename T>
    struct eq_combine<std::vector<T>> {
      using value_type = std::vector<get_value_type<T>>;
      bool operator()(const value_type& lhs, const value_type& rhs) const {
        if(lhs.size() == rhs.size()) {
          T comp;
          for(std::size_t i = 0; i < lhs.size(); ++i) {
            if(not comp(lhs[i], rhs[i])) {
              return false;
            }
          }
          return true;
        } else {
          return false;
        }
      }
    };

    // eq for tuple
    template<typename ... Hs>
    struct eq_combine<std::tuple<Hs ...>> {
      using value_type = std::tuple<get_value_type_keep_const<Hs> ...>;
      using eq_type = std::tuple<Hs ...>;
      using type = eq_combine<std::tuple<Hs ...>>;
      template<std::size_t... I>
      bool operator()(const value_type& lhs, const value_type& rhs) const {
        return type::_inner(lhs, rhs, std::make_index_sequence<std::tuple_size_v<eq_type>>{});
      }
    private:
      template<std::size_t ... I>
      static constexpr bool _inner(const value_type& lhs, const value_type& rhs, std::index_sequence<I...>) {
        return (std::tuple_element_t<I, eq_type>()(std::get<I>(lhs), std::get<I>(rhs)) && ...);
      }
    };

    // eq for variant
    template<typename ... Hs>
    struct eq_combine<std::variant<Hs ...>> {
      static_assert(typelist_isset<get_value_type<Hs> ...>::value);
      using value_type = std::variant<get_value_type_keep_const<Hs> ...>;
      using t_eq_tuple = std::tuple<Hs...>;
      using type = eq_combine<std::variant<Hs ...>>;
      bool operator()(const value_type& lhs, const value_type& rhs) const {
        if((lhs.index() == rhs.index()) && (rhs.index() != std::variant_npos)) {
          return type::template _inner(lhs, rhs);
        } else {
          return false;
        }
      }
    private:
      template<std::size_t I=0>
      static constexpr bool _inner(const value_type& lhs, const value_type& rhs) {
        using t_eq = std::tuple_element_t<I, t_eq_tuple>;
        if(lhs.index() == I) {
          return t_eq()(std::get<I>(lhs), std::get<I>(rhs));
        }
        if constexpr(I+1 < std::variant_size_v<value_type>) {
          return type::_inner<I+1>(lhs, rhs);
        } else {
          throw std::bad_variant_access{};
        }
      }
    };

    // eq for unordered_map
    template<typename Key, typename T, typename key_hash, typename key_equal, typename Alloc, typename val_equal>
    struct eq_combine<std::unordered_map<Key, T, key_hash, key_equal, Alloc>, val_equal> {
      using value_type = std::unordered_map<Key, T, key_hash, key_equal, Alloc>;
      static_assert(std::is_same_v<T, typename val_equal::value_type>);
      bool operator()(const value_type& lhs, const value_type& rhs) const {
        if(lhs.size() != rhs.size()) { return false; }
        for(auto el: lhs) {
          auto it = rhs.find(el.first);
          if(it == rhs.end()) { return false; }
          else if(not val_equal()(el.second, it->second)) { return false; }
        }
        return true;
      }
    };


    //////////////////////////////////////////
    // helper to get hash class
    struct has_t_hash {
      template<typename T> using _get = typename T::t_hash;
      template<typename T> static constexpr bool value = inner_type<_get>::template has_v<T>;
    };
    template<typename T> constexpr bool has_t_hash_v = has_t_hash::value<T>;


    template<typename T, bool=std::is_constructible_v<std::hash<T>>, bool=has_t_hash_v<T>> struct get_hash;

    template<typename T> struct get_hash<T, true, false> { using type = hash<T>; };
    template<typename T> struct get_hash<T, false, true> { using type = typename T::t_hash; };
    template<typename T> struct get_hash<T, true, true>: public get_hash<T, false, true> {};

    template<template<typename ...> typename TT, typename ... Args> struct get_hash<TT<Args...>, false, false> {
      using type = hash_combine<TT<typename get_hash<Args>::type ...>>;
    };

    template<typename T> using get_hash_t = typename get_hash<T>::type;



    //////////////////////////////////////////
    // helper to get equality class
    struct has_t_eq {
      template<typename T> using _get = typename T::t_eq;
      template<typename T> static constexpr bool value = inner_type<_get>::has_v<T>;
    };
    template<typename T> constexpr bool has_t_eq_v = has_t_eq::value<T>;


    template<typename T, bool=has_operator::equality_v<bool, const T&, const T&>, bool=has_t_eq_v<T>> struct get_eq;

    template<typename T> struct get_eq<T, true, false> { using type = std::equal_to<T>; };
    template<typename T> struct get_eq<T, false, true> { using type = typename T::t_eq; };
    template<typename T> struct get_eq<T, true, true>: public get_eq<T, false, true> {};
    template<typename T> struct get_eq<T, false, false>: public get_eq<T, true, false> {};

    template<typename T> using get_eq_t = typename get_eq<T>::type;

  }
}


#endif // __HREWRITE_UTILS_HASH_H__

