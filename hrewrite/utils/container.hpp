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


#ifndef __HREWRITE_UTILS_CONTAINER_H__
#define __HREWRITE_UTILS_CONTAINER_H__


#include <optional>
#include <memory>
#include <iostream>

#include "hrewrite/utils/iterator.hpp"
#include "hrewrite/utils/hash.hpp"
#include "hrewrite/utils/type_traits.hpp"
#include "hrewrite/exceptions/utils_core.hpp"

namespace hrw {
  namespace utils {


    /////////////////////////////////////////////////////////////////////////////
    // DATA WRAPPER (useful for printing and containers)
    /////////////////////////////////////////////////////////////////////////////

    template<typename T> struct data_wrapper {
      template<typename TT, bool=std::is_rvalue_reference_v<TT>> struct _get_type;
      template<typename TT> struct _get_type<TT, true> { using type = std::remove_reference_t<TT>; static constexpr bool lref = false; };
      template<typename TT> struct _get_type<TT, false> { using type = TT; static constexpr bool lref = std::is_lvalue_reference_v<TT>; };
      using type = typename _get_type<T>::type;
      template<typename TT> static constexpr bool lref = _get_type<T>::lref;
      
      template<typename TT, std::enable_if_t<!lref<TT>, bool> = true>
      data_wrapper(TT&& c): m_content(std::move(c)) {}

      template<typename TT, std::enable_if_t<lref<TT>, bool> = true>
      data_wrapper(TT& c): m_content(c) {}

      type m_content;
    };




    /////////////////////////////////////////////////////////////////////////////
    // SINGLETON CONTAINER
    /////////////////////////////////////////////////////////////////////////////

    template<typename T>
    class container_single {
    public:
      explicit container_single(): m_content() {}
      explicit container_single(const T& t): m_content(t) {}
      explicit container_single(T&& t): m_content(std::move(t)) {}

      bool has() { return this->m_content.has_value(); }
      std::size_t size() { return (this->has()?1:0);}
      T& get() { return this->m_content.value(); }

      using iterator = iterator_single<T>;
      iterator begin() { if(this->has()) { return iterator(this->get()); } else { return iterator(); } }
      iterator end() { return iterator(); }


      void push_back(const T& t) {
        if(this->has()) {
          throw hrw::exception::single_container_full();
        } else {
          this->m_content = t;        
        }
      }

      void push_back(T&& t) {
        if(this->has()) {
          throw hrw::exception::single_container_full();
        } else {
          this->m_content = std::move(t);        
        }
      }
      
      void add(const T& t) { this->push_back(t); }
      void add(T&& t) { this->push_back(std::move(t)); }

      template<typename inputIt>
      iterator insert([[maybe_unused]] iterator bl, inputIt bother, inputIt eother) {
        if(bother != eother) {
          if(!this->has()) {
            this->m_content = *bother;
            if(++bother != eother) { throw hrw::exception::single_container_full(); }
          }
        }
        return this->end();
      }


      void clear() { this->m_content.reset(); }

    private:
      std::optional<T> m_content;      
    };



    /////////////////////////////////////////////////////////////////////////////
    // REGISTERY
    /////////////////////////////////////////////////////////////////////////////

    template<typename T, typename H>
    struct tt_hash {
      using type = tt_hash<T, H>;
      using value_type = T;
      using vvalue_type = typename value_type::value_type;
      using t_ptr = typename value_type::content_ptr;
      tt_hash() {}
      hrw::utils::hash_value operator()(const value_type& t) const { return hrw::utils::get_hash_t<t_ptr>()(t.m_ptr); }
      hrw::utils::hash_value operator()(const vvalue_type& t) const { return hrw::utils::get_hash_t<t_ptr>()(&t); }

      friend void swap(type&, type&) {}
    };


    template<typename T, typename H>
    struct tt_hash_fwd {
      using type = tt_hash_fwd<T, H>;
      using value_type = T;
      using vvalue_type = typename value_type::value_type;
      tt_hash_fwd(): m_hash() {}
      tt_hash_fwd(H h): m_hash(h) {}
      hrw::utils::hash_value operator()(const value_type& t) const { return this->m_hash(*t.m_ptr); }
      hrw::utils::hash_value operator()(const vvalue_type& t) const { return this->m_hash(t); }

