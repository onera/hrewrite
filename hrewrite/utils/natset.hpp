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

#ifndef __HREWRITE_UTILS_NATSET_H__
#define __HREWRITE_UTILS_NATSET_H__

#include <iostream>
#include <algorithm>
#include <vector>
#include <bitset>
#include <iterator>
#include <sstream>

#include "hrewrite/utils/type_traits.hpp"
#include "hrewrite/utils/hash.hpp"
#include "hrewrite/exceptions/natset.hpp"

namespace hrw {
  namespace utils {


    namespace _natset_detail {
      using t_nat = unsigned int;
      using size_type = typename std::vector<bool>::size_type;

      template<typename arg_t_holder>
      class tt_iterator {
      public:
        using type = tt_iterator<arg_t_holder>;

        using difference_type = void;
        using value_type = t_nat;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::input_iterator_tag;

        using t_holder = arg_t_holder;

        tt_iterator(const t_holder& t, t_nat v): m_parent(t), m_i(v) {
          if(this->m_i < this->m_parent.size()) {
            if(!(this->m_parent[this->m_i])) {
              ++(*this);
            }
          }
        }

        tt_iterator& operator++() {
          do {
            ++this->m_i;
            if((this->m_i >= this->m_parent.size()) || (this->m_parent[this->m_i])) {
              return *this;
            }
          } while(true);
        }

        t_nat operator*() { return this->m_i; }

        bool operator==(const tt_iterator& it) const {
          return ((&(this->m_parent)) == (&(it.m_parent))) && (this->m_i == it.m_i);
        }

        bool operator!=(const tt_iterator& it) const {
         return ((&(this->m_parent)) != (&(it.m_parent))) || (this->m_i != it.m_i);
       }


      private:
        const t_holder& m_parent;
        t_nat m_i;
      };
    }


    enum e_natset_kind { STATIC, FIXED, FREE };
    enum e_natset_extensible_conf { CELL, BLOCK };
    template<e_natset_extensible_conf n> using p_natset_extensible_conf = std::integral_constant<e_natset_extensible_conf, n>;
  
    //////////////////////////////////////////
    //////////////////////////////////////////

    template<_natset_detail::t_nat limit, bool check=true>
    class natset_static {
    public:
      using type = natset_static<limit, check>;
      using t_nat = _natset_detail::t_nat;
      using size_type = _natset_detail::size_type;
      using t_content = std::bitset<limit>;

      inline static constexpr e_natset_kind kind = e_natset_kind::STATIC;

      //////////////////////////////////////////
      // 1. constructors / destructors
      natset_static(): m_content() {}
      natset_static(const type& s): m_content(s.m_content) {}
      natset_static(size_type t): m_content() { this->check_size(t); }

      natset_static(size_type t, std::initializer_list<t_nat> init) {
        this->check_size(t);
        this->add(init);
      }
      template<typename inputIt>
      natset_static(size_type t, inputIt begin, inputIt end): m_content() {
        this->check_size(t);
        this->add(begin, end);
      }
      ~natset_static() = default;

      //////////////////////////////////////////
      // 2. adding / removing
      void add(t_nat val) {
        if constexpr(check) {
          if(val >= limit) {
            throw hrw::exception::natset_cannot_contain<type>(val);
          }
        }
        this->m_content[val] = 1;
      }

      void add(std::initializer_list<t_nat> init) { for(auto v: init) { this->add(v); } }
      template<typename inputIt>
      void add(inputIt begin, inputIt end) { while(begin != end) { this->add(*begin); ++begin; } }
      void add(const type& s) { this->m_content |= s.m_content; }
      void cup_update(const type& s) { this->add(s); }

      void cap_update(const type& s) { this->m_content &= s.m_content; }

      void rm(t_nat val)  {
        if(val <= limit) {
          this->m_content[val] = 0;
        }
      }

      void insert(t_nat val) { this->add(val); }
      void insert(std::initializer_list<t_nat> init) { this->add(init); }
      template<typename inputIt>
      void insert(inputIt begin, inputIt end) { this->add(begin, end); }
      void insert(const type& s) { this->add(s); }

      void erase(t_nat val) { this->rm(val); }
      void clear() { this->m_content.reset(); }
      constexpr t_nat size() const { return limit; }



      //////////////////////////////////////////
      // 3. iterator

      bool operator[](t_nat val) const { return this->contains(val); }

      using const_iterator = _natset_detail::tt_iterator<type>;
      friend const_iterator;

      const_iterator begin() const { return const_iterator(*this, 0); }
      const_iterator end() const { return const_iterator(*this, limit);}

