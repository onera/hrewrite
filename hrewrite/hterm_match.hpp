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


#ifndef __HREWRITE_HTERM_MATCH_H__
#define __HREWRITE_HTERM_MATCH_H__

#include "hrewrite/utils.hpp"
#include "hrewrite/theory/core.hpp"

#include <iostream>
#include <vector>

namespace hrw {

  template<typename rw_t, typename substitution_t>
  using t_guard = std::function<bool(rw_t *, substitution_t *)>;


  template<typename termIt>
  struct tt_term_full_match_helper_iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = t_sort_id;
    using difference_type = std::ptrdiff_t;
    using pointer = t_sort_id*;
    using reference = t_sort_id&;
    using type = tt_term_full_match_helper_iterator<termIt>;

    tt_term_full_match_helper_iterator(termIt it): m_it(it) {}
    t_sort_id operator*() { return (*this->m_it)->get_sort(); }
    tt_term_full_match_helper_iterator& operator++() { ++(this->m_it); return *this; }
    bool operator==(const tt_term_full_match_helper_iterator& other) const { return this->m_it == other.m_it; }

    termIt m_it;
  };


  struct t_match_data_empty {};

  ///////////////////////////////////////////////////////////////////////////////
  // STANDARD IMPLEMENTATION: FOR ELEMENT SPEC
  ///////////////////////////////////////////////////////////////////////////////

  template<typename tt_term_full, typename t_data=t_match_data_empty>
  struct tt_term_full_match_element {
    using type = tt_term_full_match_element<tt_term_full, t_data>;
    using reference = typename tt_term_full::reference;
    using t_variable = typename tt_term_full::t_variable;
    using t_substitution = typename tt_term_full::t_substitution;

    static constexpr bool is_const_v = tt_term_full::is_const_v;
    static constexpr bool has_manage = not std::is_same_v<t_data, t_match_data_empty>;

    template<typename T=t_data, std::enable_if_t<std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match_element() {}

    template<typename T=t_data, std::enable_if_t<not std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match_element(t_data d): m_data(d) {}

    template<typename T>
    bool match_term(T* pattern, reference tground, t_substitution& subst) {
      static_assert(std::is_same_v<std::decay_t<T>, tt_term_full>);
      match_term_helper obj(tground, subst, *this);
      return VISIT_SINGLE(obj, pattern->m_content);
    }

    template<typename T, typename rw_t>
    bool match_term(rw_t * rw, T* pattern, reference tground, t_substitution& subst, t_guard<rw_t, t_substitution> & guard) {
      static_assert(std::is_same_v<std::decay_t<T>, tt_term_full>);
      match_term_helper obj(tground, subst, *this);
      if(VISIT_SINGLE(obj, pattern->m_content)) {
        return guard(rw, &subst);
      }
      return false;
    }

    template<typename termIt1, typename termIt2>
    bool match_term(termIt1 p_current, termIt1 p_end, termIt2 tg_current, termIt2 tg_end, t_substitution& subst) {
      while(p_current != p_end) {
        if(tg_current == tg_end) {
          return false;
        } else {
          reference pattern = *p_current;
          reference tground = *tg_current;
          match_term_helper obj(tground, subst, *this);
          bool success = VISIT_SINGLE(obj, pattern->m_content);
          if(!success) { return false; }
          ++p_current;
          ++tg_current;
        }
      }
      return (tg_current == tg_end);
    }

  private: 
    struct match_term_helper {
      match_term_helper(reference tg, t_substitution& subst, type& m): m_tg(tg), m_subst(subst), m_match(m) {}
      template<typename T>
      bool operator()(T& p) {
        using t_term_core = std::decay_t<T>; // the type stored in the t_content variant
        using t_term_cv = std::conditional_t<is_const_v, std::add_const_t<t_term_core>, t_term_core>; // the type as it is used (if is_const_v, then the iterators are const)
        if constexpr(std::is_same_v<t_term_core, t_variable>) {
          if(p.get_spec().get_letter() == this->m_tg->get_sort()) {
            this->m_subst.insert(&p, this->m_tg);
            return true;
          } else {
            return false;
          }
        } else {
          t_term_cv* tt = this->m_tg->template get_if<t_term_core>();
          if(tt == &p) {
            return true;
          } else {
            if constexpr(!has_manage) {
              return ((tt != nullptr) && (p.template match<t_substitution, type>(*tt, this->m_subst, this->m_match)));
            } else {
              if((tt != nullptr) && (p.match_shallow(*tt))) {
                return p.template match_subterms<t_substitution, type>(*tt, this->m_subst, this->m_match);
              } else {
                this->m_tg = this->m_match.m_data.manage(this->m_tg);
                tt = this->m_tg->template get_if<t_term_core>();
                return ((tt != nullptr) && (p.template match<t_substitution, type>(*tt, this->m_subst, this->m_match)));
              }
            }
          }
        }
      }

    private:
      reference m_tg;
      t_substitution& m_subst;
      type& m_match;
    };

  private:
    using t_content = std::conditional_t<std::is_same_v<t_data, t_match_data_empty>, t_match_data_empty, t_data>;
    t_content m_data;  

  };


  ///////////////////////////////////////////////////////////////////////////////
  // LIST IMPLEMENTATION: FOR LIST SPEC
  ///////////////////////////////////////////////////////////////////////////////

  template<typename tt_term_full, typename t_data=t_match_data_empty>
  struct tt_term_full_match_sequence {
    using type = tt_term_full_match_sequence<tt_term_full, t_data>;
    using reference = typename tt_term_full::reference;
    using t_variable = typename tt_term_full::t_variable;
    using t_substitution = typename tt_term_full::t_substitution;
    using single_iterator = typename tt_term_full::single_iterator;

    static constexpr bool is_const_v = tt_term_full::is_const_v;
    static constexpr bool has_manage = !std::is_same_v<t_data, t_match_data_empty>;

    template<typename T=t_data, std::enable_if_t<std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match_sequence() {}

    template<typename T=t_data, std::enable_if_t<!std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match_sequence(t_data d): m_data(d) {}

    template<typename T>
    bool match_term(T* pattern, reference tground, t_substitution& subst) {
      static_assert(std::is_same_v<std::decay_t<T>, tt_term_full>);
      auto tg_begin = single_iterator(tground);
      auto tg_end = single_iterator();
      match_term_helper<single_iterator> obj(tg_begin, tg_end, subst, *this);
      return VISIT_SINGLE(obj, pattern->m_content);
    }

    template<typename T, typename rw_t>
    bool match_term(rw_t * rw, T* pattern, reference tground, t_substitution& subst, t_guard<rw_t, t_substitution> & guard) {
      static_assert(std::is_same_v<std::decay_t<T>, tt_term_full>);
      auto tg_begin = single_iterator(tground);
      auto tg_end = single_iterator();
      match_term_helper<single_iterator> obj(tg_begin, tg_end, subst, *this);
      if(VISIT_SINGLE(obj, pattern->m_content)) {
        return guard(rw, &subst);
      }
      return false;
    }

    template<typename termIt1, typename termIt2>
    bool match_term(termIt1 p_current, termIt1 p_end, termIt2 tg_current, termIt2 tg_end, t_substitution& subst) {
      while(p_current != p_end) {
        if(tg_current == tg_end) {
          return false;
        } else {
          reference pattern = *p_current;
          match_term_helper<termIt2> obj(tg_current, tg_end, subst, *this);
          bool success = VISIT_SINGLE(obj, pattern->m_content);
          if(!success) { return false; }
          ++p_current;
          tg_current = obj.m_res;
        }
      }
      return (tg_current == tg_end);
    }

  private:
    template<typename termIt>
    struct match_term_helper {
      match_term_helper(termIt tg_current, termIt tg_end, t_substitution& subst, type& m):
        m_tg_current(tg_current), m_tg_end(tg_end), m_res(tg_end), m_subst(subst), m_match(m) {}
      template<typename T>
      bool operator()(T& p) {
        using t_term_core = std::decay_t<T>; // the type stored in the t_content variant
        using t_term_cv = std::conditional_t<is_const_v, std::add_const_t<t_term_core>, t_term_core>; // the type as it is used (if is_const_v, then the iterators are const)
        if constexpr(std::is_same_v<t_term_core, t_variable>) {
          auto m = hrw::utils::match(
            p.get_spec(),
            tt_term_full_match_helper_iterator<termIt>(this->m_tg_current),
            tt_term_full_match_helper_iterator<termIt>(this->m_tg_end)
          );
          auto res = m.begin();
          if(res != m.end()) {
            this->m_subst.insert(&p, this->m_tg_current, (*res).m_it);
            this->m_res = (*res).m_it;
            return true;
          } else {
            return false;
          }
        } else {
          t_term_cv* tt = std::get_if<t_term_core>(&((*this->m_tg_current)->m_content));
          if(tt != nullptr) {
            this->m_res = this->m_tg_current+1;
            return ((&p == tt) || (p.template match<t_substitution, type>(*tt, this->m_subst, this->m_match)));
          } else {
            return false;
          }
        }
      }

    private:
      termIt m_tg_current;
      termIt m_tg_end;
      type& m_match;
      t_substitution& m_subst;
    public:
      termIt m_res;
    };

  private:
    using t_content = std::conditional_t<std::is_same_v<t_data, t_match_data_empty>, t_match_data_empty, t_data>;
    t_content m_data;  

  };



  ///////////////////////////////////////////////////////////////////////////////
  // FULL BACKTRACK IMPLEMENTATION
  ///////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////
  // utils
  template<typename t_data, typename t_rw, typename t_substitution>
  struct tt_term_full_match_backtrack_content {
    tt_term_full_match_backtrack_content(t_data & data, t_rw * rw, t_guard<t_rw, t_substitution> & guard): m_data(data), m_rw(rw), m_guard(guard) {}
    t_data & m_data;
    t_rw * m_rw;
    t_guard<t_rw, t_substitution> & m_guard;
  };
  template<typename t_rw, typename t_substitution>
  struct tt_term_full_match_backtrack_content<void, t_rw, t_substitution> {
    tt_term_full_match_backtrack_content(t_rw * rw, t_guard<t_rw, t_substitution> & guard): m_rw(rw), m_guard(guard) {}
    t_rw * m_rw;
    t_guard<t_rw, t_substitution> & m_guard;
  };
  template<typename t_data, typename t_substitution>
  struct tt_term_full_match_backtrack_content<t_data, void, t_substitution> {
    tt_term_full_match_backtrack_content(t_data & data): m_data(data) {}
    t_data & m_data;
  };
  template<typename t_substitution>
  struct tt_term_full_match_backtrack_content<void, void, t_substitution> {
  };


  template<typename tt_term_full, typename t_data, typename t_rw>
  struct tt_term_full_match_backtrack_inner {
    using type = tt_term_full_match_backtrack_inner<tt_term_full, t_data, t_rw>;
    using reference = typename tt_term_full::reference;
    using t_variable = typename tt_term_full::t_variable;
    using t_substitution = typename tt_term_full::t_substitution;
    using single_iterator = typename tt_term_full::single_iterator;

    static constexpr bool is_const_v = tt_term_full::is_const_v;
    static constexpr bool has_manage = !std::is_same_v<t_data, t_match_data_empty>;

    template<typename T=t_data, typename R=t_rw, std::enable_if_t<std::is_same_v<T, void> and std::is_same_v<R, void>, bool> = true>
    tt_term_full_match_backtrack_inner(): m_content(), m_stack() {}
    template<typename T=t_data, typename R=t_rw, std::enable_if_t<(not std::is_same_v<T, void>) and std::is_same_v<R, void>, bool> = true>
    tt_term_full_match_backtrack_inner(T & d): m_content(d), m_stack() {}
    template<typename T=t_data, typename R=t_rw, std::enable_if_t<std::is_same_v<T, void> and (not std::is_same_v<R, void>), bool> = true>
    tt_term_full_match_backtrack_inner(R * rw, t_guard<R, t_substitution> & guard): m_content(rw, guard), m_stack() {}
    template<typename T=t_data, typename R=t_rw, std::enable_if_t<(not std::is_same_v<T, void>) and (not std::is_same_v<R, void>), bool> = true>
    tt_term_full_match_backtrack_inner(T & d, R * rw, t_guard<R, t_substitution> & guard): m_content(d, rw, guard), m_stack() {}

    template<typename T>
    bool match_term(T* pattern, reference tground, t_substitution& subst) {
      static_assert(std::is_same_v<std::decay_t<T>, tt_term_full>);
      using t_iterator = hrw::utils::iterator_single<T*, true>;
      auto t1_begin = t_iterator(pattern);
      auto t1_end = t_iterator();
      auto t2_begin = single_iterator(tground);
      auto t2_end = single_iterator();
      return this->match_term(t1_begin, t1_end, t2_begin, t2_end, subst);
    }


    template<typename termIt1, typename termIt2>
    bool match_term(termIt1 p_current, termIt1 p_end, termIt2 tg_current, termIt2 tg_end, t_substitution& subst) {
      if(p_current != p_end) {
        auto t = *p_current;
        match_term_helper<termIt1, termIt2> obj(p_current+1, p_end, tg_current, tg_end, subst, *this);
        return VISIT_SINGLE(obj, t->m_content); // direct access to term content
      } else if(tg_current == tg_end) {
        if(this->m_stack.size() == 0) {
          if constexpr(not std::is_same_v<t_rw, void>) { return this->m_content.m_guard(this->m_content.m_rw, &subst); }
          else { return true; }
        } else {
          auto pair_el = this->m_stack.back();
          this->m_stack.pop_back();
          match_term_wrapper obj(subst, *this);
          if(not VISIT_SINGLE(obj, pair_el.first, pair_el.second)) {
            this->m_stack.push_back(pair_el);
            return false;  
          }
          return true;
        }
      }
      return false;
    }

  private:
    template<typename termIt1, typename termIt2>
    struct match_term_helper {
      match_term_helper(termIt1 p_next, termIt1 p_end, termIt2 tg_current, termIt2 tg_end, t_substitution& subst, type& m):
        m_p_next(p_next), m_p_end(p_end), m_tg_current(tg_current), m_tg_end(tg_end), m_subst(subst), m_match(m) {}

      template<typename T>
      bool operator()(T& pattern) {
        using t_term_core = std::decay_t<T>; // the type stored in the t_content variant
        using t_term_cv = std::conditional_t<is_const_v, std::add_const_t<t_term_core>, t_term_core>; // the type as it is used (if is_const_v, then the iterators are const)
        if constexpr(std::is_same_v<t_term_core, t_variable>) {
          auto m = hrw::utils::match(
            pattern.get_spec(),
            tt_term_full_match_helper_iterator<termIt2>(this->m_tg_current),
            tt_term_full_match_helper_iterator<termIt2>(this->m_tg_end)
          );
          for(auto it: m) {
            // std::cout << "  found solution" << std::endl;
            this->m_subst.insert(&pattern, this->m_tg_current, it.m_it);
            if(this->m_match.match_term(this->m_p_next, this->m_p_end, it.m_it, this->m_tg_end, this->m_subst)) { return true; }
          }
          return false;
        } else {
          if(this->m_tg_current != this->m_tg_end) {
            t_term_cv* tt = std::get_if<t_term_core>(&((*(this->m_tg_current))->m_content));
            if(tt != nullptr) {
              if(&pattern == tt) { return this->m_match.match_term(this->m_p_next, this->m_p_end, this->m_tg_current+1, this->m_tg_end, this->m_subst); }
              if constexpr(hrw::has_container_v<t_term_core>) {
                this->m_match.m_stack.push_back(std::make_pair(
                  t_stack_el(std::make_pair(this->m_p_next, this->m_p_end)),
                  t_stack_el(std::make_pair(this->m_tg_current+1, this->m_tg_end))
                ));
                if(not pattern.template match<t_substitution, type>(*tt, this->m_subst, this->m_match)) {
                  this->m_match.m_stack.pop_back();
                  return false;
                }
                return true;
              } else {
                if(pattern.template match<t_substitution, type>(*tt, this->m_subst, this->m_match)) {
                  return this->m_match.match_term(this->m_p_next, this->m_p_end, this->m_tg_current+1, this->m_tg_end, this->m_subst);
                } else {
                  return false;
                }
              }
            }
          }
          return false;
        }
      }

    private:
      termIt1 m_p_next;
      termIt1 m_p_end;
      termIt2 m_tg_current;
      termIt2 m_tg_end;
      t_substitution& m_subst;
      type& m_match;

    };

  struct match_term_wrapper {
    match_term_wrapper(t_substitution& subst, type& m): m_subst(subst), m_match(m) {}
    template<typename termIt1, typename termIt2>
    bool operator()(std::pair<termIt1, termIt1> p, std::pair<termIt2, termIt2> tg) {
      return this->m_match.match_term(p.first, p.second, tg.first, tg.second, this->m_subst);
    }

    private:
      t_substitution& m_subst;
      type& m_match;
  };

  private:
    using t_content = tt_term_full_match_backtrack_content<t_data, t_rw, t_substitution>;
    t_content m_content;

    // construct the type form the stack, which may contain any pairs or pairs of iterators in a term
    template<typename T> using to_pairs = std::pair<T, T>;
    using t_iterator_pairs = hrw::utils::tuple_map_t<to_pairs, typename t_substitution::t_iterators>;
    using t_stack_el = hrw::utils::tuple_convert_t<std::variant, t_iterator_pairs>;
    std::vector<std::pair<t_stack_el, t_stack_el>> m_stack;
  };



  //////////////////////////////////////////
  // main class

  template<typename tt_term_full, typename t_data=t_match_data_empty>
  struct tt_term_full_match_backtrack {
    using type = tt_term_full_match_backtrack<tt_term_full, t_data>;
    using reference = typename tt_term_full::reference;
    using t_variable = typename tt_term_full::t_variable;
    using t_substitution = typename tt_term_full::t_substitution;
    using single_iterator = typename tt_term_full::single_iterator;

    static constexpr bool is_const_v = tt_term_full::is_const_v;
    static constexpr bool has_manage = !std::is_same_v<t_data, t_match_data_empty>;

    template<typename T=t_data, std::enable_if_t<std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match_backtrack() {}

    template<typename T=t_data, std::enable_if_t<!std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match_backtrack(t_data d): m_data(d) {}

    template<typename T>
    bool match_term(T* pattern, reference tground, t_substitution& subst) {
      if constexpr(std::is_same_v<t_data, t_match_data_empty>) {
        tt_term_full_match_backtrack_inner<tt_term_full, void, void> inner;
        return inner.template match_term(pattern, tground, subst);
      } else {
        tt_term_full_match_backtrack_inner<tt_term_full, t_data, void> inner(this->m_data);
        return inner.template match_term(pattern, tground, subst);
      }
    }

    template<typename T, typename t_rw>
    bool match_term(t_rw * rw, T* pattern, reference tground, t_substitution& subst, t_guard<t_rw, t_substitution> & guard) {
      if constexpr(std::is_same_v<t_data, t_match_data_empty>) {
        tt_term_full_match_backtrack_inner<tt_term_full, void, t_rw> inner(rw, guard);
        return inner.template match_term(pattern, tground, subst);
      } else {
        tt_term_full_match_backtrack_inner<tt_term_full, t_data, t_rw> inner(this->m_data, rw, guard);
        return inner.template match_term(pattern, tground, subst);
      }
    }

    template<typename termIt1, typename termIt2>
    bool match_term(termIt1 p_current, termIt1 p_end, termIt2 tg_current, termIt2 tg_end, t_substitution& subst) {
      if constexpr(std::is_same_v<t_data, t_match_data_empty>) {
        tt_term_full_match_backtrack_inner<tt_term_full, void, void> inner;
        return inner.template match_term(p_current, p_end, tg_current, tg_end, subst);
      } else {
        tt_term_full_match_backtrack_inner<tt_term_full, t_data, void> inner(this->m_data);
        return inner.template match_term(p_current, p_end, tg_current, tg_end, subst);
      }
    }

    template<typename termIt1, typename termIt2, typename t_rw>
    bool match_term(t_rw * rw, termIt1 p_current, termIt1 p_end, termIt2 tg_current, termIt2 tg_end, t_substitution& subst, t_guard<t_rw, t_substitution> & guard) {
      if constexpr(std::is_same_v<t_data, t_match_data_empty>) {
        tt_term_full_match_backtrack_inner<tt_term_full, void, t_rw> inner(rw, guard);
        return inner.template match_term(p_current, p_end, tg_current, tg_end, subst);
      } else {
        tt_term_full_match_backtrack_inner<tt_term_full, t_data, t_rw> inner(this->m_data, rw, guard);
        return inner.template match_term(p_current, p_end, tg_current, tg_end, subst);
      }
    }

  private:
    using t_content = std::conditional_t<std::is_same_v<t_data, t_match_data_empty>, t_match_data_empty, t_data>;
    t_content m_data;
  };





  ///////////////////////////////////////////////////////////////////////////////
  // COMBINER
  ///////////////////////////////////////////////////////////////////////////////

  template<typename tt_term_full, typename t_data=t_match_data_empty, typename=void> struct tt_term_full_match;

  template<typename tt_term_full, typename t_data> struct tt_term_full_match<tt_term_full, t_data, std::enable_if_t<
      tt_term_full::t_variable::t_spec::complexity == hrw::utils::parsing_complexity::ELEMENT, void>>:
      public tt_term_full_match_element<tt_term_full, t_data> {
    using t_parent = tt_term_full_match_element<tt_term_full, t_data>;

    template<typename T=t_data, std::enable_if_t<std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match(): t_parent() {}

    template<typename T=t_data, std::enable_if_t<!std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match(t_data d): t_parent(d) {}

  };

  template<typename tt_term_full, typename t_data> struct tt_term_full_match<tt_term_full, t_data, std::enable_if_t<
      tt_term_full::t_variable::t_spec::complexity == hrw::utils::parsing_complexity::SEQUENCE, void>>:
      public tt_term_full_match_sequence<tt_term_full, t_data> {
    using t_parent = tt_term_full_match_sequence<tt_term_full, t_data>;

    template<typename T=t_data, std::enable_if_t<std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match(): t_parent() {}

    template<typename T=t_data, std::enable_if_t<!std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match(t_data d): t_parent(d) {}
  };


  template<typename tt_term_full, typename t_data> struct tt_term_full_match<tt_term_full, t_data, std::enable_if_t<
      tt_term_full::t_variable::t_spec::complexity == hrw::utils::parsing_complexity::FULL, void>>:
      public tt_term_full_match_backtrack<tt_term_full, t_data> {
    using t_parent = tt_term_full_match_backtrack<tt_term_full, t_data>;

    template<typename T=t_data, std::enable_if_t<std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match(): t_parent() {}

    template<typename T=t_data, std::enable_if_t<!std::is_same_v<T, t_match_data_empty>, bool> = true>
    tt_term_full_match(t_data d): t_parent(d) {}
  };
}


#endif // __HREWRITE_HTERM_MATCH_H__

