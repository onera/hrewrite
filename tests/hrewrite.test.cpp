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

#include "tests/common/activation.hpp"
#if ENABLE_HREWRITE

#include "doctest/doctest.h"

#include "hrewrite/utils.hpp"
#include "hrewrite/parsing.hpp"
#include "hrewrite/hterm.hpp"
#include "hrewrite/hrewrite.hpp"
#include "hrewrite/theory/theory_variable.hpp"
#include "hrewrite/theory/theory_free.hpp"
#include "hrewrite/theory/theory_literal.hpp"
#include "hrewrite/theory/theory_leaf.hpp"

#include "hrewrite/utils/type_traits.hpp"

#include "tests/hrewrite.test.hpp"

using namespace hrw;

#include <unordered_map>
#include <iostream>
#include <string>


////////////////////////////////////////////////////////////////////////////////
//// HREWRITE FULL SETUP
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////
// 1. list of term registry

template<typename ... Args> using unordered_set_wrapper = std::unordered_set<Args...>;

using t_term_registry_list = term_registry_list<
  hrw::utils::registry_unique<unordered_set_wrapper, true, false>
  // hrw::utils::registry_unique<unordered_set_wrapper, true, true>
  // hrw::utils::registry_shared
>;


//////////////////////////////////////////
// 2. list of map for term refs

template<typename ... Args> using my_umap = std::unordered_map<Args...>;

using t_map_list = map_list<my_umap>;
// using t_map_list = map_list<my_umap, my_rmap>;


//////////////////////////////////////////
// 3. list of sort context natset

using t_sort_context_param_list = sort_context_param_list<
  hrw::utils::natset
>;


//////////////////////////////////////////
// 4. parsers

template<typename t_alphabet, const t_alphabet& alphabet>
using my_automata = utils::tt_automata<hrw::utils::natset, hrw::utils::natset>::template type<t_alphabet, alphabet>;

template<typename targ_alphabet, const targ_alphabet& alphabet>
using p_combine = utils::combine_variant<targ_alphabet, alphabet, utils::element, my_automata>;

// using t_parser_list = parser_list<utils::sequence>;
using t_parser_list = parser_list<p_combine>;
// using t_parser_list = parser_list<utils::element, utils::sequence, my_automata, p_combine>;


//////////////////////////////////////////
// 5. list of variable implementations

using t_variable_theory_list = variable_theory_list<hrw::theory::tp_theory_variable_vector>;
// using t_variable_theory_list = variable_theory_list<hrw::theory::tp_theory_variable_map, hrw::theory::tp_theory_variable_vector>;


//////////////////////////////////////////
// 6. list of structured theory lists

template<typename t_alphabet, const t_alphabet& alphabet>
using t_sparser = utils::combine_variant<t_alphabet, alphabet, utils::sequence, my_automata>;

// using t_structured_theory_list_list = std::tuple<
//   structured_theory_list<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type>,
//   structured_theory_list<hrw::theory::tp_theory_literal<int>::template type>
// >;

using t_structured_theory_list_list = std::tuple<
  // structured_theory_list<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type>,
  // structured_theory_list<hrw::theory::tp_theory_leaf::template type>,
  // structured_theory_list<hrw::theory::tp_theory_literal<int>::template type>,
  // structured_theory_list<
  //   hrw::theory::tp_theory_free<std::vector, t_sparser>::template type,
  //   hrw::theory::tp_theory_literal<int>::template type
  // >
  structured_theory_list<
    hrw::theory::tp_theory_free<std::vector, t_sparser>::template type,
    hrw::theory::tp_theory_literal<int>::template type,
    hrw::theory::tp_theory_literal<double>::template type,
    hrw::theory::tp_theory_leaf::template type
  >
>;


////////////////////////////////////////////////////////////////////////////////
//// GENERIC TESTING SETUP
////////////////////////////////////////////////////////////////////////////////

template<typename config, typename can_run=void> struct test_all;

template<typename config>
struct test_all<config, std::enable_if_t<
    !is_valid_sth_v<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>
    || !is_valid_sth_v<hrw::theory::tp_theory_literal<int>::template type, config>
    || !is_valid_sth_v<hrw::theory::tp_theory_literal<double>::template type, config>
    || !is_valid_sth_v<hrw::theory::tp_theory_leaf::template type, config>
  , void>> { void run() {} };