      //////////////////////////////////////////
      // 4. set operations
      type cup(const type& s) const { return type(this->m_content | s.m_content); }
      type cap(const type& s) const { return type(this->m_content & s.m_content); }

      //////////////////////////////////////////
      // 5. testing
      bool contains(t_nat val) const  {
        if(val <= limit) {
          return this->m_content[val];
        } else {
          return false;
        }
      }

      bool empty() const { return this->m_content.none(); }


      //////////////////////////////////////////
      // 4. operators / friends
      type& operator=(const type& s) { this->m_content = s.m_content; return *this; }

      bool operator==(const type& s) const { return this->m_content == s.m_content; }

      friend std::ostream& operator<<(std::ostream& os, const type& s) {
        os << "[ ";
        for(t_nat i = 0; i < limit; ++i) {
          if(s.m_content[i]) {
            os << i << " ";
          }
        }
        os << "]";
        return os;
      }

      friend void swap(type& s1, type& s2) {
        using std::swap;
        swap(s1.m_content, s2.m_content);
      }

      struct t_hash {
        using value_type = type;
        using H = hrw::utils::hash<t_content>;
        hrw::utils::hash_value operator()(const value_type& t) {
          return H()(t.m_content);
        }
      };
      struct t_eq   { constexpr bool operator()( const type& lhs, const type& rhs ) const { return lhs == rhs; } };

    private:
      t_content m_content;

      natset_static(t_content&& m): m_content(m) {}

      inline void check_size(size_type t) {
        if(t >= limit) {
          throw hrw::exception::natset_limit<type>(t);
        }
      }
    };



    //////////////////////////////////////////
    //////////////////////////////////////////

    template<bool check=true>
    class natset_fixed {
    public:
      using type = natset_fixed<check>;
      using t_nat = _natset_detail::t_nat;
      using t_content = std::vector<bool>;
      using size_type = typename t_content::size_type;

      inline static constexpr e_natset_kind kind = e_natset_kind::FIXED;

      //////////////////////////////////////////
      // 1. constructors / destructors
      natset_fixed(const type& s): m_content(s.m_content) {}
      natset_fixed(size_type max): m_content(max) {}

      natset_fixed(size_type max, std::initializer_list<t_nat> init): m_content(max) { this->add(init); }
      template<typename inputIt>
      natset_fixed(size_type max, inputIt begin, inputIt end): m_content(max) { this->add(begin, end); }
      ~natset_fixed() = default;

      //////////////////////////////////////////
      // 2. adding / removing
      void add(t_nat val) {
        if constexpr(check) {
          if(val > this->m_content.size()) {
            throw hrw::exception::natset_cannot_contain<type>(val);
          }
        }
        this->m_content[val] = 1;
      }

      void add(std::initializer_list<t_nat> init) { for(auto v: init) { this->add(v); } }
      template<typename inputIt>
      void add(inputIt begin, inputIt end) { while(begin != end) { this->add(*begin); ++begin; } }
      void add(const type& s) {
        if(s.m_content.size() <= this->m_content.size()) {
          for(auto i=0; i < s.m_content.size(); ++i) {
            this->m_content[i] = this->m_content[i] || s.m_content[i];
          }
        }
      }
      void cup_update(const type& s) { this->add(s); }

      void cap_update(const type& s) {
        auto max = std::min(s.m_content.size(), this->m_content.size());
        auto i=0;
        for(; i < max; ++i) {
          this->m_content[i] = this->m_content[i] && s.m_content[i];
        }
        for(; i < this->m_content.size(); ++i) {
          this->m_content[i] = 0;
        }
      }

      void rm(t_nat val)  {
        if(val <= this->m_content.size()) {
          this->m_content[val] = 0;
        }
      }

      void insert(t_nat val) { this->add(val); }
      void insert(std::initializer_list<t_nat> init) { this->add(init); }
      template<typename inputIt>
      void insert(inputIt begin, inputIt end) { this->add(begin, end); }
      void insert(const type& s) { this->add(s); }

      void erase(t_nat val) { this->rm(val); }
      void clear() { std::fill(this->m_content.begin(), this->m_content.end(), 0);; }
      constexpr size_type size() const { return this->m_content.size(); }



      //////////////////////////////////////////
      // 3. iterator

      bool operator[](t_nat val) const { return this->contains(val); }

      using const_iterator = _natset_detail::tt_iterator<type>;
      friend const_iterator;

      const_iterator begin() const { return const_iterator(*this, 0); }
      const_iterator end() const { return const_iterator(*this, this->m_content.size());}


