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


#ifndef __HREWRITE_C_RW_H__
#define __HREWRITE_C_RW_H__

#include <vector>
#include <tuple>
#include <stack>

#include <iostream>
#include <functional>

#include "hrewrite/utils/variant.hpp"
#include "hrewrite/hterm_print.hpp"
#include "hrewrite/hterm.hpp"
#include "hrewrite/exceptions/common.hpp"
#include "hrewrite/exceptions/rewrite.hpp"

namespace hrw {


  /////////////////////////////////////////////////////////////////////////////
  // REWRITING STRATEGY
  /////////////////////////////////////////////////////////////////////////////

  enum e_strategy { STY_INNER, STY_OUTER };
  enum e_configuration { STORE, SWAP, NO_SWAP };
  template<bool swap, bool store, typename t_term_full_ref> struct t_configuration_make;
  template<typename t_term_full_ref> struct t_configuration_make<false, true, t_term_full_ref>  {
    static inline constexpr bool swap_v = false;
    static inline constexpr bool store_v = true;
    static inline constexpr e_configuration conf = e_configuration::STORE;
    using t_result = t_term_full_ref;
    static inline constexpr t_term_full_ref get_result(t_result r, t_term_full_ref) { return r; }
  };
  template<typename t_term_full_ref> struct t_configuration_make<true, false, t_term_full_ref>  {
    static inline constexpr bool swap_v = true;
    static inline constexpr bool store_v = false;
    static inline constexpr e_configuration conf = e_configuration::SWAP;
    using t_result = bool;
    static inline constexpr t_term_full_ref get_result(t_result, t_term_full_ref t) { return t; }
  };
  template<typename t_term_full_ref> struct t_configuration_make<false, false, t_term_full_ref> {
    static inline constexpr bool swap_v = false;
    static inline constexpr bool store_v = false;
    static inline constexpr e_configuration conf = e_configuration::NO_SWAP;
    using t_result = std::pair<bool, t_term_full_ref>;
    static inline constexpr t_term_full_ref get_result(t_result r, t_term_full_ref) { return r.second; }
  };


  template<typename type, bool has_limit, e_strategy stg=e_strategy::STY_INNER>
  class helper_rewrite_inner {
  public:
    using t_term_full_ref = typename type::t_term_full_ref;
    using t_rw_result = typename type::t_rw_result;
    using t_configuration = typename type::t_configuration;


    helper_rewrite_inner(type& ctx, t_term_full_ref t): m_ctx(ctx), m_t(t) {}

    template<typename t_term>
    // t_term_full_ref operator()([[maybe_unused]] t_term t) {
    t_rw_result operator()([[maybe_unused]] t_term t) {
      if constexpr(has_container_v<t_term>) {
        if constexpr(t_configuration::conf == e_configuration::STORE) {
          typename t_term::t_container c;
          for(t_term_full_ref s: t) {
            if constexpr(stg == e_strategy::STY_INNER) {
              c.push_back(this->m_ctx.template rewrite_by_value<has_limit>(s));
            } else {
              c.push_back(this->m_ctx.template rewrite_by_need<has_limit, e_rw_status::FULL>(s));
            }
          }
          return this->m_ctx.m_ctx_term.template create_sterm_from_diff<t_term>(t, std::move(c));
        } else if constexpr(t_configuration::conf == e_configuration::SWAP) {
          bool res = false;
          for(t_term_full_ref s: t) {
            if constexpr(stg == e_strategy::STY_INNER) {
              res = this->m_ctx.template rewrite_by_value<has_limit>(s) || res ;
            } else {
              res = this->m_ctx.template rewrite_by_need<has_limit, e_rw_status::FULL>(s) || res;
            }
          }
          return res;
        } else {
          typename t_term::t_container c;
          bool res = false;
          for(t_term_full_ref s: t) {
            if constexpr(stg == e_strategy::STY_INNER) {
              auto tmp = this->m_ctx.template rewrite_by_value<has_limit>(s);
              res = res || tmp.first;
              c.push_back(tmp.second);
            } else {
              auto tmp = this->m_ctx.template rewrite_by_need<has_limit, e_rw_status::FULL>(s);
              res = res || tmp.first;
              c.push_back(tmp.second);                
            }
          }
          return std::make_pair(res, this->m_ctx.m_ctx_term.template create_sterm_from_diff<t_term>(t, std::move(c)));
        }
      } else {
        if constexpr(t_configuration::conf == e_configuration::STORE) {
          return this->m_t;
        } else if constexpr(t_configuration::conf == e_configuration::SWAP) {
          return false;
        } else {
          return std::make_pair(false, this->m_t);
        }
      }
    }
  private:
    type& m_ctx;
    t_term_full_ref m_t;
  };




