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
#if ENABLE_HTERM

#include "doctest/doctest.h"

#include "hrewrite/parsing.hpp"
#include "hrewrite/hterm.hpp"
#include "hrewrite/theory/theory_variable.hpp"
#include "hrewrite/theory/theory_free.hpp"
#include "hrewrite/theory/theory_literal.hpp"
#include "hrewrite/theory/theory_leaf.hpp"

#include "hrewrite/utils/type_traits.hpp"

#include "tests/hterm.test.hpp"

using namespace hrw;

#include <iostream>
#include <string>
#include <optional>
#include <vector>
#include <tuple>





////////////////////////////////////////////////////////////////////////////////
//// T_TERM_FULL SETUP
////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////
// 1. list of make_ptr implementations

template<typename T> struct t_make_ptr_normal {
  using value_type = T;
  using value_const = const T;
  using ptr_type = T*;
  using ptr_hash = std::hash<ptr_type>;
  static inline constexpr ptr_type make_ptr(T& t) { return &t; }
  static inline constexpr value_const* to_ptr(ptr_type& t) { return t; }
};

template<typename T> struct t_make_ptr_const {
  using value_type = T;
  using value_const = const T;
  using ptr_type = T*;
  using ptr_hash = std::hash<ptr_type>;
  static inline constexpr ptr_type make_ptr(T& t) { return &t; }
  static inline constexpr value_const* to_ptr(ptr_type& t) { return t; }
};


// using t_make_ptr_list = make_make_ptr_list<t_make_ptr_normal>;
using t_make_ptr_list = make_make_ptr_list<t_make_ptr_normal, t_make_ptr_const>;


//////////////////////////////////////////
// 2. parsers

template<typename t_alphabet, const t_alphabet& alphabet>
using my_automata = utils::tt_automata<hrw::utils::natset, hrw::utils::natset>::template type<t_alphabet, alphabet>;

template<typename targ_alphabet, const targ_alphabet& alphabet>
using p_combine = utils::combine_variant<targ_alphabet, alphabet, utils::element, my_automata>;

// using t_vparser_list = t_make_parser_list<p_combine>;
// using t_vparser_list = t_make_parser_list<utils::element>;
// using t_vparser_list = t_make_parser_list<utils::sequence>;
using t_vparser_list = make_parser_list<utils::element, utils::sequence, my_automata, p_combine>;




//////////////////////////////////////////
// 3. list of variable implementations

using t_vth_list = make_vth_list<hrw::theory::tp_theory_variable_vector>;
// using t_vth_list = make_vth_list<hrw::theory::tp_theory_variable_map, hrw::theory::tp_theory_variable_vector>;


//////////////////////////////////////////
// 4. list of structured theory lists

using t_sparser = int; // cannot be void

using t_th_free = hrw::theory::tt_theory_free<t_sparser, std::vector>;
using t_th_leaf = hrw::theory::t_theory_leaf;
using t_th_lit_int = hrw::theory::tt_theory_literal<int>;
using t_th_lit_string = hrw::theory::tt_theory_literal<std::string>;


using t_sth_list_list = std::tuple<make_sth_list<t_th_free>>;

// using t_sth_list_list = std::tuple<
//   make_sth_list<t_th_free>,
//   make_sth_list<t_th_leaf>,
//   make_sth_list<t_th_lit_int>,
//   make_sth_list<t_th_lit_string>,
//   make_sth_list<
//     t_th_free,
//     t_th_leaf,
//     t_th_lit_int,
//     t_th_lit_string
//   >
// >;

////////////////////////////////////////////////////////////////////////////////
//// GENERIC TESTING SETUP
////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////
// sorts and constructors declaration

struct t_sigma_manager {
  using t_letter = unsigned int;
  using t_letter_set = hrw::utils::natset;
  static t_letter get_letter(const std::string& s) { return stoi(s); }
  bool is_subletter(t_letter l, t_letter r) const { return l == r; }
};

static t_sigma_manager sigma_manager;


#define sort_int    0
#define sort_double 1
#define sort_string 2

struct t_sort_resolver {
  static std::string m_content[3];
  static const std::string& get_sort_name(typename t_sigma_manager::t_letter l) { return t_sort_resolver::m_content[l]; }
};
std::string t_sort_resolver::m_content[3] = {"int", "double", "string"};