      //////////////////////////////////////////
      // 5. testing
      bool contains(t_nat val) const  {
        if constexpr(check) {
          if(val <= this->m_content.size()) {
            return this->m_content[val];
          } else {
            return false;
          }
        } else {
          return this->m_content[val];
        }
      }

      bool empty() const { return std::all_of(this->m_content.begin(), this->m_content.end(), [](bool b) {return b; }); }


      //////////////////////////////////////////
      // 4. operators / friends
      bool operator==(const type& s) const { return this->m_content == s.m_content; }

      friend std::ostream& operator<<(std::ostream& os, const type& s) {
        os << "[ ";
        for(t_nat i = 0; i < s.m_content.size(); ++i) {
          if(s.m_content[i]) {
            os << i << " ";
          }
        }
        os << "]";
        return os;
      }

      friend void swap(type& s1, type& s2) {
        using std::swap;
        swap(s1.m_content, s2.m_content);
      }

      struct t_hash {
        using value_type = type;
        using H = hrw::utils::hash<t_content>;
        hrw::utils::hash_value operator()(const value_type& t) {
          return H()(t.m_content);
        }
      };
      struct t_eq   { constexpr bool operator()( const type& lhs, const type& rhs ) const { return lhs == rhs; } };

    private:
      t_content m_content;
    };




    //////////////////////////////////////////
    //////////////////////////////////////////

    template<std::size_t block_size=64>
    class natset_extensible {
    public:
      using type = natset_extensible;
      using t_nat = _natset_detail::t_nat;
      using size_type = _natset_detail::size_type;

      using t_cell = std::bitset<block_size>;
      using t_content = std::vector<t_cell>;

      using t_info = typename type_as_bits<t_cell>::template make<_natset_detail::t_nat, typename t_content::size_type>;
      using t_cell_index = typename t_info::t_cell_index;
      using t_inner_index = typename t_info::t_inner_index;

      inline static constexpr e_natset_kind kind = e_natset_kind::FREE;

      //////////////////////////////////////////
      // 1. constructors / destructors
      natset_extensible(): m_content() {}
      natset_extensible(const type& s): m_content(s.m_content) {}
      natset_extensible(type&& s): m_content(std::move(s.m_content)) {}

      template<e_natset_extensible_conf c=e_natset_extensible_conf::CELL>
      natset_extensible(size_type t, [[maybe_unused]] p_natset_extensible_conf<c> v=p_natset_extensible_conf<c>()): m_content([&](){
        if constexpr(c == e_natset_extensible_conf::CELL) {
          return ((t/block_size)+1);
        } else {
          return t;
        }
      }()) {}
  
      template<typename inputIt, e_natset_extensible_conf c=e_natset_extensible_conf::CELL>
      natset_extensible(size_type t, inputIt begin, inputIt end, p_natset_extensible_conf<c> v=p_natset_extensible_conf<c>()): natset_extensible(t, v) { this->add(begin, end); }
      template<e_natset_extensible_conf c=e_natset_extensible_conf::CELL>
      natset_extensible(size_type t, std::initializer_list<t_nat> init, p_natset_extensible_conf<c> v=p_natset_extensible_conf<c>()): natset_extensible(t, v) { for(auto v: init) { this->_add_inner(v); } }

     ~natset_extensible() = default;

      //////////////////////////////////////////
      // 2. adding / removing
      void add(t_nat val) {
          type::t_cell_index idx = t_info::right_shift(val);
          if(this->m_content.size() <= idx) {
            this->m_content.resize(idx+1);
          }
          this->_add_inner(val, idx);
      }
      void add(std::initializer_list<t_nat> init) {
        auto it = std::max_element(init.begin(), init.end());
        if(it != init.end()) {
          type::t_cell_index idx = t_info::right_shift(*it);
          if(this->m_content.size() <= idx) {
            this->m_content.resize(idx+1);
          }
          for(auto v: init) { this->_add_inner(v); }
        }
      }
      template<typename inputIt>
      void add(inputIt begin, inputIt end) { while(begin != end) { this->add(*begin); ++begin; } }
      void add(const type& s) {
        type::t_cell_index size = s.m_content.size();
        if(this->m_content.size() < size) {
          this->m_content.resize(size);
        }
        for(t_cell_index i = 0; i < size; ++i) {
          this->m_content[i] |= s.m_content[i];
        }
      }
      void cup_update(const type& s) { this->add(s); }

      void cap_update(const type& s) {
        type::t_cell_index size = std::min(this->m_content.size(), s.m_content.size());
        this->m_content.resize(size);
        for(t_cell_index i = 0; i < size; ++i) {
          this->m_content[i] &= s.m_content[i];
        }
      }