      friend void swap(type& h1, type& h2) {
        using std::swap;
        swap(h1.m_hash, h2.m_hash);
      }
      H m_hash;
    };

    template<typename T, typename E>
    struct tt_eq   {
      using type = tt_eq<T, E>;
      using value_type = T;
      using vvalue_type = typename value_type::value_type;
      using t_ptr = typename value_type::content_ptr;
      using is_transparent = void; // to trick the container
      tt_eq() {}
      bool operator()(const value_type& lhs, const value_type& rhs) const { return hrw::utils::get_eq_t<t_ptr>()(lhs.m_ptr, rhs.m_ptr); }
      bool operator()(const value_type& lhs, const vvalue_type& rhs) const { return hrw::utils::get_eq_t<t_ptr>()(lhs.m_ptr, &rhs); }
      bool operator()(const vvalue_type& lhs, const value_type& rhs) const { return hrw::utils::get_eq_t<t_ptr>()(&lhs, rhs.m_ptr); }

      friend void swap(type&, type&) {}
    };

    template<typename T, typename E>
    struct tt_eq_fwd   {
      using type = tt_eq_fwd<T, E>;
      using value_type = T;
      using vvalue_type = typename value_type::value_type;
      using is_transparent = void; // to trick the container
      tt_eq_fwd(): m_eq() {}
      tt_eq_fwd(E e): m_eq(e) {}
      bool operator()(const value_type& lhs, const value_type& rhs) const { return this->m_eq(*lhs.m_ptr, *rhs.m_ptr); }
      bool operator()(const value_type& lhs, const vvalue_type& rhs) const { return this->m_eq(*lhs.m_ptr, rhs); }
      bool operator()(const vvalue_type& lhs, const value_type& rhs) const { return this->m_eq(lhs, *rhs.m_ptr); }

      friend void swap(type& e1, type& e2) {
        using std::swap;
        swap(e1.m_eq, e2.m_eq);          
      }
      E m_eq;
    };


    template<typename T> using tt_hash_get_tmp     = typename T::t_hash;
    template<typename T> using tt_hash_fwd_get_tmp = typename T::t_hash_fwd;
    template<typename T> using tt_eq_get_tmp       = typename T::t_eq;
    template<typename T> using tt_eq_fwd_get_tmp   = typename T::t_eq_fwd;

    template<typename T> using tt_hash_get     = typename inner_type<tt_hash_get_tmp    >::template dflt<int>::template get_t<T>;
    template<typename T> using tt_hash_fwd_get = typename inner_type<tt_hash_fwd_get_tmp>::template dflt<int>::template get_t<T>;
    template<typename T> using tt_eq_get       = typename inner_type<tt_eq_get_tmp      >::template dflt<int>::template get_t<T>;
    template<typename T> using tt_eq_fwd_get   = typename inner_type<tt_eq_fwd_get_tmp  >::template dflt<int>::template get_t<T>;


    template<typename T, typename H, typename E, template<typename> typename A, typename t_reg>
    class t_basic_ptr {
    public:
      static_assert(!std::is_const_v<T>);

      using type = t_basic_ptr<T, H, E, A, t_reg>;
      using value_type = T;
      using value_ptr  = value_type*;
      using value_const = std::add_const_t<value_type>;
      using value_const_ptr = value_const*;

      using content_core = value_type;
      using content_type = value_type;
      using content_ptr = value_type*;
      using allocator_type = A<content_type>;

      t_basic_ptr(): m_ptr(nullptr) {}
      t_basic_ptr(value_ptr ptr): m_ptr(ptr) {}
      t_basic_ptr(value_type&& v) {
        using traits_t = std::allocator_traits<allocator_type>;
        this->m_ptr = traits_t::allocate(type::m_alloc, 1);
        traits_t::construct(type::m_alloc, this->m_ptr, std::move(v));
      }
      t_basic_ptr(type&  c): m_ptr(c.m_ptr) { c.m_ptr = nullptr; }
      t_basic_ptr(type&& c): m_ptr(c.m_ptr) { c.m_ptr = nullptr; }
      ~t_basic_ptr() { if(this->m_ptr != nullptr) { this->destroy_value(); } }