#define c_zero 0
#define c_succ 1
#define c_plus 2
#define c_sum  3

#define c_int 4
#define c_double 5
#define c_string 6

#define c_string_from_int 7
#define c_string_from_double 8

#define c_epsilon 9
#define c_concact 10

static std::string c_names[11] = {
  "zero",
  "s",
  "+",
  "sum",
  "int",
  "double",
  "string",
  "string_from_int",
  "string_from_double",
  "epsilon",
  "concat"
};

struct t_context_print {
  template<typename t_factory>
  const std::string& get_name_constructor(const t_constructor_id id) const { return c_names[id]; }
  const std::string& get_name_variable(const std::size_t v_id) {
    auto it = this->m_vnames.find(v_id);
    if(it == this->m_vnames.end()) {
      auto pair = this->m_vnames.insert(std::make_pair(v_id, this->new_name()));
      it = pair.first;
    }
    return it->second;
  }
private:
  std::unordered_map<std::size_t, const std::string> m_vnames;
  unsigned int m_count;

  std::string new_name() {
    int vid = this->m_count;
    std::stringstream os;
    ++this->m_count;
    os << '\'';
    while(vid > 26) {
      os << (char)(97 + (vid % 26));
      vid = vid / 26;
    }
    os << (char)(97 + (vid % 26));
    return os.str();
  }
};

static t_context_print context_print;




template<typename termIt>
struct match_helper_iterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = t_sort_id;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;
  using const_pointer = value_type const *;
  using const_reference = value_type const &;

  match_helper_iterator(termIt it): m_it(it) {}
  termIt m_it;
  t_sort_id operator*() { return (*this->m_it)->get_sort(); }
  match_helper_iterator& operator++() { ++(this->m_it); return *this; }
  bool operator==(const match_helper_iterator& other) const { return this->m_it == other.m_it; }
  bool operator!=(const match_helper_iterator& other) const { return this->m_it != other.m_it; }
};


template<typename termIt>
void print_image(const std::pair<termIt,termIt>& p) {
  termIt it_term = p.first;
  termIt it_end = p.second;
  std::cout << "[ ";
  if(it_term != it_end) {
    while(true) {
      // std::cout << "DEBUG: " << (void*)((*it_term)) << std::endl;
      (*it_term)->print(std::cout, context_print);
      ++it_term;
      if(it_term != it_end) {
        std::cout << ", " << std::flush;
      } else {
        break;
      }
    }
  }
  std::cout << " ]";
}

////////////////////////////////////////////////////////////////////////////////
//// TESTING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

template<typename t_make_ptr, typename t_vparser, typename t_vth, typename t_sths>
void print_test(std::string const & s) {
  std::cout << "running test " << s << "<" <<
  hrw::utils::type_name<t_make_ptr>() << ", " <<
  hrw::utils::type_name<t_vparser>() << ", " <<
  hrw::utils::type_name<t_vth>() << ", " <<
  hrw::utils::type_name<t_sths>() << ">" << std::endl;
}

template<typename t_term_full_ref, typename t_variable, typename t_substitution, typename ... Ts>
void check_substitution_image(std::optional<t_substitution> subst, t_term_full_ref alpha, Ts ... args) {
  using t_container = std::vector<t_term_full_ref>;
  t_container c1({args...});
  t_container c2;

  const t_variable* alpha_ref = alpha->template get_if<t_variable>();

  REQUIRE_NE(alpha_ref, nullptr);
  REQUIRE(subst.has_value());
  t_substitution s_match = subst.value();
  s_match.retrieve(alpha_ref, c2);

  CHECK_EQ(c1, c2);
}



//////////////////////////////////////////
// 1. test free

template<typename config, typename can_run=void> struct test_free;

template<typename config>
struct test_free<config, std::enable_if_t<!is_valid_sth_v<t_th_free, config>, void>> { void run() {} };

template<typename config>
struct test_free<config, std::enable_if_t<is_valid_sth_v<t_th_free, config>, void>> {
  using t_term_full = get_term_full_t<config>;
  using t_term_full_ref = typename t_term_full::reference;
  using t_make_ptr = get_make_ptr_t<config>;
  using t_vparser = typename config::t_vparser;