template<typename config>
struct test_all<config, std::enable_if_t<
    is_valid_sth_v<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>
    && is_valid_sth_v<hrw::theory::tp_theory_literal<int>::template type, config>
    && is_valid_sth_v<hrw::theory::tp_theory_literal<double>::template type, config>
    && is_valid_sth_v<hrw::theory::tp_theory_leaf::template type, config>
  , void>> {
  // 1. types
  using t_hrewrite = get_hrewrite<config>;

  using ctx_th = typename t_hrewrite::ctx_th;
  using t_ctx_tm = typename t_hrewrite::t_ctx_tm;

  using t_term_full = typename t_hrewrite::t_term_full;
  using t_term_full_ref = typename t_hrewrite::t_term_full_ref;
  using t_variable = typename t_hrewrite::t_variable;
  using t_substitution = typename t_hrewrite::t_substitution;

  using t_ctx_rw = typename t_hrewrite::t_ctx_rw;

  using t_print = typename t_hrewrite::t_print;

  using t_theory_free = get_stheory_t<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>;
  using t_theory_lit_int = get_stheory_t<hrw::theory::tp_theory_literal<int>::template type, config>;
  using t_theory_lit_double = get_stheory_t<hrw::theory::tp_theory_literal<double>::template type, config>;
  using t_theory_leaf = get_stheory_t<hrw::theory::tp_theory_leaf::template type, config>;

  using t_container = typename t_theory_free::template tt_term<t_term_full>::t_container;


  // 2. fields
  t_sort_id sort_int   ;
  t_sort_id sort_double;
  t_sort_id sort_string;

  t_constructor_core<t_theory_leaf>       c_zero        ;
  t_constructor_core<t_theory_free>       c_succ        ;
  t_constructor_core<t_theory_free>       c_plus        ;
  t_constructor_core<t_theory_free>       c_sum         ;
  t_constructor_core<t_theory_lit_int>    c_value_int   ;
  t_constructor_core<t_theory_lit_double> c_value_double;
  t_constructor_core<t_theory_free>       c_print       ;

  // 3. constructor
  test_all():
    sort_int       (ctx_th::add_sort("int")),
    sort_double    (ctx_th::add_sort("double")),
    sort_string    (ctx_th::add_sort("string")),
    c_zero         (ctx_th::template add_constructor<t_theory_leaf>(sort_int, "zero")),
    c_succ         (ctx_th::template add_constructor<t_theory_free>(sort_int, "succ", "int")),
    c_plus         (ctx_th::template add_constructor<t_theory_free>(sort_int, "plus", "int int")),
    c_sum          (ctx_th::template add_constructor<t_theory_free>(sort_int, "sum", "int*")),
    c_value_int    (ctx_th::template add_constructor<t_theory_lit_int>(sort_int, "int")),
    c_value_double (ctx_th::template add_constructor<t_theory_lit_double>(sort_double, "double")),
    c_print        (ctx_th::template add_constructor<t_theory_free>(sort_string, "print", "int")) {}


  // 4. test functions
  void test_fields() {
    std::cout << "= hrewrite - sort/constructor\n";

    // std::cout << hrw::utils::type_name<typename ctx_th::t_spec_sequence>() << std::endl;

    CHECK(sort_int != sort_double);
    CHECK(sort_int != sort_string);

    CHECK(ctx_th::contains_sort(sort_int));
    CHECK(ctx_th::contains_sort(sort_double));
    CHECK(ctx_th::contains_sort(sort_string));


    CHECK(c_zero.id() != c_print.id());
    CHECK(c_succ.id() != c_plus .id());
    CHECK(c_succ.id() != c_sum  .id());

    bool hs_zero = ctx_th::contains_constructor(c_zero);
    bool hs_succ = ctx_th::contains_constructor(c_succ);

    CHECK(hs_zero);
    CHECK(hs_succ);

    CHECK(ctx_th::contains_constructor(c_zero        ));
    CHECK(ctx_th::contains_constructor(c_succ        ));
    CHECK(ctx_th::contains_constructor(c_plus        ));
    CHECK(ctx_th::contains_constructor(c_sum         ));
    CHECK(ctx_th::contains_constructor(c_value_int   ));
    CHECK(ctx_th::contains_constructor(c_value_double));
    CHECK(ctx_th::contains_constructor(c_print       ));

    CHECK(ctx_th::get_sort(c_zero        ) == sort_int);
    CHECK(ctx_th::get_sort(c_succ        ) == sort_int);
    CHECK(ctx_th::get_sort(c_plus        ) == sort_int);
    CHECK(ctx_th::get_sort(c_sum         ) == sort_int);
    CHECK(ctx_th::get_sort(c_value_int   ) == sort_int);
    CHECK(ctx_th::get_sort(c_value_double) == sort_double);
    CHECK(ctx_th::get_sort(c_print       ) == sort_string);

    CHECK(ctx_th::get_name(c_zero        ) == "zero");
    CHECK(ctx_th::get_name(c_succ        ) == "succ");
    CHECK(ctx_th::get_name(c_plus        ) == "plus");
    CHECK(ctx_th::get_name(c_sum         ) == "sum");
    CHECK(ctx_th::get_name(c_value_int   ) == "int");
    CHECK(ctx_th::get_name(c_value_double) == "double");
    CHECK(ctx_th::get_name(c_print       ) == "print");
  }

  void test_term_creation() {
    std::cout << "= hrewrite - term creation\n";
    t_ctx_tm ctx_tm;
    t_print ctx_print;

    t_term_full_ref zero  = ctx_tm.create_sterm_no_check(c_zero);
    if(zero->m_content.valueless_by_exception()) { std::cout << "!! zero\n"; } else { std::cout << " ok zero\n"; }
    t_term_full_ref one   = ctx_tm.create_sterm_no_check(c_succ, t_container({zero}));
    if(one->m_content.valueless_by_exception()) { std::cout << "!! one\n"; } else { std::cout << " ok one\n"; }
    t_term_full_ref two   = ctx_tm.create_sterm_no_check(c_succ, t_container({one}));
    if(two->m_content.valueless_by_exception()) { std::cout << "!! two\n"; } else { std::cout << " ok two\n"; }
    t_term_full_ref three = ctx_tm.create_sterm_no_check(c_succ, t_container({two}));
    if(three->m_content.valueless_by_exception()) { std::cout << "!! three\n"; } else { std::cout << " ok three\n"; }
    t_term_full_ref plus  = ctx_tm.create_sterm_no_check(c_plus, t_container({two, three}));
    if(plus->m_content.valueless_by_exception()) { std::cout << "!! plus\n"; } else { std::cout << " ok plus\n"; }
    t_term_full_ref sum   = ctx_tm.create_sterm_no_check(c_sum , t_container({plus, two, three}));
    if(sum->m_content.valueless_by_exception()) { std::cout << "!! sum\n"; } else { std::cout << " ok sum\n"; }

    t_term_full_ref print = ctx_tm.create_sterm_no_check(c_print, t_container({zero}));
    if(print->m_content.valueless_by_exception()) { std::cout << "!! print\n"; } else { std::cout << " ok print\n"; }

    t_term_full_ref huge  = ctx_tm.create_sterm_no_check(c_value_int, 9001);
    if(huge->m_content.valueless_by_exception()) { std::cout << "!! huge\n"; } else { std::cout << " ok huge\n"; }
    t_term_full_ref huged = ctx_tm.create_sterm_no_check(c_value_double, 9001.0);
    if(huged->m_content.valueless_by_exception()) { std::cout << "!! huged\n"; } else { std::cout << " ok huged\n"; }

    // print
    std::cout << "zero  = \"" << ctx_print.print(zero ) << "\"\n"; 
    std::cout << "one   = \"" << ctx_print.print(one  ) << "\"\n"; 
    std::cout << "two   = \"" << ctx_print.print(two  ) << "\"\n"; 
    std::cout << "three = \"" << ctx_print.print(three) << "\"\n"; 
    std::cout << "plus  = \"" << ctx_print.print(plus ) << "\"\n"; 
    std::cout << "sum   = \"" << ctx_print.print(sum  ) << "\"\n"; 
    std::cout << "print = \"" << ctx_print.print(print) << "\"\n"; 
    std::cout << "huge  = \"" << ctx_print.print(huge ) << "\"\n"; 
    std::cout << "huged = \"" << ctx_print.print(huged) << "\"\n"; 

    // unicity
    if constexpr(t_ctx_tm::ensure_unique_v) {
      t_term_full_ref tmp_zero  = ctx_tm.create_sterm(c_zero);
      t_term_full_ref tmp_one   = ctx_tm.create_sterm(c_succ, t_container({zero}));
      t_term_full_ref tmp_two   = ctx_tm.create_sterm(c_succ, t_container({one}));
      t_term_full_ref tmp_three = ctx_tm.create_sterm(c_succ, t_container({two}));
      t_term_full_ref tmp_plus  = ctx_tm.create_sterm(c_plus, t_container({two, three}));
      t_term_full_ref tmp_sum   = ctx_tm.create_sterm(c_sum , t_container({plus, two, three}));

      t_term_full_ref tmp_print = ctx_tm.create_sterm(c_print, t_container({zero}));

      t_term_full_ref tmp_huge  = ctx_tm.create_sterm(c_value_int, 9001);
      t_term_full_ref tmp_huged = ctx_tm.create_sterm(c_value_double, 9001.0);

      CHECK(zero  == tmp_zero );
      CHECK(one   == tmp_one  );
      CHECK(two   == tmp_two  );
      CHECK(three == tmp_three);
      CHECK(plus  == tmp_plus );
      CHECK(sum   == tmp_sum  );
      CHECK(print == tmp_print);
      CHECK(huge  == tmp_huge );
      CHECK(huged == tmp_huged);
    }
  }

  void test_manipulation() {
    std::cout << "= hrewrite - term manipulation\n";
    t_ctx_tm ctx_tm;
    t_print ctx_print;

    t_term_full_ref zero  (ctx_tm.create_sterm(c_zero));
    t_term_full_ref one   (ctx_tm.create_sterm(c_succ, t_container({zero})));
    t_term_full_ref two   (ctx_tm.create_sterm(c_succ, t_container({one})));
    t_term_full_ref three (ctx_tm.create_sterm(c_succ, t_container({two})));
    t_term_full_ref plus  (ctx_tm.create_sterm(c_plus, t_container({two, three})));
    t_term_full_ref sum   (ctx_tm.create_sterm(c_sum , t_container({plus, two, three})));

    // t_term_full_ref print = ctx_tm.create_sterm(c_print, t_container({zero}));

    // t_term_full_ref huge  = ctx_tm.create_sterm(c_value_int, 9001);
    // t_term_full_ref huged = ctx_tm.create_sterm(c_value_double, 9001.0);


    t_term_full_ref alpha (ctx_tm.create_vterm("int"));
    t_term_full_ref beta  (ctx_tm.create_vterm("int"));
    t_term_full_ref gamma ([&]() {
      if constexpr(get_p_complexity_v<config> == hrw::utils::parsing_complexity::FULL) {
        return ctx_tm.create_vterm("int*");
      } else {
        return ctx_tm.create_vterm("int");
      }
    }());


    const t_variable* pv_alpha = &(std::get<t_variable>(alpha->m_content));
    const t_variable* pv_beta  = &(std::get<t_variable>(beta->m_content));
    const t_variable* pv_gamma = &(std::get<t_variable>(gamma->m_content));

    std::cout << "ALPHA identity: " << (void*)pv_alpha << std::endl;
    std::cout << "BETA  identity: " << (void*)pv_beta  << std::endl;
    std::cout << "GAMMA identity: " << (void*)pv_gamma << std::endl;

    t_term_full_ref p_sum   (ctx_tm.create_sterm(c_sum,  t_container({alpha, beta, gamma})));
    t_term_full_ref i_sum_1 (ctx_tm.create_sterm(c_plus, t_container({alpha, beta})));
    t_term_full_ref i_sum   (ctx_tm.create_sterm(c_sum,  t_container({i_sum_1, gamma})));


    std::optional<t_substitution> subst = p_sum->match(sum);

    REQUIRE(subst.has_value());

    t_substitution & s = subst.value();
    std::cout << ctx_print.print(s) << "\n";
    CHECK(s.contains(pv_alpha));
    CHECK(s.contains(pv_beta));
    CHECK(s.contains(pv_gamma));
    bool good = true;
    if(!s.contains(pv_alpha)) {
      std::cout << "no image for alpha!" << std::endl;
      good = false;
    }
    if(!s.contains(pv_beta)) {
      std::cout << "no image for beta!" << std::endl;
      good = false;
    }
    if(!s.contains(pv_gamma)) {
      std::cout << "no image for gamma!" << std::endl;
      good = false;
    }

    if(good) {
      t_term_full_ref im = ctx_tm.instantiate(i_sum, s);
      // t_term_full_ref im = ctx_tm.instantiate(alpha, s);
      std::cout << "im = \"" << ctx_print.print(im) << "\"\n"; 
    }

  }


  void test_rewrite() {
    std::cout << "= hrewrite - rewrite\n";
    t_ctx_tm ctx_tm;
    t_ctx_rw ctx_rw(ctx_tm);
    t_print ctx_print;

    t_term_full_ref alpha (ctx_tm.create_vterm("int"));
    t_term_full_ref beta  (ctx_tm.create_vterm("int"));

    // semantics of plus
    t_term_full_ref zero       (ctx_tm.create_sterm(c_zero));
    t_term_full_ref p_plus_1   (ctx_tm.create_sterm(c_plus, t_container({zero, alpha})));
    t_term_full_ref i_plus_1   (alpha);
    ctx_rw.add(p_plus_1, i_plus_1);

    t_term_full_ref p_plus_2   (ctx_tm.create_sterm(c_plus, t_container({
      ctx_tm.create_sterm(c_succ, t_container({alpha})),
      beta
    })));
    t_term_full_ref i_plus_2   (ctx_tm.create_sterm(c_plus, t_container({
      alpha,
      ctx_tm.create_sterm(c_succ, t_container({beta}))
    })));
    ctx_rw.add(p_plus_2, i_plus_2);

    // semantics of sum
    if constexpr(get_p_complexity_v<config> == hrw::utils::parsing_complexity::FULL) {
      t_term_full_ref gamma   (ctx_tm.create_vterm("int*"));

      t_term_full_ref p_sum_1 (ctx_tm.create_sterm(c_sum, t_container()));
      t_term_full_ref i_sum_1 (zero);
      ctx_rw.add(p_sum_1, i_sum_1);

      t_term_full_ref p_sum_2 (ctx_tm.create_sterm(c_sum, t_container({alpha})));
      t_term_full_ref i_sum_2 (alpha);
      ctx_rw.add(p_sum_2, i_sum_2);

      t_term_full_ref p_sum_3 (ctx_tm.create_sterm(c_sum, t_container({alpha, beta, gamma})));
      t_term_full_ref i_sum_3 (ctx_tm.create_sterm(c_sum, t_container({ctx_tm.create_sterm(c_plus, t_container({alpha, beta})), gamma})));
      ctx_rw.add(p_sum_3, i_sum_3);
    }


    std::cout << "registered rules:\n" << ctx_rw;

    ///////////////////////
    // rw testing
    auto incr  = [&](t_term_full_ref t) { return ctx_tm.create_sterm(c_succ, t_container({t})); };
    //ctx_tm.m_registry.print(ctx_print);
    t_term_full_ref one   (incr(zero));
    t_term_full_ref two   (incr(one));
    t_term_full_ref three (incr(two));
    t_term_full_ref eight (incr(incr(incr(incr(incr(incr(incr(incr(zero)))))))));

    t_term_full const * eight_ptr = t_term_full::make_ptr_struct::to_ptr(eight);
    std::cout << "eight [" << (void*)(eight_ptr) << "] = " << ctx_print.print(eight) << std::endl;

    t_term_full_ref plus  (ctx_tm.create_sterm(c_plus,  t_container({
      ctx_tm.create_sterm(c_sum,  t_container({two, three})),
      ctx_tm.create_sterm(c_sum,  t_container({two, one}))
    })));

    t_term_full_ref plus_res (ctx_rw.rewrite(plus));
    t_term_full const * plus_res_ptr = t_term_full::make_ptr_struct::to_ptr(plus_res);
    // CHECK_EQ(plus_res, eight);
    std::cout << "plus_res [" << (void*)(plus_res_ptr) << "] = " << ctx_print.print(plus_res) << std::endl;

    if constexpr(get_p_complexity_v<config> == hrw::utils::parsing_complexity::FULL) {
      t_term_full_ref sum     (ctx_tm.create_sterm(c_sum,  t_container({two, three, two, one})));
      t_term_full_ref sum_res (ctx_rw.rewrite(sum));
      t_term_full const * sum_res_ptr = t_term_full::make_ptr_struct::to_ptr(sum_res);
      std::cout << "sum_res [" << (void*)(sum_res_ptr) << "] = " << ctx_print.print(sum_res) << std::endl;
      // CHECK_EQ(sum_res, eight);
    }

  }

  // 5. wrap up
  void run() {
    std::cout << "  - main" << std::endl;
    this->test_fields();
    this->test_term_creation();
    this->test_manipulation();
    this->test_rewrite();
  }
};