      value_const& operator*() const { return *this->m_ptr; }
      value_const& get()       const { return *this->m_ptr; }
      value_const_ptr operator->() const { return this->m_ptr; }
      value_const_ptr get_ptr()    const { return this->m_ptr; }


      type& operator=(type& t) {
        if(this->m_ptr != nullptr) {
          this->destroy_value();
        }
        this->m_ptr = t->m_ptr;
        t->m_ptr = nullptr;
        return *this;
      }

      friend void swap(type& c1, type& c2) {
        using std::swap;
        swap(c1.m_ptr, c2.m_ptr);
      }

      using t_hash = tt_hash<type, H>;
      using t_hash_fwd = tt_hash_fwd<type, H>;
      using t_eq = tt_eq<type, E>;
      using t_eq_fwd = tt_eq_fwd<type, E>;
      friend t_hash;
      friend t_hash_fwd;
      friend t_eq;
      friend t_eq_fwd;

      friend bool operator==(type const & c1, type const & c2) { return t_eq()(c1, c2); }
      friend bool operator!=(type const & c1, type const & c2) { return !t_eq()(c1, c2); }

    private:
      friend t_reg;
      inline content_ptr get_cptr() { return this->m_ptr; }

      mutable content_ptr m_ptr; // WARNING: workaround some set implementations that return a const iterator with begin() and end()
      static allocator_type m_alloc;

      void destroy_value() {
        using traits_t = std::allocator_traits<allocator_type>;
        traits_t::destroy(type::m_alloc, this->m_ptr);
        traits_t::deallocate(type::m_alloc, this->m_ptr, 1);
      }

    };

    template<typename T, typename H, typename E, template<typename> typename A, typename t_reg>
    typename t_basic_ptr<T, H, E, A, t_reg>::allocator_type t_basic_ptr<T, H, E, A, t_reg>::m_alloc;





    template<typename T, typename H, typename E, typename t_reg>
    struct t_count_wrapper {
      using type = t_count_wrapper<T, H, E, t_reg>;
      using value_type = T;
      using value_const = const T;

      t_count_wrapper(type&& t): m_content(t.m_content), m_reg(t.m_reg), m_count(t.m_count) {}
      t_count_wrapper(t_reg& m, T&& c): m_content(c), m_reg(m), m_count(0) {}
      template<typename ... Args>
      t_count_wrapper(t_reg& m, Args&& ... args): m_content(args...), m_reg(m), m_count(0) {}

      value_const & get() const { return this->m_content; }
      value_const * get_ptr() const { return &(this->m_content); }


      T m_content;
      t_reg& m_reg;
      mutable unsigned int m_count;
    };

    template<typename T, typename H, typename E, typename t_reg>
    struct t_count_wrapper_hash {
      using type = t_count_wrapper_hash<T, H, E, t_reg>;
      using value_type = t_count_wrapper<T, H, E, t_reg>;
      t_count_wrapper_hash(): m_hash() {}
      t_count_wrapper_hash(const H& h): m_hash(h) {}
      t_count_wrapper_hash(const type& h): m_hash(h.m_hash) {}
      t_count_wrapper_hash(type&& h): m_hash(h.m_hash) {}
      // using vvalue_type = typename value_type::value_type;
      hrw::utils::hash_value operator()(const value_type& t) const { return this->m_hash(t.m_content); }
      // hrw::utils::hash_value operator()(const vvalue_type& t) const { return this->m_hash(t); }

      friend void swap(type& h1, type& h2) {
        using std::swap;
        swap(h1.m_hash, h2.m_hash);
      }
    private:
      H m_hash;
    };