  // using t_vth = get_vth_t<config>;
  using t_vfactory = get_vfactory_t<config>;
  using t_variable = get_vterm_t<config>;
  using t_substitution = get_substitution_t<config>;
  using t_vth = typename config::t_vth;
  using t_sths = typename config::t_sths;

  using t_factory_free = get_sfactory_t<t_th_free, config>;
  using t_term_free = get_sterm_t<t_th_free, config>;
  using t_container = typename t_term_free::t_container;
  using t_iterator = typename t_term_free::t_iterator;

  using t_hash = typename t_term_full::t_hash;
  using t_eq   = typename t_term_full::t_eq;

  using t_factory_leaf = std::conditional_t<is_valid_sth_v<t_th_leaf, config>, get_sfactory_t<t_th_leaf, config>, t_factory_free>;
  using t_term_leaf = std::conditional_t<is_valid_sth_v<t_th_leaf, config>, get_sterm_t<t_th_leaf, config>, t_term_free>;

  static t_term_full create_term_leaf(t_factory_leaf f, const t_sort_id s, const t_constructor_id c) {
    if constexpr(is_valid_sth_v<t_th_leaf, config>) {
      return f.create_term(s, c);
    } else {
      return f.create_term(s, c, t_container({}));
    }
  }

  void run() {
    std::cout << "  - main" << std::endl;

    t_vfactory factory_variable;
    t_factory_free factory_free;
    t_factory_leaf factory_leaf;


    // 1. term creation
    t_term_full zero  = create_term_leaf(factory_leaf, sort_int, c_zero);
    t_term_full one   = factory_free.create_term(sort_int, c_succ, t_container({t_make_ptr::make_ptr(zero)}));
    t_term_full two   = factory_free.create_term(sort_int, c_succ, t_container({t_make_ptr::make_ptr(one)}));
    t_term_full three = factory_free.create_term(sort_int, c_succ, t_container({t_make_ptr::make_ptr(two)}));
    t_term_full plus  = factory_free.create_term(sort_int, c_plus, t_container({t_make_ptr::make_ptr(two), t_make_ptr::make_ptr(three)}));
    t_term_full sum   = factory_free.create_term(sort_int, c_sum , t_container({t_make_ptr::make_ptr(plus), t_make_ptr::make_ptr(two), t_make_ptr::make_ptr(three)}));

    t_term_full print = factory_free.create_term(sort_string, c_string_from_int, t_container({t_make_ptr::make_ptr(zero)}));


    // base variable spec, should always work
    t_term_full alpha = factory_variable.create_term(std::to_string(sort_int));
    t_term_full beta  = factory_variable.create_term(std::to_string(sort_int));

    t_term_full_ref r_zero = t_make_ptr::make_ptr(zero);
    const t_term_leaf* f_zero = zero.template get_if<t_term_leaf>();
    t_term_full_ref r_one = t_make_ptr::make_ptr(one);
    const t_term_free* f_one = one.template get_if<t_term_free>();
    t_term_full_ref r_plus = t_make_ptr::make_ptr(plus);
    const t_term_free* f_plus = plus.template get_if<t_term_free>();
    t_term_full_ref r_sum = t_make_ptr::make_ptr(sum);
    const t_term_free* f_sum = sum.template get_if<t_term_free>();

    t_term_full_ref r_alpha = t_make_ptr::make_ptr(alpha);
    const t_variable* alpha_ref = alpha.template get_if<t_variable>();
    t_term_full_ref r_beta = t_make_ptr::make_ptr(beta);
    const t_variable* beta_ref  = beta .template get_if<t_variable>();

    CHECK(r_zero    != nullptr);
    CHECK(f_zero    != nullptr);

    CHECK(r_one     != nullptr);
    CHECK(f_one     != nullptr);

    CHECK(alpha_ref != nullptr);
    CHECK(beta_ref  != nullptr);


    // 2. term print
    std::cout << "     zero  = \""; zero .print(std::cout, context_print); std::cout << "\"\n"; 
    std::cout << "     one   = \""; one  .print(std::cout, context_print); std::cout << "\"\n"; 
    std::cout << "     two   = \""; two  .print(std::cout, context_print); std::cout << "\"\n"; 
    std::cout << "     three = \""; three.print(std::cout, context_print); std::cout << "\"\n"; 
    std::cout << "     plus  = \""; plus .print(std::cout, context_print); std::cout << "\"\n"; 
    std::cout << "     sum   = \""; sum  .print(std::cout, context_print); std::cout << "\"\n"; 
    std::cout << "     print = \""; print.print(std::cout, context_print); std::cout << "\"\n"; 


    CHECK(zero.get_spec() == "int");
    CHECK(print.get_spec() == "string");


    // 3. hash and eq
    size_t hash_zero  = t_hash()(zero );
    size_t hash_one   = t_hash()(one  );
    size_t hash_two   = t_hash()(two  );
    size_t hash_three = t_hash()(three);
    size_t hash_plus  = t_hash()(plus );
    size_t hash_sum   = t_hash()(sum  );

    WARN_NE(hash_zero, hash_one);
    WARN_NE(hash_zero, hash_two);
    WARN_NE(hash_zero, hash_three);
    WARN_NE(hash_zero, hash_plus);
    WARN_NE(hash_zero, hash_sum);

    CHECK(t_eq()(zero, zero ));
    CHECK_FALSE(t_eq()(zero, one  ));
    CHECK_FALSE(t_eq()(zero, two  ));
    CHECK_FALSE(t_eq()(zero, three));
    CHECK_FALSE(t_eq()(zero, plus ));
    CHECK_FALSE(t_eq()(zero, sum  ));

    CHECK_FALSE(t_eq()(one, zero ));
    CHECK(t_eq()(one, one  ));
    CHECK_FALSE(t_eq()(one, two  ));
    CHECK_FALSE(t_eq()(one, three));
    CHECK_FALSE(t_eq()(one, plus ));
    CHECK_FALSE(t_eq()(one, sum  ));


    // 4.  sort iterator
    t_container c_term_sort_iterator({
      t_make_ptr::make_ptr(zero ),
      t_make_ptr::make_ptr(one  ),
      t_make_ptr::make_ptr(two  ),
      t_make_ptr::make_ptr(three),
      t_make_ptr::make_ptr(plus ),
      t_make_ptr::make_ptr(sum  ),
      t_make_ptr::make_ptr(print)
    });

    std::vector<t_sort_id> c_sort_sort_iterator({
      sort_int,
      sort_int,
      sort_int,
      sort_int,
      sort_int,
      sort_int,
      sort_string
    });


    match_helper_iterator<t_iterator> begin = match_helper_iterator(c_term_sort_iterator.begin());
    match_helper_iterator<t_iterator> end = match_helper_iterator(c_term_sort_iterator.end());
    std::size_t i = 0;
    for(auto it = begin; it != end; ++it) {
      CHECK_MESSAGE(*it == c_sort_sort_iterator[i], "iteration ", i);
      CHECK_MESSAGE(c_term_sort_iterator[i]->get_sort() == c_sort_sort_iterator[i], "iteration ", i);
      ++i;
    }


    // 5. substitution
    CHECK(alpha.get_spec() == std::to_string(sort_int));

    t_container c1_substitution({r_zero});

    t_substitution s_substitution;
    if constexpr(get_pcomplexity_v<config> >= hrw::utils::parsing_complexity::SEQUENCE) {
      s_substitution.insert(alpha_ref, c1_substitution.begin(), c1_substitution.end());
    } else {
      s_substitution.insert(alpha_ref, c1_substitution[0]);
    }
    std::optional<t_substitution> subst_substitution(s_substitution);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_substitution, r_alpha, r_zero);