  template<typename type, bool has_limit>
  struct tt_match_by_need_data {
    using t_term_full_ref = typename type::t_term_full_ref;
    using t_rw_result = typename type::t_rw_result;
    using t_configuration = typename type::t_configuration;

    tt_match_by_need_data(type& t): m_content(t) {}

    t_term_full_ref manage(t_term_full_ref t) {
      // std::cout << "manage(...)" << std::endl;
      t_rw_result res = this->m_content.template rewrite_by_need<has_limit, e_rw_status::SHALLOW>(t);
      return t_configuration::get_result(res, t);
    }

    type& m_content;
  };


  /////////////////////////////////////////////////////////////////////////////
  // REWRITING CONTEXT
  /////////////////////////////////////////////////////////////////////////////

  template<typename targ_ctx_term, template<typename ... Args> typename targ_map> // targ_map is map-like
  class context_rw {
  public:
    using type = context_rw<targ_ctx_term, targ_map>;
    using t_ctx_term = targ_ctx_term;


    using t_term_full = typename t_ctx_term::t_term_full;
    using t_term_full_ref = typename t_ctx_term::t_term_full_ref;
    using t_term_full_ref_hash = typename t_term_full::pointer_hash;
    using t_substitution = typename t_ctx_term::t_substitution;

    using t_match_by_value = typename t_term_full::t_match;
    template<bool has_limit> using t_match_by_need  = tt_term_full_match<t_term_full,  tt_match_by_need_data<type, has_limit>>;

    using t_configuration = t_configuration_make<
      !t_ctx_term::term_const_v,
      t_ctx_term::ensure_unique_v,
      t_term_full_ref
    >;



    context_rw(t_ctx_term& ctx): m_ctx_term(ctx), m_substitution() {};

    t_ctx_term& get_ctx_term() { return this->m_ctx_term; }

    // using t_print = t_hterm_print<typename targ_ctx_term::ctx_theory, typename context_rw<targ_ctx_term, targ_map>::t_term_full>;
    // t_print ctx_print;

 
    //////////////////////////////////////////
    // rewriting rule management

    using t_guard = std::function<bool(type *, t_substitution *)>;

    template<bool strict_sorting=false>
    void add(t_term_full_ref pattern, t_term_full_ref image);
    template<bool strict_sorting=false>
    void add(t_term_full_ref pattern, t_term_full_ref image, t_guard guard);
    void add(type const & ctx_rw);
    void clear();
    void clear_nf();


    //////////////////////////////////////////
    // rewriting

    template<e_strategy stg=e_strategy::STY_INNER>
    t_term_full_ref rewrite(t_term_full_ref t, unsigned int nb_steps);
    template<e_strategy stg=e_strategy::STY_INNER>
    t_term_full_ref rewrite(t_term_full_ref t);
    unsigned int get_rw_count() const;


    //////////////////////////////////////////
    // print

    friend std::ostream& operator<<(std::ostream& os, const type& reng) {
      using t_print = t_hterm_print<typename t_ctx_term::ctx_theory, t_term_full>;
      t_print ctx_print;
      for(std::size_t i = 0; i < t_term_full::nb_alternative - 1; ++i) {
        int j = 0;
        for(auto itc: reng.m_rules[i]) {
          for(auto itr: itc) {
            os << "rule (" << j << "): " << ctx_print.print(std::get<0>(itr)) << " -> " << ctx_print.print(std::get<1>(itr)) << "\n"; 
          }
          ++j;
        }
      }
      return os;
    }