    template<typename T, typename H, typename E, typename t_reg>
    struct t_count_wrapper_eq {
      using type = t_count_wrapper_eq<T, H, E, t_reg>;
      using value_type = t_count_wrapper<T, H, E, t_reg>;
      using vvalue_type = typename value_type::value_type;
      using is_transparent = void; // to trick the container
      t_count_wrapper_eq(): m_eq() {}
      t_count_wrapper_eq(const E& eq): m_eq(eq) {}
      t_count_wrapper_eq(const type& e): m_eq(e.m_eq) {}
      t_count_wrapper_eq(type&& e): m_eq(e.m_eq) {}
      bool operator()( const value_type& lhs, const value_type& rhs) const { return this->m_eq(lhs.m_content, rhs.m_content); }
      bool operator()( const value_type& lhs, const vvalue_type& rhs) const { return this->m_eq(lhs.m_content, rhs); }
      bool operator()( const vvalue_type& lhs, const value_type& rhs) const { return this->m_eq(lhs, rhs.m_content); }

      friend void swap(type& e1, type& e2) {
        using std::swap;
        swap(e1.m_eq, e2.m_eq);
      }
    private:
      E m_eq;
    };


    template<typename T, typename H, typename E, template<typename> typename A, typename t_reg, bool c_const>
    class t_count_ptr {
    public:
      using type = t_count_ptr<T, H, E, A, t_reg, c_const>;
      using value_type = T;
      using value_ptr  = value_type*;
      using value_const = std::add_const_t<value_type>;
      using value_const_ptr = value_const*;

      using content_core = t_count_wrapper<T, H, E, t_reg>;
      using content_hash = t_count_wrapper_hash<T, H, E, t_reg>;
      using content_eq   = t_count_wrapper_eq<T, H, E, t_reg>;
      using content_type = std::conditional_t<c_const, const content_core, content_core>;
      using content_ptr = content_type*;
      using allocator_type = A<content_core>;

      using element_type = value_const;

      t_count_ptr(content_ptr c): m_ptr(c) {
        // if(c == nullptr) { throw hrw::exception::generic("cannot be null");}
        ++this->m_ptr->m_count;
        // std::cout << "new ref(1) " << reinterpret_cast<std::size_t>(this->m_ptr) << "[" << ((this->m_ptr != nullptr)?(this->m_ptr->m_count):-1) << "]" << std::endl;
      }
      t_count_ptr(type const & r): m_ptr(r.m_ptr) {
        ++this->m_ptr->m_count;
        // std::cout << "new ref(2) " << reinterpret_cast<std::size_t>(this->m_ptr) << "[" << ((this->m_ptr != nullptr)?(this->m_ptr->m_count):-1) << "]" << std::endl;
      }

      template<bool b=!c_const, std::enable_if_t<b, bool> = true>
      t_count_ptr(t_reg& m, value_type&& v) {
        using traits_t = std::allocator_traits<allocator_type>;
        this->m_ptr = traits_t::allocate(type::m_alloc, 1);
        traits_t::construct(type::m_alloc, this->m_ptr, m, std::move(v));
        this->m_ptr->m_count = 1;
        // std::cout << "new ref(4) " << reinterpret_cast<std::size_t>(this->m_ptr) << "[" << ((this->m_ptr != nullptr)?(this->m_ptr->m_count):-1) << "]" << std::endl;
      }

      ~t_count_ptr() {
        // std::cout << "delete ref " << reinterpret_cast<std::size_t>(this->m_ptr) << "[" << ((this->m_ptr != nullptr)?(this->m_ptr->m_count):-1) << "]" << std::endl;
        this->remove_ref();
      }

      value_const& operator*() const { return this->m_ptr->m_content; }
      value_const& get()       const { return this->m_ptr->m_content; }
      value_const_ptr operator->() const { return &(this->m_ptr->m_content); }
      value_const_ptr get_ptr()    const { return &(this->m_ptr->m_content); }