    // 6. match
    std::optional<t_substitution> subst_match1 = alpha.match(r_zero);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match1, r_alpha, r_zero);

    t_term_full term_pattern_match2 = factory_free.create_term(sort_int, c_succ, t_container({t_make_ptr::make_ptr(alpha)}));
    std::optional<t_substitution> subst_match2 = term_pattern_match2.match(r_one);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match2, r_alpha, r_zero);


    t_term_full term_pattern_match3  = factory_free.create_term(sort_int, c_plus, t_container({r_alpha, r_beta}));
    std::optional<t_substitution> subst_match3 = term_pattern_match3.match(r_plus);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match3, r_alpha, t_make_ptr::make_ptr(two));
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match3, r_beta, t_make_ptr::make_ptr(three));


    if constexpr(get_pcomplexity_v<config> >= hrw::utils::parsing_complexity::SEQUENCE) {
      std::cout << "  - sequence" << std::endl;
      t_term_full gamma = factory_variable.create_term(std::to_string(sort_int) + " " + std::to_string(sort_int));
      t_term_full_ref r_gamma = t_make_ptr::make_ptr(gamma);

      t_term_full term_pattern_match4   = factory_free.create_term(sort_int, c_sum , t_container({r_alpha, r_gamma}));
      std::optional<t_substitution> subst_match4 = term_pattern_match4.match(r_sum);
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match4, r_alpha, t_make_ptr::make_ptr(plus));
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match4, r_gamma, t_make_ptr::make_ptr(two), t_make_ptr::make_ptr(three));

      t_term_full term_pattern_match5   = factory_free.create_term(sort_int, c_sum , t_container({r_gamma, r_alpha}));
      std::optional<t_substitution> subst_match5 = term_pattern_match5.match(r_sum);
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match5, r_gamma, t_make_ptr::make_ptr(plus), t_make_ptr::make_ptr(two));
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match5, r_alpha, t_make_ptr::make_ptr(three));


    t_term_full sum   = factory_free.create_term(sort_int, c_sum , t_container({t_make_ptr::make_ptr(plus), t_make_ptr::make_ptr(two), t_make_ptr::make_ptr(three)}));

    }

    if constexpr(get_pcomplexity_v<config> >= hrw::utils::parsing_complexity::FULL) {
      std::cout << "  - full" << std::endl;
      t_term_full gamma = factory_variable.create_term(std::to_string(sort_int) + "*");
      t_term_full_ref r_gamma = t_make_ptr::make_ptr(gamma);

      t_term_full term_pattern_match4   = factory_free.create_term(sort_int, c_sum , t_container({r_alpha, r_gamma}));
      std::optional<t_substitution> subst_match4 = term_pattern_match4.match(r_sum);
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match4, r_alpha, t_make_ptr::make_ptr(plus));
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match4, r_gamma, t_make_ptr::make_ptr(two), t_make_ptr::make_ptr(three));

      t_term_full term_pattern_match5   = factory_free.create_term(sort_int, c_sum , t_container({r_gamma, r_alpha}));
      std::optional<t_substitution> subst_match5 = term_pattern_match5.match(r_sum);
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match5, r_gamma, t_make_ptr::make_ptr(plus), t_make_ptr::make_ptr(two));
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match5, r_alpha, t_make_ptr::make_ptr(three));

      t_term_full term_pattern_match6   = factory_free.create_term(sort_int, c_sum , t_container({r_alpha, r_beta, r_gamma}));
      std::optional<t_substitution> subst_match6 = term_pattern_match6.match(r_sum);
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match6, r_alpha, t_make_ptr::make_ptr(plus));
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match6, r_beta, t_make_ptr::make_ptr(two));
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match6, r_gamma, t_make_ptr::make_ptr(three));

      t_term_full term_pattern_match7  = factory_free.create_term(sort_int, c_plus, t_container({r_alpha, r_beta, r_gamma}));
      std::optional<t_substitution> subst_match7 = term_pattern_match7.match(r_plus);
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match7, r_alpha, t_make_ptr::make_ptr(two));
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match7, r_beta, t_make_ptr::make_ptr(three));
      check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match7, r_gamma);
    }
  }
};


