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
#if ENABLE_UTILS_TYPE_TRAITS

#include <string>
#include <vector>
#include <tuple>
#include <variant>
#include <functional>
#include <type_traits>

#include "doctest/doctest.h"

#include "hrewrite/utils/type_traits.hpp"

using namespace hrw::utils;

/////////////////////////////////////////////////////////////////////////////
// CONTEXT
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////// 
// structures

struct t1_t { using type = char; };
struct t2_t { using type  = int ; };
struct t3_t { using type  = char; };
struct t4_t { using type  = std::string; };

struct t1_t_err { using typee = char; };
struct t2_t_err { using typee  = int ; };
struct t3_t_err { using typee  = char; };
struct t4_t_err { using typee  = std::string; };

struct t1_field { typename t1_t::type f; };
struct t2_field { typename t2_t::type f; };
struct t3_field { typename t3_t::type f; };
struct t4_field { typename t4_t::type f; };

struct t1_fun { typename t1_t::type f(t1_t::type) { throw 0; } };
struct t2_fun { typename t2_t::type f(t2_t::type) { throw 0; } };
struct t3_fun { typename t3_t::type f(t3_t::type) { throw 0; } };
struct t4_fun { typename t4_t::type f(t4_t::type) { throw 0; } };

struct t1_fun_static { static typename t1_t::type f(t1_t::type) { throw 0; } };
struct t2_fun_static { static typename t2_t::type f(t2_t::type) { throw 0; } };
struct t3_fun_static { static typename t3_t::type f(t3_t::type) { throw 0; } };
struct t4_fun_static { static typename t4_t::type f(t4_t::type) { throw 0; } };

/////////////////////////////////////////// 
// template: container
template<typename ... Ts> struct C_t {};

/////////////////////////////////////////// 
// template: map and get
template<typename T> using get_type = typename T::type;

template<typename T> using get_field_t1 = decltype(std::declval<T>().f);
template<typename T> using get_field_t2 = decltype(std::declval<T>().f);
template<typename T> using get_field_t3 = decltype(std::declval<T>().f);
template<typename T> using get_field_t4 = decltype(std::declval<T>().f);

template<typename T> using get_fun_t1 = decltype(std::declval<T>().f(std::declval<typename t1_t::type>()));
template<typename T> using get_fun_t2 = decltype(std::declval<T>().f(std::declval<typename t2_t::type>()));
template<typename T> using get_fun_t3 = decltype(std::declval<T>().f(std::declval<typename t3_t::type>()));
template<typename T> using get_fun_t4 = decltype(std::declval<T>().f(std::declval<typename t4_t::type>()));

template<typename T> using get_fun_static_t1 = decltype(T::f(std::declval<typename t1_t::type>()));
template<typename T> using get_fun_static_t2 = decltype(T::f(std::declval<typename t2_t::type>()));
template<typename T> using get_fun_static_t3 = decltype(T::f(std::declval<typename t3_t::type>()));
template<typename T> using get_fun_static_t4 = decltype(T::f(std::declval<typename t4_t::type>()));

/////
template<typename T1, typename T2> using check_t = std::enable_if_t<std::is_same_v<T1, T2>>;

template<typename T> using get_field_t1_check = check_t<get_field_t1<T>, typename t1_t::type>;
template<typename T> using get_field_t2_check = check_t<get_field_t2<T>, typename t2_t::type>;
template<typename T> using get_field_t3_check = check_t<get_field_t3<T>, typename t3_t::type>;
template<typename T> using get_field_t4_check = check_t<get_field_t4<T>, typename t4_t::type>;

template<typename T> using get_fun_t1_check = check_t<get_fun_t1<T>, typename t1_t::type>;
template<typename T> using get_fun_t2_check = check_t<get_fun_t2<T>, typename t2_t::type>;
template<typename T> using get_fun_t3_check = check_t<get_fun_t3<T>, typename t3_t::type>;
template<typename T> using get_fun_t4_check = check_t<get_fun_t4<T>, typename t4_t::type>;

template<typename T> using get_fun_static_t1_check = check_t<get_fun_static_t1<T>, typename t1_t::type>;
template<typename T> using get_fun_static_t2_check = check_t<get_fun_static_t2<T>, typename t2_t::type>;
template<typename T> using get_fun_static_t3_check = check_t<get_fun_static_t3<T>, typename t3_t::type>;
template<typename T> using get_fun_static_t4_check = check_t<get_fun_static_t4<T>, typename t4_t::type>;