      t_count_ptr& operator=(t_count_ptr const & r) {
        // std::cout << "operator= " << reinterpret_cast<std::size_t>(this->m_ptr) << "[" << ((this->m_ptr != nullptr)?(this->m_ptr->m_count):-1) << "]" << std::endl;
        if(this->m_ptr != r.m_ptr) {
          this->remove_ref();
          this->m_ptr = r.m_ptr;
          ++this->m_ptr->m_count;
        }
        return *this;
      } 

      friend void swap(type& c1, type& c2) {
        using std::swap;
        swap(c1.m_ptr, c2.m_ptr);
      }


      using t_hash = tt_hash<type, content_hash>;
      using t_hash_fwd = tt_hash_fwd<type, content_hash>;
      using t_eq = tt_eq<type, content_eq>;
      using t_eq_fwd = tt_eq_fwd<type, content_eq>;
      friend t_hash;
      friend t_hash_fwd;
      friend t_eq;
      friend t_eq_fwd;

      friend bool operator==(type const & c1, type const & c2) { return t_eq()(c1, c2); }
      friend bool operator!=(type const & c1, type const & c2) { return !t_eq()(c1, c2); }

    private:
      friend t_reg;
      inline content_type& get_content() { return *(this->m_ptr); }
      inline content_ptr get_cptr() { return this->m_ptr; }

      content_ptr m_ptr;
      static allocator_type m_alloc;

      inline void remove_ref() {
        if constexpr(c_const) {
          if((--this->m_ptr->m_count) == 0) {
            this->m_ptr->m_reg.clear(*this);
          }
        } else {
          if(this->m_ptr != nullptr) {
            --this->m_ptr->m_count;
            if((this->m_ptr->m_count) == 1) {
              this->m_ptr->m_reg.clear(*this);              
            } else if((this->m_ptr->m_count) == 0) {
              using traits_t = std::allocator_traits<allocator_type>;
              traits_t::destroy(type::m_alloc, this->m_ptr);
              traits_t::deallocate(type::m_alloc, this->m_ptr, 1);
            }
          }
        }
      }

    };

    template<typename T, typename H, typename E, template<typename> typename A, typename t_reg, bool c_const>
    typename t_count_ptr<T, H, E, A, t_reg, c_const>::allocator_type t_count_ptr<T, H, E, A, t_reg, c_const>::m_alloc;



    template<typename Tit>
    struct iterator_wrapper {
      using type = iterator_wrapper<Tit>;

      using iterator_category = std::input_iterator_tag;
      using value_type = typename Tit::value_type::value_const;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;
      using const_pointer = value_type const *;
      using const_reference = value_type const &;

      iterator_wrapper(Tit it): m_it(it) {}
      Tit m_it;

      type& operator++() { ++(this->m_it); return *this; }
      value_type& operator*() const { return this->m_it->get(); }
      value_type* operator->() const { return this->m_it->get_ptr(); }
      friend bool operator==(type const & it1, type const & it2) { return (it1.m_it == it2.m_it); }
      friend bool operator!=(type const & it1, type const & it2) { return (it1.m_it != it2.m_it); }
    };

    // TODO: make registry_unique use t_count_ptr
    // PB: t_count_ptr is a template on the type of map: t_make_ref depends on the kind of map!!!


    template<template<typename ... Args> typename tt_set, bool arg_safe_reference, bool count_ptr=false>
    struct registry_unique {

      template<
        typename T,
        typename Hash = std::hash<T>,
        typename KeyEqual = std::equal_to<T>,
        template<typename> typename Allocator = std::allocator,
        bool=arg_safe_reference
      > class make;