//////////////////////////////////////////
// 2. test leaf

template<typename config, typename can_run=void> struct test_leaf;

template<typename config>
struct test_leaf<config, std::enable_if_t<!is_valid_sth_v<t_th_leaf, config>, void>> { void run() {} };

template<typename config>
struct test_leaf<config, std::enable_if_t<is_valid_sth_v<t_th_leaf, config>, void>> {
  using t_term_full = get_term_full_t<config>;
  using t_term_full_ref = typename t_term_full::reference;
  using t_make_ptr = get_make_ptr_t<config>;
  using t_vparser = typename config::t_vparser;

  // using t_vth = get_vth_t<config>;
  using t_vfactory = get_vfactory_t<config>;
  using t_variable = get_vterm_t<config>;
  using t_substitution = get_substitution_t<config>;
  using t_vth = typename config::t_vth;
  using t_sths = typename config::t_sths;

  using t_factory_leaf = get_sfactory_t<t_th_leaf, config>;
  using t_term_leaf = get_sterm_t<t_th_leaf, config>;

  using t_hash = typename t_term_full::t_hash;
  using t_eq   = typename t_term_full::t_eq;

  using t_container = std::vector<t_term_full_ref>;

  void run() {
    std::cout << "  - main" << std::endl;

    t_vfactory factory_variable;
    t_factory_leaf factory_leaf;

    // 1. term creation
    t_term_full zero  = factory_leaf.create_term(sort_int, c_zero);

    t_term_full alpha = factory_variable.create_term(std::to_string(sort_int));

    t_term_full_ref r_zero = t_make_ptr::make_ptr(zero);
    const t_term_leaf* f_zero = zero.template get_if<t_term_leaf>();

    t_term_full_ref r_alpha = t_make_ptr::make_ptr(alpha);
    const t_variable* alpha_ref = alpha.template get_if<t_variable>();

    CHECK(r_zero    != nullptr);
    CHECK(f_zero    != nullptr);
    CHECK(r_alpha   != nullptr);
    CHECK(alpha_ref != nullptr);

    // 2. term print
    std::cout << "     zero  = \""; zero .print(std::cout, context_print); std::cout << "\"\n"; 

    // 3. spec check
    CHECK(zero.get_spec() == "int");
    CHECK(alpha.get_spec() == std::to_string(sort_int));

    CHECK(f_zero->get_sort() == sort_int);
    CHECK(f_zero->get_constructor() == c_zero);

    size_t hash_zero  = t_hash()(zero );

    // 4. substitution
    t_container c1_substitution({r_zero});

    t_substitution s_substitution;
    if constexpr(get_pcomplexity_v<config> >= hrw::utils::parsing_complexity::SEQUENCE) {
      s_substitution.insert(alpha_ref, c1_substitution.begin(), c1_substitution.end());
    } else {
      s_substitution.insert(alpha_ref, c1_substitution[0]);
    }
    std::optional<t_substitution> subst_substitution(s_substitution);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_substitution, r_alpha, r_zero);