  private:
    t_ctx_term& m_ctx_term;
    t_substitution m_substitution;

    template<typename type, bool has_limit, e_strategy stg>
    friend class helper_rewrite_inner;

    template<typename tt_term_full, typename t_data, typename>
    friend struct tt_term_full_match;

    template<typename type, bool has_limit>
    friend struct  tt_match_by_need_data;


    //////////////////////////
    // rule repo
    static constexpr std::size_t nb_alternative = t_term_full::nb_alternative - 1;
    using t_rule = std::tuple<t_term_full_ref, t_term_full_ref, t_guard>;
    using t_rules = std::vector<std::vector<t_rule>>;
    t_rules m_rules[nb_alternative];

    //////////////////////////
    // rule application repo
    template<bool store, typename=void> struct t_registry_make { struct type {}; };
    template<bool store> struct t_registry_make<store, std::enable_if_t<store, void>> { using type = targ_map<t_term_full_ref, t_term_full_ref, t_term_full_ref_hash>; };
    using t_registry = typename t_registry_make<t_configuration::store_v>::type;
    t_registry m_applications;


    //////////////////////////
    // rewriting helpers
    unsigned int m_rw_count;
    unsigned int m_rw_count_max;

    using t_rw_result = typename t_configuration::t_result;


    // t_term_full_ref rewrite_single(t_term_full_ref t);  
    template<bool has_limit, e_strategy stg>
    t_rw_result rewrite_single(t_term_full_ref t);    

    template<bool has_limit>
    t_rw_result rewrite_by_value(t_term_full_ref t);

    template<bool has_limit, e_rw_status goal>
    t_rw_result rewrite_by_need(t_term_full_ref t);

    static inline void check_structured(t_term_full_ref t) {
      if(not t->is_structured()) {
        throw hrw::exception::rw_gterm();
      }
    }

  };

  /////////////////////////////////////////////////////////////////////////////
  // IMPLEMENTATION
  /////////////////////////////////////////////////////////////////////////////


  //////////////////////////////////////////
  // rewriting rule management

  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  template<bool strict_sorting>
  void context_rw<targ_ctx_term, targ_map>::add(t_term_full_ref pattern, t_term_full_ref image) {
    this->add<strict_sorting>(pattern, image, t_guard(nullptr));
  }

  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  template<bool strict_sorting>
  void context_rw<targ_ctx_term, targ_map>::add(t_term_full_ref pattern, t_term_full_ref image, t_guard guard) {
    if(pattern->is_structured()) {
      if constexpr(strict_sorting) {
        if(!this->m_ctx_term.is_instance_of(image, pattern->get_sort())) {
          using t_exception = hrw::exception::rw_rule<t_term_full_ref>;
          throw t_exception(pattern, image);
        }
      }
      std::size_t idx = pattern->index() - 1;
      auto& rule_reg = this->m_rules[idx];
      t_constructor_id constructor = pattern->get_constructor();
      if(rule_reg.size() <= constructor) {
        rule_reg.resize(constructor+1);
      }
      rule_reg[constructor].emplace_back(std::make_tuple(pattern, image, guard));
    } else {
      throw hrw::exception::rw_pattern();
    }
  }

  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  void context_rw<targ_ctx_term, targ_map>::add(context_rw<targ_ctx_term, targ_map> const & ctx_rw) {
    using t_i = typename t_rules::size_type;

    if(&(this->m_ctx_term) != &(ctx_rw.m_ctx_term)) {
      throw hrw::exception::generic("ERROR: can only update a rewriting context with another one using the same term context");
    }

    for(std::size_t idx = 0; idx < type::nb_alternative; ++idx) {
      if(this->m_rules[idx].size() < ctx_rw.m_rules[idx].size()) {
        this->m_rules[idx].resize(ctx_rw.m_rules[idx].size());
      }
      for(t_i i = 0; i < ctx_rw.m_rules[idx].size(); ++i) {
        this->m_rules[idx][i].reserve(this->m_rules[idx][i].size() + ctx_rw.m_rules[idx][i].size());
        this->m_rules[idx][i].insert(this->m_rules[idx][i].end(), ctx_rw.m_rules[idx][i].begin(), ctx_rw.m_rules[idx][i].end());
      }
    }
  }


  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  void context_rw<targ_ctx_term, targ_map>::clear() {
    for(std::size_t i = 0; i < t_term_full::nb_alternative - 1; ++i) {
      this->m_rules[i].clear(); // clear all the rules
    }
    this->clear_nf();
  }

  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  void context_rw<targ_ctx_term, targ_map>::clear_nf() {
    this->m_applications.clear(); // clear all the recorded rule application
  }