      template<
        typename T,
        typename Hash = std::hash<T>,
        typename KeyEqual = std::equal_to<T>,
        template<typename> typename Allocator = std::allocator
      > struct make_ref {
        using type = make_ref<T, Hash, KeyEqual, Allocator>;
        using t_reg = make<T, Hash, KeyEqual, Allocator>;

        using value_type  = T;
        using value_const = const value_type;
        using value_hash  = Hash;
        using value_equal = KeyEqual;
        using value_allocator = Allocator<value_type>;

        using _tmp_basic_ptr = t_basic_ptr<value_type, value_hash, value_equal, Allocator, t_reg>;
        using _tmp_count_ptr = t_count_ptr<value_const, value_hash, value_equal, Allocator, t_reg, arg_safe_reference>;
        using _tmp_count_wrapper = typename _tmp_count_ptr::content_type;
        using _tmp_count_wrapper_core = typename _tmp_count_ptr::content_core;

        using cell_type  = std::conditional_t<count_ptr, _tmp_count_wrapper, value_const>;
        using cell_hash  = std::conditional_t<count_ptr, typename _tmp_count_ptr::content_hash, value_hash>;
        using cell_equal = std::conditional_t<count_ptr, typename _tmp_count_ptr::content_eq, value_equal>;
        using cell_type_core = std::conditional_t<count_ptr, _tmp_count_wrapper_core, value_type>;
        using cell_allocator = Allocator<cell_type_core>;

        using cell_ptr_type  = std::conditional_t<count_ptr, _tmp_count_ptr, std::conditional_t<!arg_safe_reference, _tmp_basic_ptr, value_type*>>;
        using cell_ptr_hash  = std::conditional_t<count_ptr || !arg_safe_reference, tt_hash_fwd_get<cell_ptr_type>, std::hash<value_type *>>;
        using cell_ptr_equal = std::conditional_t<count_ptr || !arg_safe_reference, tt_eq_fwd_get<cell_ptr_type>, std::equal_to<value_type *>>;
        using cell_ptr_allocator = Allocator<cell_ptr_type>;

        using ptr_type = std::conditional_t<count_ptr, _tmp_count_ptr, value_const *>;
        using ptr_const = std::add_const_t<ptr_type>;
        using ptr_hash = std::conditional_t<count_ptr, tt_hash_get<ptr_type>, std::hash<value_const *>>;
        using ptr_equal = std::conditional_t<count_ptr, tt_eq_get<ptr_type>, std::equal_to<value_const *>>;

        static inline constexpr ptr_type from_cell(cell_type& v) {
          if constexpr(count_ptr) {
            return ptr_type(&v);
          } else {
            return &v;
          }
        }

        static inline constexpr ptr_type from_cell_ref(cell_ptr_type const & v) {
          if constexpr(count_ptr) {
            return v;
          } else if constexpr(!arg_safe_reference) {
            return v.get_ptr();
          } else {
            return v;
          }
        }

        static inline constexpr value_const * to_ptr(ptr_const& v) {
          if constexpr(count_ptr) {
            return v.get_ptr();
          } else {
            return v;            
          }
        }


        template<typename Tit>
        using const_iterator = std::conditional_t<
          count_ptr || !arg_safe_reference,
          iterator_wrapper<Tit>,
          Tit
        >;

        template<typename Tit>
        static inline constexpr const_iterator<Tit> make_iterator(Tit it) {
          if constexpr(count_ptr || !arg_safe_reference) {
            return iterator_wrapper(it);
          } else {
            return it;
          }
        }

      };

      static inline constexpr bool ensure_unique_v = true;
      static inline constexpr bool ref_counting = count_ptr;


      template<typename T, typename Hash, typename KeyEqual, template<typename> typename Allocator>
      class make<T, Hash, KeyEqual, Allocator, true> {
      public:
        using type = make<T, Hash, KeyEqual, Allocator>;
        using value_type = T;

        using ref_struct = make_ref<T, Hash, KeyEqual, Allocator>;
        using t_cell = typename ref_struct::cell_type;
        using t_ptr  = typename ref_struct::ptr_type;

        using hasher         = typename ref_struct::cell_hash;
        using key_equal      = typename ref_struct::cell_equal;
        using allocator_type = typename ref_struct::cell_allocator;

        using t_content = tt_set<std::remove_cv_t<t_cell>, hasher, key_equal, allocator_type>;
        using size_type = typename t_content::size_type;

        using const_iterator = typename ref_struct::template const_iterator<typename t_content::const_iterator>;

        static inline constexpr bool safe_reference = arg_safe_reference;
        static inline constexpr bool ref_counting = count_ptr;