/////////////////////////////////////////////////////////////////////////////
// TEST: TUPLE MANIPULATION
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////// 
// template: check => set of integrals
template<typename T, typename U> struct my_check;
template<typename T> struct my_check<T, std::tuple<>>: public std::is_integral<T> {};
template<typename T, typename TT, typename ... Ts> struct my_check<T, std::tuple<TT, Ts...>>:
  public std::conjunction<std::is_integral<T>, tuple_ncontains<T, std::tuple<TT, Ts...>>, my_check<T, std::tuple<Ts...>>> {};

using t1 = t1_t;
using t2 = t2_t;
using t3 = t3_t;
using t4 = t4_t;


TEST_CASE("type_traits_tuple") {

  /////////////////////////////////////////// 
  // fold_left
  CHECK(std::is_same_v<tuple_fold_left_t<C_t, void, std::tuple<t1, t2>>, C_t<C_t<void, t1>, t2> >);
  CHECK(std::is_same_v<tuple_fold_left_t<C_t, void, std::tuple<t3, t2, t1>>, C_t<C_t<C_t<void, t3>, t2>, t1> >);
  CHECK(std::is_same_v<tuple_fold_left_t<C_t, void, std::tuple<t1, t4, t3, t2>>, C_t<C_t<C_t<C_t<void, t1>, t4>, t3>, t2> >);

  ///////////////////////////////////////////
  // fold_right
  CHECK(std::is_same_v<tuple_fold_right_t<C_t, std::tuple<t1, t2>, void>, C_t<t1, C_t<t2, void>> >);
  CHECK(std::is_same_v<tuple_fold_right_t<C_t, std::tuple<t3, t2, t1>, void>, C_t<t3, C_t<t2, C_t<t1, void>>> >);
  CHECK(std::is_same_v<tuple_fold_right_t<C_t, std::tuple<t1, t4, t3, t2>, void>, C_t<t1, C_t<t4, C_t<t3, C_t<t2, void>>>> >);

  ///////////////////////////////////////////
  // map
  CHECK(std::is_same_v<tuple_map_t<get_type, std::tuple<t1, t2>>, std::tuple<typename t1::type, typename t2::type> >);
  CHECK(std::is_same_v<tuple_map_t<get_type, std::tuple<t3, t2, t1>>, std::tuple<typename t3::type, typename t2::type, typename t1::type> >);
  CHECK(std::is_same_v<tuple_map_t<get_type, std::tuple<t1, t4, t3, t2>>, std::tuple<typename t1::type, typename t4::type, typename t3::type, typename t2::type> >);

  //////////////////////////////////////////
  //  contains
  CHECK(tuple_contains_v<t1, std::tuple<t1, t2, t3>>);
  CHECK(tuple_contains_v<t2, std::tuple<t1, t2, t3>>);
  CHECK(tuple_contains_v<t3, std::tuple<t1, t2, t3>>);

  CHECK_FALSE(tuple_contains_v<t1, std::tuple<t2, t3>>);
  CHECK_FALSE(tuple_contains_v<t2, std::tuple<t1, t3>>);
  CHECK_FALSE(tuple_contains_v<t3, std::tuple<t1, t2>>);

  ///////////////////////////////////////////
  // not contains
  CHECK(tuple_ncontains_v<t1, std::tuple<t2, t3>>);
  CHECK(tuple_ncontains_v<t2, std::tuple<t1, t3>>);
  CHECK(tuple_ncontains_v<t3, std::tuple<t1, t2>>);

  CHECK_FALSE(tuple_ncontains_v<t1, std::tuple<t1, t2, t3>>);
  CHECK_FALSE(tuple_ncontains_v<t2, std::tuple<t1, t2, t3>>);
  CHECK_FALSE(tuple_ncontains_v<t3, std::tuple<t1, t2, t3>>);

  //////////////////////////////////////////
  // subset
  CHECK(tuple_subset_v<std::tuple<t1>, std::tuple<t1, t2, t3>>);
  CHECK(tuple_subset_v<std::tuple<t1, t2>, std::tuple<t1, t2, t3>>);
  CHECK(tuple_subset_v<std::tuple<t1, t3>, std::tuple<t1, t2, t3>>);

  CHECK_FALSE(tuple_contains_v<std::tuple<t1>, std::tuple<t2, t3>>);
  CHECK_FALSE(tuple_contains_v<std::tuple<t1, t2>, std::tuple<t1, t3>>);
  CHECK_FALSE(tuple_contains_v<std::tuple<t1, t3>, std::tuple<t1, t2>>);

  ///////////////////////////////////////////
  // index
  CHECK(tuple_index_v<t1, std::tuple<t1, t2>>         == 0);
  CHECK(tuple_index_v<t2, std::tuple<t3, t2, t1>>     == 1);
  CHECK(tuple_index_v<t3, std::tuple<t1, t4, t3, t2>> == 2);
  CHECK(tuple_index_v<t4, std::tuple<t1, t4, t3, t2>> == 1);
  CHECK(tuple_index_v<t2, std::tuple<t1, t4, t3, t2>> == 3);

  //////////////////////////////////////////
  //  3. concat
  CHECK(std::is_same_v<std::tuple<t1, t1, t1>, tuple_concat_t<std::tuple<t1>, std::tuple<t1>, std::tuple<t1>>>);
  CHECK(std::is_same_v<std::tuple<t1, t1, t1>, tuple_concat_t<std::tuple<t1>, std::tuple<t1, t1>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2, t1>, tuple_concat_t<std::tuple<t1>, std::tuple<t2>, std::tuple<t1>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2, t1>, tuple_concat_t<std::tuple<t1>, std::tuple<t2, t1>>>);

  ///////////////////////////////////////////
  // add
  CHECK(std::is_same_v<std::tuple<t1, t1, t2>, tuple_add_t<t1, std::tuple<t1, t2>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2, t1>, tuple_add_t<t1, std::tuple<t2, t1>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2, t3>, tuple_add_t<t1, std::tuple<t2, t3>>>);

  ///////////////////////////////////////////
  // add_if
  CHECK(std::is_same_v<std::tuple<t1, t1, t2>, tuple_add_if_t<tuple_contains, t1, std::tuple<t1, t2>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2, t1>, tuple_add_if_t<tuple_contains, t1, std::tuple<t2, t1>>>);
  CHECK(std::is_same_v<std::tuple<t2, t3>, tuple_add_if_t<tuple_contains, t1, std::tuple<t2, t3>>>);

  ///////////////////////////////////////////
  // remove all
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_remove_all_t<std::tuple<t3, t4>, std::tuple<t1, t2>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_remove_all_t<std::tuple<t3, t4>, std::tuple<t1, t2, t3>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_remove_all_t<std::tuple<t3, t4>, std::tuple<t1, t3, t2>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_remove_all_t<std::tuple<t3, t4>, std::tuple<t1, t3, t4, t2>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_remove_all_t<std::tuple<t3, t4>, std::tuple<t1, t4, t3, t2>>>);

  ///////////////////////////////////////////
  // remove
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_remove_t<t3, std::tuple<t1, t2>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_remove_t<t3, std::tuple<t1, t2, t3>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_remove_t<t3, std::tuple<t1, t3, t2>>>);

  //////////////////////////////////////////
  //  filter
  CHECK(std::is_same_v<std::tuple<int, char>, tuple_filter_t<my_check, std::tuple<int, t1, char>>>);
  CHECK(std::is_same_v<std::tuple<char, int>, tuple_filter_t<my_check, std::tuple<int, char, int>>>);
  CHECK(std::is_same_v<std::tuple<char, int>, tuple_filter_t<my_check, std::tuple<int, char, int, t1>>>);
  CHECK_FALSE(std::is_same_v<std::tuple<int, char>, tuple_filter_t<my_check, std::tuple<int, char, int>>>);

  ///////////////////////////////////////////
  // element filter
  CHECK(std::is_same_v<std::tuple<int, char>, tuple_elfilter_t<std::is_integral, std::tuple<int, t1, char>>>);
  CHECK(std::is_same_v<std::tuple<int, char, int>, tuple_elfilter_t<std::is_integral, std::tuple<int, char, int>>>);
  CHECK_FALSE(std::is_same_v<std::tuple<int, char>, tuple_elfilter_t<std::is_integral, std::tuple<int, char, int>>>);

  ///////////////////////////////////////////
  // all
  CHECK(tuple_all_v<my_check, std::tuple<int, char>>);
  CHECK(tuple_all_v<my_check, std::tuple<int, char, short>>);
  CHECK_FALSE(tuple_all_v<my_check, std::tuple<int, char, float>>);
  CHECK_FALSE(tuple_all_v<my_check, std::tuple<int, char, int, t1>>);
  CHECK_FALSE(tuple_all_v<my_check, std::tuple<int, t1, char>>);
  CHECK_FALSE(tuple_all_v<my_check, std::tuple<int, char, int>>);
  CHECK_FALSE(tuple_all_v<my_check, std::tuple<int, char, int, char>>);

  ///////////////////////////////////////////
  // element all
  CHECK(tuple_elall_v<std::is_integral, std::tuple<int, char>>);
  CHECK(tuple_elall_v<std::is_integral, std::tuple<int, char, short>>);
  CHECK_FALSE(tuple_elall_v<std::is_integral, std::tuple<int, char, float>>);
  CHECK_FALSE(tuple_elall_v<std::is_integral, std::tuple<int, char, int, t1>>);
  CHECK_FALSE(tuple_elall_v<std::is_integral, std::tuple<int, t1, char>>);

  ///////////////////////////////////////////
  // any

  ///////////////////////////////////////////
  // element any
  CHECK(tuple_elany_v<std::is_integral, std::tuple<int, char>>);
  CHECK(tuple_elany_v<std::is_integral, std::tuple<int, char, short>>);
  CHECK(tuple_elany_v<std::is_integral, std::tuple<int, char, float>>);
  CHECK(tuple_elany_v<std::is_integral, std::tuple<int, char, int, t1>>);
  CHECK(tuple_elany_v<std::is_integral, std::tuple<int, t1, char>>);
  CHECK_FALSE(tuple_elany_v<std::is_integral, std::tuple<t1>>);
  CHECK_FALSE(tuple_elany_v<std::is_integral, std::tuple<t1, t2, t3, t4>>);

  //////////////////////////////////////////
  // to set
  CHECK(std::is_same_v<std::tuple<t1>, tuple_toset_t<std::tuple<t1, t1, t1>>>);
  CHECK(std::is_same_v<std::tuple<t1, t2>, tuple_toset_t<std::tuple<t1, t1, t2>>>);
  CHECK(std::is_same_v<std::tuple<t2, t1>, tuple_toset_t<std::tuple<t1, t2, t1>>>);

  CHECK_FALSE(std::is_same_v<std::tuple<t1>, tuple_toset_t<std::tuple<t1, t2, t1>>>);
  CHECK_FALSE(std::is_same_v<std::tuple<t1, t2>, tuple_toset_t<std::tuple<t1, t2, t1>>>);

  ///////////////////////////////////////////
  // is set
  CHECK(tuple_isset_v<std::tuple<int, float, char, std::string>>);
  CHECK_FALSE(tuple_isset_v<std::tuple<int, float, char, int>>);

  ///////////////////////////////////////////
  // convert
  CHECK(std::is_same_v<tuple_convert_t<C_t, std::tuple<t1, t2>>, C_t<t1, t2> >);
  CHECK(std::is_same_v<tuple_convert_t<C_t, std::tuple<t3, t2, t1>>, C_t<t3, t2, t1> >);
  CHECK(std::is_same_v<tuple_convert_t<C_t, std::tuple<t1, t4, t3, t2>>, C_t<t1, t4, t3, t2> >);

  ///////////////////////////////////////////
  // product
  CHECK(std::is_same_v<tuple_product_t<std::tuple<t1, t2>, std::tuple<t3, t4>>, std::tuple<
    std::tuple<t1, t3>, std::tuple<t1, t4>,
    std::tuple<t2, t3>, std::tuple<t2, t4>
  >>);
  CHECK(std::is_same_v<tuple_product_t<std::tuple<t1, t2>, std::tuple<t3, t4>, std::tuple<t1, t4>>, std::tuple<
    std::tuple<t1, t3, t1>, std::tuple<t1, t3, t4>, std::tuple<t1, t4, t1>, std::tuple<t1, t4, t4>,
    std::tuple<t2, t3, t1>, std::tuple<t2, t3, t4>, std::tuple<t2, t4, t1>, std::tuple<t2, t4, t4>
  >>);

}