template<typename config, typename can_run=void> struct test_unicity;
template<typename config> struct test_unicity<config, std::enable_if_t<!get_hrewrite<config>::t_ctx_tm::ensure_unique_v, void>> { void run() {} };
template<typename config> struct test_unicity<config, std::enable_if_t<get_hrewrite<config>::t_ctx_tm::ensure_unique_v, void>> {
  // 1. types
  using t_hrewrite = get_hrewrite<config>;

  using ctx_th = typename t_hrewrite::ctx_th;
  using t_ctx_tm = typename t_hrewrite::t_ctx_tm;

  using t_term_full = typename t_hrewrite::t_term_full;
  using t_term_full_ref = typename t_hrewrite::t_term_full_ref;
  using t_variable = typename t_hrewrite::t_variable;
  using t_substitution = typename t_hrewrite::t_substitution;

  using t_ctx_rw = typename t_hrewrite::t_ctx_rw;

  using t_print = typename t_hrewrite::t_print;

  static inline constexpr bool has_th_free = is_valid_sth_v<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>;
  static inline constexpr bool has_th_lit_int = is_valid_sth_v<hrw::theory::tp_theory_literal<int>::template type, config>;
  static inline constexpr bool has_th_lit_double = is_valid_sth_v<hrw::theory::tp_theory_literal<double>::template type, config>;
  static inline constexpr bool has_th_leaf = is_valid_sth_v<hrw::theory::tp_theory_leaf::template type, config>;


  //// construction of t_term_theory
  template<typename targ_config, bool value> static constexpr bool tmp = value;

  struct S_default {};

  template<typename targ_config, bool=tmp<targ_config, has_th_leaf>> struct S_leaf;
  template<typename targ_config> struct S_leaf<targ_config, true> {
    using type = get_stheory_t<hrw::theory::tp_theory_leaf::template type, config>;
  };
  template<typename targ_config> struct S_leaf<targ_config, false>: public S_default {};

  template<typename targ_config, bool=tmp<targ_config, has_th_lit_double>> struct S_lit_double;
  template<typename targ_config> struct S_lit_double<targ_config, true> {
    using type = get_stheory_t<hrw::theory::tp_theory_literal<double>::template type, config>;
  };
  template<typename targ_config> struct S_lit_double<targ_config, false>: public S_leaf<targ_config> {};

  template<typename targ_config, bool=tmp<targ_config, has_th_lit_int>> struct S_lit_int;
  template<typename targ_config> struct S_lit_int<targ_config, true> {
    using type = get_stheory_t<hrw::theory::tp_theory_literal<int>::template type, config>;
  };
  template<typename targ_config> struct S_lit_int<targ_config, false>: public S_lit_double<targ_config> {};

  template<typename targ_config, bool=tmp<targ_config, has_th_free>> struct S_free;
  template<typename targ_config> struct S_free<targ_config, true> {
    using type = get_stheory_t<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>;
  };
  template<typename targ_config> struct S_free<targ_config, false>: public S_lit_int<targ_config> {};


  template<typename targ_config> using S = S_free<targ_config>;
  using t_term_theory = typename S<config>::type;
  using t_term = typename t_term_theory::template tt_term<t_term_full>;



  static t_term_full_ref get_term(t_ctx_tm & ctx_tm, t_constructor_core<t_term_theory> const c) {
    if constexpr(has_th_free) {
      using t_container = typename t_term_theory::template tt_term<t_term_full>::t_container;
      return ctx_tm.create_sterm(c, t_container());
    } else if constexpr(has_th_lit_int) {
      return ctx_tm.create_sterm(c, 0);
    } else if constexpr(has_th_lit_double) {
      return ctx_tm.create_sterm(c, 0.0);
    } else if constexpr(has_th_leaf) {
      return ctx_tm.create_sterm(c);
    }
  }

  t_constructor_core<t_term_theory> get_c_zero() {
    if constexpr(has_th_free) {
      return ctx_th::template add_constructor<t_term_theory>(sort_int, "zero", "");
    } else {
      return ctx_th::template add_constructor<t_term_theory>(sort_int, "zero");
    }
  }

  t_constructor_core<t_term_theory> get_c_one() {
    if constexpr(has_th_free) {
      return ctx_th::template add_constructor<t_term_theory>(sort_int, "one", "");
    } else {
      return ctx_th::template add_constructor<t_term_theory>(sort_int, "one");
    }
  }


  t_sort_id sort_int;

  t_constructor_core<t_term_theory> c_zero;
  t_constructor_core<t_term_theory> c_one;


  test_unicity():
    sort_int(ctx_th::add_sort("int")),
    c_zero(get_c_zero()),
    c_one (get_c_one()) {}

  void variable_without() {
    t_ctx_tm ctx_tm;
    t_ctx_rw ctx_rw(ctx_tm);
    t_print ctx_print;



    t_term_full_ref zero_1 (get_term(ctx_tm, c_zero));
    t_term_full_ref zero_2 (get_term(ctx_tm, c_zero));

    t_term_full_ref one_1  (get_term(ctx_tm, c_one));
    t_term_full_ref one_2  (get_term(ctx_tm, c_one));

    CHECK_EQ(zero_1, zero_2);
    CHECK_EQ(one_1, one_2);
    CHECK_NE(zero_1, one_1);
    CHECK_NE(zero_2, one_1);
    CHECK_NE(zero_1, one_2);
    CHECK_NE(zero_2, one_2);

    // with rewriting now
    ctx_rw.add(zero_1, one_2);

    t_term_full_ref res_4 (ctx_rw.rewrite(one_2));
    t_term_full_ref res_1 (ctx_rw.rewrite(zero_1));
    t_term_full_ref res_2 (ctx_rw.rewrite(zero_2));
    t_term_full_ref res_3 (ctx_rw.rewrite(one_1));

    CHECK_EQ(res_1, one_2);
    CHECK_EQ(res_2, one_2);
    CHECK_EQ(res_3, one_2);
    CHECK_EQ(res_4, one_2);

  }

  void variable_with() {
    if constexpr(has_th_free) {
      using t_container = typename t_term_theory::template tt_term<t_term_full>::t_container;

      t_ctx_tm ctx_tm;
      t_ctx_rw ctx_rw(ctx_tm);
      t_print ctx_print;

      t_constructor_core<t_term_theory> c_succ = ctx_th::template add_constructor<t_term_theory>(sort_int, "succ", "int");
      t_constructor_core<t_term_theory> c_plus = ctx_th::template add_constructor<t_term_theory>(sort_int, "plus", "int int");

      auto incr  = [&](t_term_full_ref t) { return ctx_tm.create_sterm(c_succ, t_container({t})); };
      auto plus  = [&](t_term_full_ref t1, t_term_full_ref t2) { return ctx_tm.create_sterm(c_plus, t_container({t1, t2})); };

      t_term_full_ref zero  (get_term(ctx_tm, c_zero));
      t_term_full_ref one   (incr(zero));
      t_term_full_ref two   (incr(one));
      t_term_full_ref three (incr(two));

      t_term_full_ref alpha (ctx_tm.create_vterm("int"));
      t_term_full_ref beta  (ctx_tm.create_vterm("int"));

      // semantics of plus
      ctx_rw.add(plus(zero, alpha), alpha);
      ctx_rw.add(plus(incr(alpha), beta), plus(alpha, incr(beta)));

      t_term_full_ref res_1 (ctx_rw.rewrite(plus(one, two)));
      t_term_full_ref res_2 (ctx_rw.rewrite(plus(two, one)));

      CHECK_EQ(res_1, res_2);
      CHECK_EQ(res_1, three);
    }
  }

  void instantiate() {
    t_ctx_tm ctx_tm;
    t_print ctx_print;
    t_substitution s;

    t_term_full_ref zero_1 (get_term(ctx_tm, c_zero));
    t_term_full_ref zero_2 (get_term(ctx_tm, c_zero));

    t_term_full_ref res_1  (ctx_tm.instantiate(zero_2, s));

    CHECK_EQ(zero_1, res_1);
    CHECK_EQ(zero_2, res_1);

  }

  void create_from_diff() {
    if constexpr(has_th_free) {
      using t_container = typename t_term::t_container;
      t_ctx_tm ctx_tm;

      t_term_full_ref zero_1 (get_term(ctx_tm, c_zero));
      t_term const * r_zero_1 = zero_1->template get_if<t_term>();
      CHECK_NE(r_zero_1, nullptr);

      t_term_full_ref zero_2 (ctx_tm.create_sterm_from_diff(*r_zero_1, t_container()));
      CHECK_EQ(zero_1, zero_2);
    }
  }

  void run() {
    std::cout << "  - main" << std::endl;
    this->variable_without();
    this->variable_with();
    this->instantiate();
    this->create_from_diff();

  }

};