        //////////////////////////////////////////
        // constructors
        make(): make(0) {}

        make(size_type count, const hasher& h=hasher(), const key_equal& k=key_equal(), const allocator_type& a=allocator_type()):
          m_content(count, h, k, a) {}

        ~make() {}


        //////////////////////////////////////////
        // api
        t_ptr add(value_type&& t) {
          // std::cout << "registry_unique::add() -> " << std::boolalpha << (this->m_content.find(t) != this->m_content.end()) << std::endl;
          auto it = [&](){
            if constexpr(count_ptr) {
              return this->m_content.emplace(*this, std::move(t));
            } else {
              return this->m_content.emplace(std::move(t));
            }
          }();
          // static_assert(std::is_same_v<decltype(it), t_cell>);
          return ref_struct::from_cell((*it.first));
        }

        const_iterator begin() { return ref_struct::make_iterator(this->m_content.cbegin()); }
        const_iterator end() { return ref_struct::make_iterator(this->m_content.cend()); }

        void clear() noexcept { this->m_content.clear(); }
        template<bool b=count_ptr, std::enable_if_t<b, bool> = true>
        void clear(t_ptr& ref) {
          this->m_content.erase(ref.get_content());
        }
      private:
        t_content m_content;
      };



      template<typename T, typename Hash, typename KeyEqual, template<typename> typename Allocator>
      class make<T, Hash, KeyEqual, Allocator, false> {
      public:
        using type = make<T, Hash, KeyEqual, Allocator>;
        using value_type = T;

        using ref_struct = make_ref<T, Hash, KeyEqual, Allocator>;
        using t_cell_ptr = typename ref_struct::cell_ptr_type;
        using t_ptr  = typename ref_struct::ptr_type;

        using hasher         = typename ref_struct::cell_ptr_hash;
        using key_equal      = typename ref_struct::cell_ptr_equal;
        // using allocator_type = typename ref_struct::cell_allocator;
        using allocator_type = typename ref_struct::cell_ptr_allocator;


        using t_content = tt_set<t_cell_ptr, hasher, key_equal, allocator_type>;
        using size_type = typename t_content::size_type;

        using const_iterator = typename ref_struct::template const_iterator<typename t_content::const_iterator>;

        static inline constexpr bool safe_reference = arg_safe_reference;
        static inline constexpr bool ref_counting = count_ptr;

        //////////////////////////////////////////
        // constructors / destructors
        make(): make(0) {}

        make(size_type count, const hasher& h=hasher(), const key_equal& k=key_equal(), const allocator_type& a=allocator_type()):
          m_content(count, h, k, a) {}

        ~make() {}

        t_ptr add(value_type&& t) {
          // create dummy cell_ptr
          using t_cell = typename t_cell_ptr::content_core;
          t_cell dummy_c = [&]() {
            if constexpr(count_ptr) {
              return t_cell(*this, t);
            } else {
              return t_cell(t);
            }
          }();
          if constexpr(count_ptr) {
            dummy_c.m_count = 9001;
          }
          t_cell_ptr dummy(&dummy_c);
          // static_assert(std::is_same_v<typename value_type_wrap::t_hash, typename t_content::hasher>);
          // static_assert(std::is_same_v<typename value_type_wrap::t_eq, typename t_content::key_equal>);
          // std::cout << "registry_unique::add() -> " << std::boolalpha << (this->m_content.find(t) != this->m_content.end()) << std::endl;
          auto it = this->m_content.find(dummy);
          if(it == this->m_content.end()) {
            if constexpr(count_ptr){
              it = (this->m_content.emplace(*this, std::move(t))).first;
            } else {
              it = (this->m_content.emplace(std::move(t))).first;
            }
          }
          dummy.m_ptr = nullptr;
          return ref_struct::from_cell_ref(*it);
        }

        // bool contains(const T & t) const { return this->m_content.find(t) != this->m_content.end(); }

        const_iterator begin() { return ref_struct::make_iterator(this->m_content.cbegin()); }
        const_iterator end() { return ref_struct::make_iterator(this->m_content.cend()); }