/////////////////////////////////////////////////////////////////////////////
// TEST: TYPE LIST MANIPULATION
/////////////////////////////////////////////////////////////////////////////

TEST_CASE("type_traits_typelist") {

  ///////////////////////////////////////////
  // contains
  CHECK(typelist_contains_v<t1, t1, t2, t3>);
  CHECK(typelist_contains_v<t2, t1, t2, t3>);
  CHECK(typelist_contains_v<t3, t1, t2, t3>);

  CHECK_FALSE(typelist_contains_v<t1, t2, t3>);
  CHECK_FALSE(typelist_contains_v<t2, t1, t3>);
  CHECK_FALSE(typelist_contains_v<t3, t1, t2>);

  ///////////////////////////////////////////
  // not contains
  CHECK(typelist_ncontains_v<t1, t2, t3>);
  CHECK(typelist_ncontains_v<t2, t1, t3>);
  CHECK(typelist_ncontains_v<t3, t1, t2>);

  CHECK_FALSE(typelist_ncontains_v<t1, t1, t2, t3>);
  CHECK_FALSE(typelist_ncontains_v<t2, t1, t2, t3>);
  CHECK_FALSE(typelist_ncontains_v<t3, t1, t2, t3>);

  ///////////////////////////////////////////
  // to set
  CHECK(std::is_same_v<C_t<t1>, typelist_toset_t<C_t, t1, t1, t1>>);
  CHECK(std::is_same_v<C_t<t1, t2>, typelist_toset_t<C_t, t1, t1, t2>>);
  CHECK(std::is_same_v<C_t<t2, t1>, typelist_toset_t<C_t, t1, t2, t1>>);

  CHECK_FALSE(std::is_same_v<C_t<t1>, typelist_toset_t<C_t, t1, t2, t1>>);
  CHECK_FALSE(std::is_same_v<C_t<t1, t2>, typelist_toset_t<C_t, t1, t2, t1>>);

  ///////////////////////////////////////////
  // is set
  CHECK(typelist_isset_v<int, float, char, std::string>);
  CHECK_FALSE(typelist_isset_v<int, float, char, int>);

  ///////////////////////////////////////////
  // element filter
  CHECK(std::is_same_v<std::tuple<int, char>, typelist_elfilter_t<std::is_integral, int, t1, char>>);
  CHECK(std::is_same_v<std::tuple<int, char, int>, typelist_elfilter_t<std::is_integral, int, char, int>>);
  CHECK_FALSE(std::is_same_v<std::tuple<int, char>, typelist_elfilter_t<std::is_integral, int, char, int>>);

}