      void rm(t_nat val)  {
        type::t_cell_index idx = t_info::right_shift(val);
        if (this->m_content.size() > idx) {
          // t_cell mask = t_info::get_mask(val);
          this->m_content[idx][t_info::get_inner_index(val)] = 0;
        }
      }


      void insert(t_nat val) { this->add(val); }
      void insert(std::initializer_list<t_nat> init) { this->add(init); }
      template<typename inputIt>
      void insert(inputIt begin, inputIt end) { this->add(begin, end); }
      void insert(const type& s) { this->add(s); }

      void erase(t_nat val) { this->rm(val); }
      void clear() { this->m_content.clear(); }


      //////////////////////////////////////////
      // 3. iterator

      bool operator[](t_nat val) const { return this->contains(val); }

      using const_iterator = _natset_detail::tt_iterator<type>;
      friend const_iterator;

      const_iterator begin() const { return const_iterator(*this, 0); }
      const_iterator end() const { return const_iterator(*this, this->size());}

      std::size_t size() const { return this->m_content.size() * t_info::s_cell; }


      //////////////////////////////////////////
      // 4. set operations
      type cup(const type& s) const {
        t_cell_index size_max = std::max(this->m_content.size(), s.m_content.size());
        t_cell_index size_min = std::min(this->m_content.size(), s.m_content.size());
        type res(size_max, p_natset_extensible_conf<e_natset_extensible_conf::BLOCK>());
        t_cell_index i = 0;
        for(; i < size_min; ++i) {
          res.m_content[i] = this->m_content[i] | s.m_content[i];
        }
        for(; i < this->m_content.size(); ++i) {
          res.m_content[i] = this->m_content[i];
        }
        for(; i < s.m_content.size(); ++i) {
          res.m_content[i] = s.m_content[i];
        }
        return res;
      }

      type cap(const type& s) const {
        t_cell_index size = std::min(this->m_content.size(), s.m_content.size());
        type res(size, p_natset_extensible_conf<e_natset_extensible_conf::BLOCK>());
        for(t_cell_index i = 0; i < size; ++i) {
          res.m_content[i] = this->m_content[i] & s.m_content[i];
        }
        return res;
      }

      //////////////////////////////////////////
      // 5. testing
      bool contains(t_nat val) const  {
        t_cell_index idx = t_info::right_shift(val);
        if(idx < this->m_content.size()) {
          // std::cout << "contains(" << val << ") -> idx=" << ((unsigned int)idx) << ", in=" << ((unsigned int)t_info::get_inner_index(val)) << std::endl;
          return this->m_content[idx][t_info::get_inner_index(val)];
        } else {
          return false;
        }
      }

      bool empty() const {
        for(auto& i: this->m_content) {
          if(i.none()) { return false; }
        }
        return true;
      }

      //////////////////////////////////////////
      // 6. operators / friends
      type& operator=(const type& s) { this->m_content = s.m_content; return *this; }

      friend std::ostream& operator<<(std::ostream& os, const type& s) {
        os << "[ ";
        for(t_cell_index i = 0; i < s.m_content.size(); ++i) {
          for(t_inner_index j=0; j <= t_info::l_cell; ++j) {
            if(s.m_content[i][j]) {
              os << ((t_info::left_shift(i)) + j) << " ";
            }
          }
        }
        os << "]";
        return os;
      }

      friend void swap(type& s1, type& s2) {
        using std::swap;
        swap(s1.m_content, s2.m_content);
      }

    private:
      inline void _add_inner(t_nat val) { this->_add_inner(val, t_info::right_shift(val)); }
      inline void _add_inner(t_nat val, t_cell_index idx) { this->m_content[idx][t_info::get_inner_index(val)] = 1; }

      t_content m_content;
    };


    /**
     * This class extends the first one with an annex array that stores the values in the set.
     * That way, iterating over the values of the set is linear in the number of elements (and not linear in the max possible element)
     * Again, if necessary, we can extend this with other set operations (union, intersection, subset testing, etc)
     */
    template<typename targ_core>
    class tt_natset {
    public:
      using type = tt_natset<targ_core>;
      using t_core = targ_core;
      using t_nat = typename t_core::t_nat;
      using t_content = std::vector<t_nat>;
      using size_type = typename t_content::size_type;
 
      inline static constexpr e_natset_kind kind = t_core::kind;

      //////////////////////////////////////////
      // 1. constructors / destructors
      tt_natset(): m_core(), m_content() {}
      tt_natset(const type& s): m_core(s.m_core), m_content(s.m_content) {}