    // 6. match
    std::optional<t_substitution> subst_match1 = alpha.match(r_zero);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match1, r_alpha, r_zero);
  }
};

//////////////////////////////////////////
// 3. test literal


using t_th_lit_int = hrw::theory::tt_theory_literal<int>;
using t_th_lit_string = hrw::theory::tt_theory_literal<std::string>;

template<typename config, typename can_run=void> struct test_literal;

template<typename config>
struct test_literal<config, std::enable_if_t<
    !is_valid_sth_v<t_th_lit_int, config>
    || !is_valid_sth_v<t_th_lit_string, config>
    , void>> { void run() {} };

template<typename config>
struct test_literal<config, std::enable_if_t<
    is_valid_sth_v<t_th_lit_int, config>
    && is_valid_sth_v<t_th_lit_string, config>
    , void>> {
  using t_term_full = get_term_full_t<config>;
  using t_term_full_ref = typename t_term_full::reference;
  using t_make_ptr = get_make_ptr_t<config>;
  using t_vparser = typename config::t_vparser;

  // using t_vth = get_vth_t<config>;
  using t_vfactory = get_vfactory_t<config>;
  using t_variable = get_vterm_t<config>;
  using t_substitution = get_substitution_t<config>;
  using t_vth = typename config::t_vth;
  using t_sths = typename config::t_sths;

  using t_factory_lit_int    = get_sfactory_t<t_th_lit_int, config>;
  using t_factory_lit_string = get_sfactory_t<t_th_lit_string, config>;
  using t_term_lit_int    = get_sterm_t<t_th_lit_int, config>;
  using t_term_lit_string = get_sterm_t<t_th_lit_string, config>;

  using t_hash = typename t_term_full::t_hash;
  using t_eq   = typename t_term_full::t_eq;

  using t_container = std::vector<t_term_full_ref>;

  void run() {
    std::cout << "  - main" << std::endl;

    t_vfactory factory_variable;
    t_factory_lit_int factory_lit_int;
    t_factory_lit_string factory_lit_string;

    // 1. term creation
    t_term_full zero_int    = factory_lit_int.create_term(sort_int, c_int, 0);
    t_term_full zero_string = factory_lit_string.create_term(sort_int, c_int, "0");

    t_term_full alpha = factory_variable.create_term(std::to_string(sort_int));

    t_term_full_ref r_zero_int = t_make_ptr::make_ptr(zero_int);
    const t_term_lit_int* f_zero_int = zero_int.template get_if<t_term_lit_int>();
    t_term_full_ref r_zero_string = t_make_ptr::make_ptr(zero_string);
    const t_term_lit_string* f_zero_string = zero_string.template get_if<t_term_lit_string>();

    t_term_full_ref r_alpha = t_make_ptr::make_ptr(alpha);
    const t_variable* alpha_ref = alpha.template get_if<t_variable>();

    CHECK(r_zero_int    != nullptr);
    CHECK(f_zero_int    != nullptr);
    CHECK(r_zero_string != nullptr);
    CHECK(f_zero_string != nullptr);
    CHECK(r_alpha       != nullptr);
    CHECK(alpha_ref     != nullptr);

    CHECK_EQ(f_zero_int->get_value(), 0);
    CHECK_EQ(f_zero_string->get_value(), "0");

    // 2. term print
    std::cout << "     zero_int     = \""; zero_int   .print(std::cout, context_print); std::cout << "\"\n"; 
    std::cout << "     zero_string  = \""; zero_string.print(std::cout, context_print); std::cout << "\"\n"; 

    // 3. spec check
    CHECK(zero_int.get_spec() == "int");
    CHECK(zero_string.get_spec() == "int");
    CHECK(alpha.get_spec() == std::to_string(sort_int));

    CHECK(f_zero_int   ->get_sort() == sort_int);
    CHECK(f_zero_int   ->get_constructor() == c_int);
    CHECK(f_zero_string->get_sort() == sort_int);
    CHECK(f_zero_string->get_constructor() == c_int);

    size_t hash_zero_int    = t_hash()(zero_int);
    size_t hash_zero_string = t_hash()(zero_string);
    WARN_NE(hash_zero_int, hash_zero_string);

    // 4. substitution
    t_container c1_substitution({r_zero_int});

    t_substitution s1_substitution;
    if constexpr(get_pcomplexity_v<config> >= hrw::utils::parsing_complexity::SEQUENCE) {
      s1_substitution.insert(alpha_ref, c1_substitution.begin(), c1_substitution.end());
    } else {
      s1_substitution.insert(alpha_ref, c1_substitution[0]);
    }
    std::optional<t_substitution> subst1_substitution(s1_substitution);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst1_substitution, r_alpha, r_zero_int);


    t_container c2_substitution({r_zero_string});

    t_substitution s2_substitution;
    if constexpr(get_pcomplexity_v<config> >= hrw::utils::parsing_complexity::SEQUENCE) {
      s2_substitution.insert(alpha_ref, c1_substitution.begin(), c1_substitution.end());
    } else {
      s2_substitution.insert(alpha_ref, c1_substitution[0]);
    }
    std::optional<t_substitution> subst2_substitution(s2_substitution);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst2_substitution, r_alpha, r_zero_string);


    // 6. match
    std::optional<t_substitution> subst_match1 = alpha.match(r_zero_int);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match1, r_alpha, r_zero_int);

    std::optional<t_substitution> subst_match2 = alpha.match(r_zero_string);
    check_substitution_image<t_term_full_ref, t_variable, t_substitution>(subst_match1, r_alpha, r_zero_string);
  }
};


////////////////////////////////////////////////////////////////////////////////
//// WRAP UP
////////////////////////////////////////////////////////////////////////////////

using t_function_class_list = t_func_class_wrapper_list<test_free, test_leaf, test_literal>;
using test_type = test_generation<
  t_sigma_manager, sigma_manager, t_sort_resolver,
  t_make_ptr_list, t_vparser_list, t_vth_list, t_sth_list_list,
  t_function_class_list
>;

TEST_CASE("hterm - all tests") {
  std::cout << "==================================================================\n";
  std::cout << "= hterm - all tests\n";

  test_type::run();

}


#endif