/////////////////////////////////////////////////////////////////////////////
// DEFINITION GETTER AND CHECKS
/////////////////////////////////////////////////////////////////////////////

using inner_type_type = inner_type<get_type>;

using inner_field_t1 = inner_type<get_field_t1>;
using inner_field_t2 = inner_type<get_field_t2>;
using inner_field_t3 = inner_type<get_field_t3>;
using inner_field_t4 = inner_type<get_field_t4>;

using inner_fun_t1 = inner_type<get_fun_t1>;
using inner_fun_t2 = inner_type<get_fun_t2>;
using inner_fun_t3 = inner_type<get_fun_t3>;
using inner_fun_t4 = inner_type<get_fun_t4>;

using inner_fun_static_t1 = inner_type<get_fun_static_t1>;
using inner_fun_static_t2 = inner_type<get_fun_static_t2>;
using inner_fun_static_t3 = inner_type<get_fun_static_t3>;
using inner_fun_static_t4 = inner_type<get_fun_static_t4>;

/////

using inner_field_t1_check = inner_type<get_field_t1_check>;
using inner_field_t2_check = inner_type<get_field_t2_check>;
using inner_field_t3_check = inner_type<get_field_t3_check>;
using inner_field_t4_check = inner_type<get_field_t4_check>;

using inner_fun_t1_check = inner_type<get_fun_t1_check>;
using inner_fun_t2_check = inner_type<get_fun_t2_check>;
using inner_fun_t3_check = inner_type<get_fun_t3_check>;
using inner_fun_t4_check = inner_type<get_fun_t4_check>;