        void clear() noexcept {
          using traits_t = std::allocator_traits<allocator_type>;
          for(auto& cell: this->m_content) {
            traits_t::destroy(this->m_alloc, cell.m_ptr);
            traits_t::deallocate(this->m_alloc, cell.m_ptr, 1);
          }
          this->m_content.clear();
        }

        template<bool b=count_ptr, std::enable_if_t<b, bool> = true>
        void clear(t_ptr& ref) {
          this->m_content.erase(ref);          
          // ref.destroy_value();
        }

      private:
        t_content m_content;
        allocator_type m_alloc;
      };
    };


    struct registry_shared {
      template<
        typename T,
        typename Hash = std::hash<T>,
        typename KeyEqual = std::equal_to<T>,
        template<typename> typename Allocator = std::allocator
      > struct make_ref {
        static_assert(!std::is_const_v<T>);
        using value_type  = T;
        using value_const = std::add_const_t<value_type>;
        using value_hash  = Hash;
        using value_equal = KeyEqual;
        using value_allocator = Allocator<value_type>;

        using ptr_type = std::shared_ptr<T>;
        using ptr_const = std::add_const_t<ptr_type>;
        using ptr_hash = std::hash<ptr_type>;
        using ptr_equal = std::equal_to<ptr_type>;

        static inline constexpr value_type * to_ptr(ptr_type& v) { return v.get(); }
        static inline constexpr value_const * to_ptr(ptr_const& v) { return v.get(); }
      };

      static inline constexpr bool ensure_unique_v = false;
      static inline constexpr bool ref_counting = true;

      template<
        typename T,
        typename Hash = std::hash<T>,
        typename KeyEqual = std::equal_to<T>,
        template<typename> typename Allocator = std::allocator
      > class make {
      public:
        using type = make<T, Hash, KeyEqual, Allocator>;
        using value_type = T;

        using ref_struct = make_ref<T, Hash, KeyEqual, Allocator>;
        using t_ptr  = typename ref_struct::ptr_type;

        t_ptr add(const T& t) { return std::shared_ptr<T>(new T(t)); }
        t_ptr add(T&& t) { return std::shared_ptr<T>(new T(std::move(t))); }

        void clear() noexcept {}
        void clear(t_ptr&) {}
      };
    };




    // the fact that the registry stores const values has effect on the type of iterators! because the content of the iterator is const
    template<typename R> struct is_make_ref_const {
      using t_ptr = typename R::ptr_type;
      struct is_const {
        template<typename T> using get_et = typename T::element_type;
        using manip_et = hrw::utils::inner_type<get_et>;
        static constexpr bool value = std::conditional_t<std::is_class_v<t_ptr>,
          std::is_const<typename manip_et::template dflt<int>::template get_t<t_ptr>>,
          std::is_const<std::remove_pointer_t<t_ptr>>
        >::value;
      };
      static constexpr bool value = is_const::value;
    };


    namespace _container_d {
      template<typename R, bool> struct get_type;
      template<typename R> struct get_type<R, true> {
        using T = int;
        using type = typename R::template make_ref<T>;
      };

      template<typename R> struct get_type<R, false> {
        using T = int;
        using type = typename R::template make_ref<T, std::hash<T>, std::equal_to<T>, std::allocator>;
      };
    }

    template<typename R> struct is_registry_const {
      using T = int;
      template<typename RR, typename=void> struct single_param: public std::false_type {};
      template<typename RR> struct single_param<RR, std::void_t<typename RR::template make_ref<T>>>: public std::true_type {};
      static inline constexpr bool single_param_v = single_param<R>::value;

      using type = typename _container_d::get_type<R, single_param_v>::type;
      static constexpr bool value = is_make_ref_const<type>::value;

    };

    template<typename R> constexpr bool is_make_ref_const_v = is_make_ref_const<R>::value;
    template<typename R> constexpr bool is_registry_const_v = is_registry_const<R>::value;


  }
}


#endif // __HREWRITE_UTILS_CONTAINER_H__