  //////////////////////////////////////////
  // rewriting

  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  template<e_strategy stg>
  typename context_rw<targ_ctx_term, targ_map>::t_term_full_ref context_rw<targ_ctx_term, targ_map>::rewrite(t_term_full_ref t) {
    this->m_rw_count = 0;
    static constexpr bool has_limit = false;

    // type::check_structured(t);
    // if(not t->is_structured()) { return t; }

    t_rw_result res = [&]() {
      if constexpr(stg == e_strategy::STY_INNER) {
        return this->template rewrite_by_value<has_limit>(t);
      } else {
        return this->template rewrite_by_need<has_limit, e_rw_status::FULL>(t);
      }
    }();

    return t_configuration::get_result(res, t);
  }

  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  template<e_strategy stg>
  typename context_rw<targ_ctx_term, targ_map>::t_term_full_ref context_rw<targ_ctx_term, targ_map>::rewrite(t_term_full_ref t, unsigned int max_rw_cout) {
    this->m_rw_count = 0;
    this->m_rw_count_max = max_rw_cout;
    static constexpr bool has_limit = true;

    // type::check_structured(t);
    // if(not t->is_structured()) { return t; }

    t_rw_result res = [&]() {
      if constexpr(stg == e_strategy::STY_INNER) {
        return this->template rewrite_by_value<has_limit>(t);
      } else {
        return this->template rewrite_by_need<has_limit, e_rw_status::FULL>(t);
      }
    }();

    return t_configuration::get_result(res, t);
  }

  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  unsigned int context_rw<targ_ctx_term, targ_map>::get_rw_count() const {
    return this->m_rw_count;
  }


  //////////////////////////////////////////
  // rewriting implementation