using inner_fun_static_t1_check = inner_type<get_fun_static_t1_check>;
using inner_fun_static_t2_check = inner_type<get_fun_static_t2_check>;
using inner_fun_static_t3_check = inner_type<get_fun_static_t3_check>;
using inner_fun_static_t4_check = inner_type<get_fun_static_t4_check>;


TEST_CASE("type_traits_typedef") {

  //////////////////////////////////////////
  //  type

  // 1. get_t
  CHECK(std::is_same_v<inner_type_type::template get_t<t1_t>, typename t1_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_t<t2_t>, typename t2_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_t<t3_t>, typename t3_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_t<t4_t>, typename t4_t::type>);

  // 2. get_fw_const_t
  CHECK(std::is_same_v<inner_type_type::template get_fw_const_t<t1_t>, typename t1_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_fw_const_t<t2_t>, typename t2_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_fw_const_t<t3_t>, typename t3_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_fw_const_t<t4_t>, typename t4_t::type>);

  CHECK(std::is_same_v<inner_type_type::template get_fw_const_t<const t1_t>, const typename t1_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_fw_const_t<const t2_t>, const typename t2_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_fw_const_t<const t3_t>, const typename t3_t::type>);
  CHECK(std::is_same_v<inner_type_type::template get_fw_const_t<const t4_t>, const typename t4_t::type>);

  // 3. has_v
  CHECK(inner_type_type::template has_v<t1_t>);
  CHECK(inner_type_type::template has_v<t2_t>);
  CHECK(inner_type_type::template has_v<t3_t>);
  CHECK(inner_type_type::template has_v<t4_t>);

  CHECK_FALSE(inner_type_type::template has_v<t1_t_err>);
  CHECK_FALSE(inner_type_type::template has_v<t2_t_err>);
  CHECK_FALSE(inner_type_type::template has_v<t3_t_err>);
  CHECK_FALSE(inner_type_type::template has_v<t4_t_err>);

  // 4. tuple_extract_t
  CHECK(std::is_same_v<inner_type_type::template tuple_extract_t<std::tuple<t1_t, t2_t, t3_t, t4_t>>, typename t1_t::type>);
  CHECK(std::is_same_v<inner_type_type::template tuple_extract_t<std::tuple<t1_t_err, t2_t, t3_t, t4_t>>, typename t2_t::type>);
  CHECK(std::is_same_v<inner_type_type::template tuple_extract_t<std::tuple<t1_t_err, t2_t_err, t3_t, t4_t>>, typename t3_t::type>);
  CHECK(std::is_same_v<inner_type_type::template tuple_extract_t<std::tuple<t1_t_err, t2_t_err, t3_t_err, t4_t>>, typename t4_t::type>);

  // 5. dflt::get_t
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_t<t1_t>, typename t1_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_t<t2_t>, typename t2_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_t<t3_t>, typename t3_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_t<t4_t>, typename t4_t::type>);

  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_t<t1_t_err>, void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_t<t2_t_err>, void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_t<t3_t_err>, void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_t<t4_t_err>, void>);

  // 6. dflt::get_fw_const_t
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<t1_t>, typename t1_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<t2_t>, typename t2_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<t3_t>, typename t3_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<t4_t>, typename t4_t::type>);

  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<const t1_t>, const typename t1_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<const t2_t>, const typename t2_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<const t3_t>, const typename t3_t::type>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<const t4_t>, const typename t4_t::type>);

  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<t1_t_err>, void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<t2_t_err>, void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<t3_t_err>, void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<t4_t_err>, void>);

  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<const t1_t_err>, const void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<const t2_t_err>, const void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<const t3_t_err>, const void>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template get_fw_const_t<const t4_t_err>, const void>);

  // 7. dflt::tuple_extract_t
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template tuple_extract_t<std::tuple<t1_t, t2_t, t3_t, t4_t>>, t1_t>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template tuple_extract_t<std::tuple<t1_t_err, t2_t, t3_t, t4_t>>, t2_t>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template tuple_extract_t<std::tuple<t1_t_err, t2_t_err, t3_t, t4_t>>, t3_t>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template tuple_extract_t<std::tuple<t1_t_err, t2_t_err, t3_t_err, t4_t>>, t4_t>);
  CHECK(std::is_same_v<inner_type_type::template dflt<void>::template tuple_extract_t<std::tuple<t1_t_err, t2_t_err, t3_t_err, t4_t_err>>, void>);


  //////////////////////////////////////////
  //  field

  //////////////////////////////////////////
  //  function

  //////////////////////////////////////////
  //  static function

  //////////////////////////////////////////
  //  field: check

  //////////////////////////////////////////
  //  function: check

  //////////////////////////////////////////
  //  static function: check


  //////////////////////////////////////////
  // operator

}


#endif