template<typename config, typename can_run=void> struct test_eval_lit;

template<typename config>
struct test_eval_lit<config, std::enable_if_t<
    !is_valid_sth_v<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>
    || !is_valid_sth_v<hrw::theory::tp_theory_literal<int>::template type, config>
  , void>> { void run() {} };

template<typename config>
struct test_eval_lit<config, std::enable_if_t<
    is_valid_sth_v<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>
    && is_valid_sth_v<hrw::theory::tp_theory_literal<int>::template type, config>
  , void>> {
  // 1. types
  using type = test_eval_lit<config, void>;
  using t_hrewrite = get_hrewrite<config>;

  using ctx_th = typename t_hrewrite::ctx_th;
  using t_ctx_tm = typename t_hrewrite::t_ctx_tm;

  using t_term_full = typename t_hrewrite::t_term_full;
  using t_term_full_ref = typename t_hrewrite::t_term_full_ref;
  using t_variable = typename t_hrewrite::t_variable;
  using t_substitution = typename t_hrewrite::t_substitution;

  using t_ctx_rw = typename t_hrewrite::t_ctx_rw;
  using t_guard = typename t_ctx_rw::t_guard;

  using t_print = typename t_hrewrite::t_print;

  using t_theory_free = get_stheory_t<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>;
  using t_theory_lit_int = get_stheory_t<hrw::theory::tp_theory_literal<int>::template type, config>;

  using t_term_free = get_sterm_t<hrw::theory::tp_theory_free<std::vector, t_sparser>::template type, config>;
  using t_term_lit_int = get_sterm_t<hrw::theory::tp_theory_literal<int>::template type, config>;

  using t_container = typename t_theory_free::template tt_term<t_term_full>::t_container;


  // 2. fields
  t_sort_id sort_int   ;

  t_constructor_core<t_theory_lit_int>    c_val         ;
  t_constructor_core<t_theory_free>       c_succ        ;
  t_constructor_core<t_theory_free>       c_plus        ;

  // 3. constructor
  test_eval_lit():
    sort_int       (ctx_th::add_sort("int")),
    c_val          (ctx_th::template add_constructor<t_theory_lit_int>(sort_int, "val")),
    c_succ         (ctx_th::template add_constructor<t_theory_free>(sort_int, "succ", "int")),
    c_plus         (ctx_th::template add_constructor<t_theory_free>(sort_int, "plus", "int int")) {}


  static void insert_single(t_substitution& s, t_term_full_ref alpha, t_container& c, t_term_full_ref t) {
    const t_variable* alpha_ref = alpha->template get_if<t_variable>();
    assert(alpha_ref != nullptr);
    if constexpr(get_p_complexity_v<config> >= hrw::utils::parsing_complexity::SEQUENCE) {
      c.clear();
      c.push_back(t);
      if constexpr(t_term_full::is_const_v) {
        s.insert(alpha_ref, c.cbegin(), c.cend());
      } else {
        s.insert(alpha_ref, c.begin(), c.end());
      }
    } else {
      s.insert(alpha_ref, t);
    }
  }

  void run() {
    std::cout << "  - main" << std::endl;
    t_print ctx_print;

    t_ctx_tm ctx_tm;
    t_ctx_rw ctx_rw(ctx_tm);

    auto val  = [&](int v) { return ctx_tm.create_sterm(c_val, v); };
    auto succ  = [&](t_term_full_ref t) { return ctx_tm.create_sterm(c_succ, t_container({t})); };
    auto plus  = [&](t_term_full_ref t1, t_term_full_ref t2) { return ctx_tm.create_sterm(c_plus, t_container({t1, t2})); };

    t_term_full_ref alpha (ctx_tm.create_vterm("int"));
    t_term_full_ref beta  (ctx_tm.create_vterm("int"));
    t_term_full_ref gamma (ctx_tm.create_vterm("int"));

    t_container c_succ;
    t_guard guard_succ = [alpha, beta, &ctx_tm, &c_succ, this](t_ctx_rw *, t_substitution * s) {
      // std::cout << "executing guard_succ" << std::endl;
      t_term_full_ref v = ctx_tm.instantiate(alpha, *s);
      t_term_lit_int const * v_ref = v->template get_if<t_term_lit_int>();
      if(v_ref != nullptr) {
        t_term_full_ref v_succ = ctx_tm.create_sterm(c_val, v_ref->get_value() + 1);
        type::insert_single(*s, beta, c_succ, v_succ);
        return true;
      } else {
        return false;
      }
    };
    ctx_rw.add(succ(alpha), beta, guard_succ);

    t_container c_plus;
    t_guard guard_plus = [alpha, beta, gamma, &ctx_tm, &c_plus, this](t_ctx_rw *, t_substitution * s) {
      // std::cout << "executing guard_plus" << std::endl;
      t_term_full_ref v1 = ctx_tm.instantiate(alpha, *s);
      t_term_full_ref v2 = ctx_tm.instantiate(beta, *s);
      t_term_lit_int const * v1_ref = v1->template get_if<t_term_lit_int>();
      t_term_lit_int const * v2_ref = v2->template get_if<t_term_lit_int>();
      if((v1_ref != nullptr) && (v2_ref != nullptr)) {
        t_term_full_ref v_plus = ctx_tm.create_sterm(c_val, v1_ref->get_value() + v2_ref->get_value());
        type::insert_single(*s, gamma, c_plus, v_plus);
        return true;
      } else {
        return false;
      }
    };
    ctx_rw.add(plus(alpha, beta), gamma, guard_plus);

    t_term_full_ref p1 (plus(succ(succ(val(1))), plus(val(2), succ(val(2)))));
    t_term_full_ref i1 (val(8));
    // std::cout << "checking " << ctx_print.print(i1) << " == ctx_rw.rewrite(" << ctx_print.print(p1) << ") [ with rewrite = " << ctx_print.print(ctx_rw.rewrite(p1)) <<"]" << std::endl;
    if constexpr(t_ctx_tm::ensure_unique_v) {
      CHECK_EQ(i1, ctx_rw.rewrite(p1));
    } else {
      using t_eq = typename t_term_full::template t_eq_ref<true>;
      CHECK(t_eq()(i1, ctx_rw.rewrite(p1)));
    }

    if constexpr(
      (get_p_complexity_v<config> == hrw::utils::parsing_complexity::FULL)
      && (t_theory_free::t_spec::complexity == hrw::utils::parsing_complexity::FULL)
    ) {
      t_constructor_core<t_theory_free> c_sum = ctx_th::template add_constructor<t_theory_free>(sort_int, "sum", "int*");
      t_term_full_ref eta (ctx_tm.create_vterm("int*"));
      const t_variable* eta_ref = eta->template get_if<t_variable>();

      t_container cont_sum;
      t_guard guard_sum = [eta_ref, alpha, &cont_sum, &ctx_tm, this](t_ctx_rw *, t_substitution * s) {
        cont_sum.clear(); // retrieve adds the new content at the end of the container that might contain something (due to previous execution)
        s->retrieve(eta_ref, cont_sum);
        int res = 0;
        for(t_term_full_ref v: cont_sum) {
          t_term_lit_int const * v_ref = v->template get_if<t_term_lit_int>();
          if(v_ref != nullptr) {
            res += v_ref->get_value();
          } else {
            return false;
          }
        }
        t_term_full_ref v_sum (ctx_tm.create_sterm(c_val, res));
        type::insert_single(*s, alpha, cont_sum, v_sum);
        return true;
      };
      ctx_rw.add(ctx_tm.create_sterm(c_sum, t_container({eta})), alpha, guard_sum);

      t_term_full_ref p2 (ctx_tm.create_sterm(c_sum, t_container({plus(succ(val(1)), val(1)), val(2), succ(val(2))})));
      // std::cout << "checking " << ctx_print.print(i1) << " == ctx_rw.rewrite(" << ctx_print.print(p2) << ") [ with rewrite = " << ctx_print.print(ctx_rw.rewrite(p2)) <<"]" << std::endl;
      if constexpr(t_ctx_tm::ensure_unique_v) {
        CHECK_EQ(i1, ctx_rw.rewrite(p2));
      } else {
        using t_eq = typename t_term_full::template t_eq_ref<true>;
        CHECK(t_eq()(i1, ctx_rw.rewrite(p2)));
      }
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
//// WRAP UP
////////////////////////////////////////////////////////////////////////////////

using t_function_class_list = function_class_list<test_all, test_unicity, test_eval_lit>;
using test_type = test_generation<
  t_term_registry_list, t_map_list, t_sort_context_param_list,
  t_parser_list, t_variable_theory_list, t_structured_theory_list_list,
  t_function_class_list
>;

TEST_CASE("hrewrite - all tests") {
  std::cout << "==================================================================\n";
  std::cout << "= hrewrite - all tests\n";

  test_type::run();

}


#endif