      tt_natset(size_type t): m_core(t), m_content() {}
      tt_natset(size_type t, std::initializer_list<t_nat> init): m_core(t, init), m_content(init) {}
      template<typename inputIt>
      tt_natset(size_type t, inputIt begin, inputIt end): m_content(t) { this->add(begin, end); }
      ~tt_natset() = default;


      //////////////////////////////////////////
      // 2. adding / removing
      void add(t_nat val) {
        if(!this->m_core.contains(val)) {
          this->m_core.add(val);
          this->m_content.push_back(val);
        }
      }
      void add(std::initializer_list<t_nat> init) {
        for(auto val: init) {
          if(!this->m_core.contains(val)) {
            this->m_content.push_back(val);
          }
        }
        this->m_core.add(init);
      }

      template<typename inputIt>
      void add(inputIt begin, inputIt end) { while(begin != end) { this->add(*begin); ++begin; } }

      void add(const type& s) {
        for(auto val: s.m_content) {
          if(!this->m_core.contains(val)) {
            this->m_content.push_back(val);
          }
        }
        this->m_core.add(s.m_core);
      }
      void cup_update(const type& s) { this->add(s); }


      void cap_update(const type& s) {
        auto it_end = std::remove_if(this->m_content.begin(), this->m_content.end(), [&s](t_nat val) { return !s.m_core.contains(val); });
        this->m_content.erase(it_end, this->m_content.end());
        this->m_core.cap_update(s.m_core);
      }

      void rm(t_nat val)  {
        this->m_core.rm(val);
        auto it_end = std::remove(this->m_content.begin(), this->m_content.end(), val);
        this->m_content.erase(it_end, this->m_content.end());
      }


      void insert(t_nat val) { this->add(val); }
      void insert(std::initializer_list<t_nat> init) { this->add(init); }
      template<typename inputIt>
      void insert(inputIt begin, inputIt end) { this->add(begin, end); }
      void insert(const type& s) { this->add(s); }

      void erase(t_nat v) { this->rm(v); }
      void clear() { this->m_core.clear(); this->m_content.clear(); }


      //////////////////////////////////////////
      // 3. iterator

      bool operator[](t_nat val) { return this->m_core[val]; }

      using const_iterator = std::vector<unsigned int>::const_iterator;

      const_iterator begin() const { return this->m_content.begin(); }
      const_iterator end() const { return this->m_content.end(); }

      std::size_t size() const { return this->m_core.size(); }


      //////////////////////////////////////////
      // 4. set operations
      type cup(const type& s) const {
        // construct the vector
        t_content tmp;
        tmp.reserve(this->m_content.size() + s.m_content.size());
        tmp.insert(tmp.begin(), this->m_content.begin(), this->m_content.end());
        for(auto val: s.m_content) {
          if(!this->m_core.contains(val)) {
            tmp.push_back(val);
          }
        }
        return type(std::move(tmp));
      }

      type cap(const type& s) const {
        t_content tmp;
        size_type size = std::min(this->m_content.size(), s.m_content.size());
        tmp.reserve(size);

        t_content const& v1 = (this->m_content.size() == size)?(this->m_content):(s.m_content);
        t_core const& v2 = (this->m_content.size() == size)?(s.m_core):(this->m_core);

        for(auto val: v1) {
          if(v2.contains(val)) {
            tmp.push_back(val);
          }
        }
        return type(std::move(tmp));
      }

      //////////////////////////////////////////
      // 5. testing

      bool contains(unsigned int val) const { return this->m_core.contains(val); }
      bool empty() const { return this->m_content.empty(); }

 
       //////////////////////////////////////////
      // 6. operators / friends
      type& operator=(const type& s) { this->m_content = s.m_content; this->m_core = s.m_core; return *this; }

      friend std::ostream& operator<<(std::ostream& os, const type& s) {
        using t_data = data_wrapper<const t_content&>;
        return (os << t_data(s.m_content));
      }

      friend void swap(type& s1, type& s2) {
        using std::swap;
        swap(s1.m_core, s2.m_core);
        swap(s1.m_content, s2.m_content);
      }

     const t_core& get_core() const { return this-> m_core; }

    private:
      t_core m_core;
      t_content m_content;

      tt_natset(t_content&& content): m_core(), m_content(std::move(content)) {
        for(auto val: this->m_content) {
          this->m_core.add(val);
        }
      }
    };


    // using natset = tt_natset<natset_core>;
    using natset = natset_static<64>;

  }
}


#endif // __HREWRITE_UTILS_NATSET_H__