  // TODO: add template for matching class
  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  template<bool has_limit, e_strategy stg>
  typename context_rw<targ_ctx_term, targ_map>::t_rw_result context_rw<targ_ctx_term, targ_map>::rewrite_single(t_term_full_ref t) {
    // using t_print = t_hterm_print<typename t_ctx_term::ctx_theory, t_term_full>;
    // t_print ctx_print;
    // std::cout << "rewrite_single(" << ctx_print.print(t) << ")" << std::endl;
    if(not t->is_structured()) {
      bool res = false;
      if constexpr(t_configuration::conf == e_configuration::STORE) {
        return t;
      } else if constexpr(t_configuration::conf == e_configuration::SWAP) {
        return res;
      } else {
        return std::make_pair(false, t);
      }
    }

    auto match = [&]() {
      if constexpr(stg == e_strategy::STY_INNER) {
        return t_match_by_value();
      } else {
        using t_match = t_match_by_need<has_limit>;
        using t_match_data =  tt_match_by_need_data<type, has_limit>;
        return t_match(t_match_data(*this));
      }
    }();

    // std::cout << "  rewrite_single[" << reinterpret_cast<std::size_t>(t_term_full::make_ptr_struct::to_ptr(t)) << "] : " << ctx_print.print(t) << std::endl;

    std::size_t idx = t->index() - 1;
    t_constructor_id constructor = t->get_constructor();
    if(this->m_rules[idx].size() > constructor) { // there are rewriting rules!
      for(auto& rule: this->m_rules[idx][constructor]) {
        // std::cout << "   context_rw::rewrite_inner::matching: "
        //    <<  ctx_print.print(t)
        //    << "\n     vs " <<  ctx_print.print(std::get<0>(rule))
        //    << " -> " << ctx_print.print(std::get<1>(rule)) << std::endl;
        this->m_substitution.clear(); // should be optional
        bool matched;
        t_guard & guard = std::get<2>(rule);
        if(guard) {
          matched = match.match_term(this, t_term_full::make_ptr_struct::to_ptr(std::get<0>(rule)), t, this->m_substitution, guard);
        } else {
          matched = match.match_term(t_term_full::make_ptr_struct::to_ptr(std::get<0>(rule)), t, this->m_substitution);
        }
        // std:: cout << "   matches: " << std::boolalpha << matched << std::endl;
        if(matched) {
          // std::cout << "   => match: substitution = " << ctx_print.print(this->m_substitution) << std::endl;
          t_term_full_ref t_new = this->m_ctx_term.instantiate(std::get<1>(rule), this->m_substitution);
          ++this->m_rw_count;
          // std::cout << "   => " << ctx_print.print(t_new) << std::endl;

          if constexpr(t_configuration::conf == e_configuration::STORE) {
            return t_new;
          } else if constexpr(t_configuration::conf == e_configuration::SWAP) {
            // using t_term_eq_ref = typename t_term_full::t_eq_ref<true>;
            // bool res = t_term_eq_ref()(t, t_new);
            bool res = true;
            *t = *t_new;
            return res;
          } else {
            // using t_term_eq_ref = typename t_term_full::t_eq_ref<true>;
            // bool res = t_term_eq_ref()(t, t_new);
            bool res = true;
            return std::make_pair(res, t_new);
          }
        }
      }
    }

    // no rewriting occured
    if constexpr(t_configuration::conf == e_configuration::STORE) {
      return t;
    } else if constexpr(t_configuration::conf == e_configuration::SWAP) {
      return false;
    } else {
      return std::make_pair(false, t);
    }

  }


  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  template<bool has_limit>
  typename context_rw<targ_ctx_term, targ_map>::t_rw_result context_rw<targ_ctx_term, targ_map>::rewrite_by_value(t_term_full_ref t) {
    using t_rewrite_inner = helper_rewrite_inner<type, has_limit, e_strategy::STY_INNER>;

    if constexpr(t_configuration::conf == e_configuration::STORE) {

      std::vector<t_term_full_ref> previous_versions;

      while(true) {
        // std::cout << "  rewriting term: " << this->ctx_print.print(t) << std::endl;
        auto it = this->m_applications.find(t);
        if(it == this->m_applications.end()) {
          // 1. rewrite subterms
          t_rewrite_inner obj(*this, t);
          t_term_full_ref t_new = t->visit(obj);
          t = t_new;
          // std::cout << "    after subterms: " << ctx_print.print(t) << std::endl;
          // 2. rewrite the term itself
          t_new = this->template rewrite_single<has_limit, e_strategy::STY_INNER>(t);
          // 3. check if irreductible
          if(t_new != t) { // change happened
            previous_versions.push_back(t);
            t = t_new;
          } else { // we have a irreductible term => register
            // std::cout << "    => irreductible! " << std::endl;
            for(t_term_full_ref prev: previous_versions) {
              this->m_applications.insert(std::make_pair(prev, t));
            }
            this->m_applications.insert(std::make_pair(t, t));
            return t;
          }
        } else { // we found the image of t -> need to associate the prevs to this image
          t_term_full_ref irr = it->second;
          // std::cout << "    already rewritten: " << ctx_print.print(irr) << std::endl;
          for(t_term_full_ref prev: previous_versions) {
            this->m_applications.insert(std::make_pair(prev, irr));
          }
          return irr;
        }
      } // end of while(true)

    } if constexpr(t_configuration::conf == e_configuration::SWAP) {

      bool main_changed = false;
      while(true) {
        // std::cout << "  rewrite_by_value[" << reinterpret_cast<std::size_t>(t_term_full::make_ptr_struct::to_ptr(t)) << "] : " << this->ctx_print.print(t) << std::endl;
        if(t->get_annex_data().status != e_rw_status::FULL) {
          // std::cout << "    => reductible" << std::endl;
          // 1. rewrite subterms
          t_rewrite_inner obj(*this, t);
          // t_term_full_ref t_new = t_term_full::template visit<t_term_full_ref, type::helper_rewrite_inner>(obj, t);
          bool changed = t->visit(obj);
          // std::cout << "    changed 1 = " << std::boolalpha << changed << std::endl;
          // 2. rewrite the term itself
          changed = this->template rewrite_single<has_limit, e_strategy::STY_INNER>(t) || changed; // || is not commutative in C, due to side effects
          main_changed = main_changed || changed;
          // std::cout << "    changed 2 = " << std::boolalpha << changed << std::endl;
          // 3. check if irreductible
          if(!changed) { // change happened
            // std::cout << "    term[" << reinterpret_cast<std::size_t>(t_term_full::make_ptr_struct::to_ptr(t)) << "] now irreductible" << std::endl;
            t->get_annex_data().status = e_rw_status::FULL;
            return main_changed;
          }
        } else {
          // std::cout << "    => irrreductible" << std::endl;
          return main_changed;
        }
      } // end of while(true)
    
    } else {
      static_assert((t_configuration::conf == e_configuration::STORE) || (t_configuration::conf == e_configuration::SWAP));
      throw hrw::exception::unimplemented();
    }

  }



  template<typename targ_ctx_term, template<typename ... Args> typename targ_map>
  template<bool has_limit, e_rw_status goal>
  typename context_rw<targ_ctx_term, targ_map>::t_rw_result context_rw<targ_ctx_term, targ_map>::rewrite_by_need(t_term_full_ref t) {
    using t_rewrite_inner = helper_rewrite_inner<type, has_limit, e_strategy::STY_OUTER>;

    // std::cout << "rewrite_by_need[" << reinterpret_cast<std::size_t>(t_term_full::make_ptr_struct::to_ptr(t)) << "] : " << this->ctx_print.print(t) << std::endl;
    // std::cout << " => begin: " << t->get_annex_data().status << std::endl;

    if constexpr(t_configuration::conf == e_configuration::STORE) {
      throw hrw::exception::unimplemented();

    } if constexpr(t_configuration::conf == e_configuration::SWAP) {
      if(goal <= t->get_annex_data().status) {
        // std::cout << " => end: " << t->get_annex_data().status << std::endl;
        return false;
      } else {
        bool main_changed = false;
        if constexpr (e_rw_status::SHALLOW <= goal) {
          // rewrite root
          bool changed;
          while(t->get_annex_data().status < e_rw_status::SHALLOW) {
            changed = this->template rewrite_single<has_limit, e_strategy::STY_OUTER>(t);
            // std::cout << "result of rewrite_single: " << std::boolalpha << changed << std::endl;
            main_changed = main_changed || changed;
            if(!changed) { // change happened
              t->get_annex_data().status = std::max(t->get_annex_data().status, e_rw_status::SHALLOW);
              // std::cout << "    term[" << reinterpret_cast<std::size_t>(t_term_full::make_ptr_struct::to_ptr(t)) << "] now irreductible: " << static_cast<int>(t->get_annex_data().status) << std::endl;
            }
          }

          if constexpr (e_rw_status::FULL <= goal) {
            t_rewrite_inner obj(*this, t);
            // t_term_full_ref t_new = t_term_full::template visit<t_term_full_ref, type::helper_rewrite_inner>(obj, t);
            changed = VISIT_SINGLE(obj, t->m_content);
            main_changed = main_changed || changed;
            t->get_annex_data().status = e_rw_status::FULL;
          }
        }
        // std::cout << " => end: " << t->get_annex_data().status << std::endl;
        return main_changed;
      }

    } else {
      throw hrw::exception::unimplemented();
    }


  }


}


#endif // __HREWRITE_C_RW_H__

