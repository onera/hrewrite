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

#ifndef __HREWRITE_TYPE_TRAITS_H__
#define __HREWRITE_TYPE_TRAITS_H__

/***
 * This file contains many useful type manipulation templates that extend the ones available in the standard library.
 * In particular it contains:
 *  - many functional templates to manipulate std::tuple as lists (e.g., tuple_fold_left, tuple_fold_right, tuple_map, tuple_contains)
 *  - some functional templates to manipulate type lists
 *  - an helper class for variant visitors
 */
#include <type_traits>
#include <climits>

#include <tuple>

#include <string>
#include <string_view>

namespace hrw {
  namespace utils {

    /////////////////////////////////////////////////////////////////////////////
    // TUPLE MANIPULATION
    /////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////// 
    // fold_left
    // tuple_fold_left_t<Acc, Init, [T1, T2, ..., Tn]> => Acc <...<Acc <Acc <Init, T1>, T2> ...>, Tn>
    template<template<typename, typename> typename Acc, typename Init, typename L> struct tuple_fold_left;
    template<template<typename, typename> typename Acc, typename Init> struct tuple_fold_left<Acc, Init, std::tuple<>> {
      using type = Init;
    };
    template<template<typename, typename> typename Acc, typename Init, typename T, typename ... Ts>
    struct tuple_fold_left<Acc, Init, std::tuple<T, Ts...>> {
      using type = typename tuple_fold_left<Acc, Acc<Init, T>, std::tuple<Ts...>>::type;
    };
    template<template<typename, typename> typename Acc, typename Init, typename L>
    using tuple_fold_left_t = typename tuple_fold_left<Acc, Init, L>::type;


    ///////////////////////////////////////////
    // fold_right
    // tuple_fold_right_t<Acc, [T1, T2, ..., Tn], Init> =>  Acc <T1, Acc <T2, ...Acc <Tn, Init> ...>>
    template<template<typename, typename> typename Acc, typename L, typename Init> struct tuple_fold_right;
    template<template<typename, typename> typename Acc, typename Init> struct tuple_fold_right<Acc, std::tuple<>, Init> {
      using type = Init;
    };
    template<template<typename, typename> typename Acc, typename Init, typename T, typename ... Ts>
    struct tuple_fold_right<Acc, std::tuple<T, Ts...>, Init> {
      using type = Acc<T, typename tuple_fold_right<Acc, std::tuple<Ts...>, Init>::type>;
    };
    template<template<typename, typename> typename Acc, typename L, typename Init>
    using tuple_fold_right_t = typename tuple_fold_right<Acc, L, Init>::type;


    ///////////////////////////////////////////
    // map
    // tuple_map_t<F, [T1, T2, ..., Tn]> =>  [F<T1>, F<T2>, ..., F<Tn>]
    template<template<typename> typename F, typename U> struct tuple_map;
    template<template<typename> typename F, typename ... Ts> struct tuple_map<F, std::tuple<Ts...>> { using type = std::tuple<F<Ts>...>; };
    template<template<typename> typename F, typename U> using tuple_map_t = typename tuple_map<F, U>::type;


    ///////////////////////////////////////////
    // contains
    // tuple_contains_v<T, [T1, ..., Tn]> => bool: T in [T1, ..., Tn]
    template<typename T, typename U> struct tuple_contains;
    template<typename T, typename ... Ts> struct tuple_contains<T, std::tuple<Ts...>>:
      public std::integral_constant<bool, std::disjunction<std::is_same<T,Ts>...>::value> {};
    template<typename T, typename U> inline constexpr bool tuple_contains_v = tuple_contains<T, U>::value;


    ///////////////////////////////////////////
    // not contains
    // tuple_ncontains_v<T, [T1, ..., Tn]> => bool: T not in [T1, ..., Tn]
    template<typename T, typename U> using tuple_ncontains = std::negation<tuple_contains<T, U>>;
    template<typename T, typename U> inline constexpr bool tuple_ncontains_v = tuple_ncontains<T, U>::value;


    ///////////////////////////////////////////
    // subset
    // tuple_subset_v<[T1, ..., Tn], [T1', ..., Tm']> => bool: all T in [T1, ..., Tn] are also in [T1', ..., Tm']
    template<typename T, typename U> struct tuple_subset;
    template<typename ... Ts, typename T> struct tuple_subset<std::tuple<Ts...>, T>:
      public std::integral_constant<bool, std::conjunction<tuple_contains<Ts, T>...>::value> {};
    template<typename T, typename U> inline constexpr bool tuple_subset_v = tuple_subset<T, U>::value;


    ///////////////////////////////////////////
    // index
    // tuple_index_v<T, [T1, ..., Tn]> => std::size_t: i such that Ti == T
    template <typename T, typename U> struct tuple_index;
    template <typename T, typename ... Ts> struct tuple_index<T, std::tuple<T, Ts ...>>: public std::integral_constant<std::size_t, 0> {};
    template <typename T, typename U, typename ... Ts> struct tuple_index<T, std::tuple<U, Ts ...>>: public std::integral_constant<std::size_t, tuple_index<T, std::tuple<Ts...>>::value+1> {};
    template <typename T, typename U> inline constexpr std::size_t tuple_index_v = tuple_index<T, U>::value;


    ///////////////////////////////////////////
    // concat
    // tuple_concat_t<[T11, ..., T1n_1], [T21, ..., T2n_2], ... [Tm1, ..., Tmn_m]> => [T1, ..., Tn, T21, ..., T2n_2, ..., Tm1, ..., Tmn_m]
    template<typename ... Ts> struct tuple_concat;
    template<typename ... Ts> struct tuple_concat<std::tuple<Ts...>> { using type = std::tuple<Ts...>; };
    template<typename ... T1s, typename ... T2s, typename ... Rs>
    struct tuple_concat<std::tuple<T1s...>, std::tuple<T2s...>, Rs...> { using type = typename tuple_concat<std::tuple<T1s..., T2s...>, Rs...>::type; };
    template<typename ... Ts> using tuple_concat_t = typename tuple_concat<Ts...>::type;


    ///////////////////////////////////////////
    // add
    // tuple_add_t<T, [T1, ..., Tn]> => [T, T1, ..., Tn]
    template<typename T, typename U> using tuple_add = tuple_concat<std::tuple<T>, U>;
    template<typename T, typename U> using tuple_add_t = typename tuple_add<T, U>::type;


    ///////////////////////////////////////////
    // add_if
    // tuple_add_if_t<F, T, [T1, ..., Tn]> => (F<T, [T1, ..., Tn]>)?([T, T1, ..., Tn]):([T1, ..., Tn])
    template<template<typename mT, typename mU> typename F, typename T, typename U, bool b=F<T,U>::value> struct tuple_add_if;
    template<template<typename mT, typename mU> typename F, typename T, typename U> struct tuple_add_if<F,T,U,true> { using type = typename tuple_add<T, U>::type; };
    template<template<typename mT, typename mU> typename F, typename T, typename U> struct tuple_add_if<F,T,U,false> { using type = U; };
    template<template<typename mT, typename mU> typename F, typename T, typename U> using tuple_add_if_t = typename tuple_add_if<F, T, U>::type;


    ///////////////////////////////////////////
    // filter
    // tuple_filter_t<F, [T1, ..., Tn]> => the list of all Ti such that F<Ti, [Ti+1, ... Tn]>
    template<template<typename mT, typename mU> typename F, typename U> struct tuple_filter;
    template<template<typename mT, typename mU> typename F> struct tuple_filter<F, std::tuple<>> { using type = std::tuple<>; };
    template<template<typename mT, typename mU> typename F, typename T, typename ... Ts> struct tuple_filter<F, std::tuple<T,Ts...>>
      { using type = tuple_add_if_t<F, T, typename tuple_filter<F, std::tuple<Ts...>>::type>; };
    template<template<typename mT, typename mU> typename F, typename U> using tuple_filter_t = typename tuple_filter<F, U>::type;


    ///////////////////////////////////////////
    // element filter
    // tuple_elfilter_t<F, [T1, ..., Tn]> => the list of all Ti such that F<Ti>
    template<template<typename> typename F, typename T> struct tuple_elfilter {
      template<typename mT, typename mU> using mF = F<mT>;
      using type = typename tuple_filter<mF, T>::type;
    };
    template<template<typename> typename F, typename T> using tuple_elfilter_t = typename tuple_elfilter<F, T>::type;


    ///////////////////////////////////////////
    // remove all
    // tuple_remove_all_t<[T1, ..., Tn], [T1', ..., Tm']> => the list of all Ti' such that Ti' not in [T1, ..., Tn]
    template<typename T1, typename T2> struct tuple_remove_all;
    template<typename ... T1s, typename ... T2s> struct tuple_remove_all<std::tuple<T1s...>, std::tuple<T2s...>> {
      template<typename T> using F = tuple_ncontains<T, std::tuple<T1s...>>;
      using type = tuple_elfilter_t<F, std::tuple<T2s...>>;
    };
    template<typename T1, typename T2> using tuple_remove_all_t = typename tuple_remove_all<T1, T2>::type;


    ///////////////////////////////////////////
    // remove
    // tuple_remove_t<T, [T1, ..., Tn]> => the list of all Ti' such that Ti' is different from T
    template<typename T1, typename T2> struct tuple_remove;
    template<typename T, typename ... Ts> struct tuple_remove<T, std::tuple<Ts...>> {
      template<typename TT> using F = std::negation<std::is_same<T, TT>>;
      using type = tuple_elfilter_t<F, std::tuple<Ts...>>;
    };
    template<typename T1, typename T2> using tuple_remove_t = typename tuple_remove<T1, T2>::type;


    ///////////////////////////////////////////
    // all
    // tuple_all_v<F, [T1, ..., Tn]> => bool: if for all 1 <= i <= n, we have F<Ti, [Ti+1, ..., Tn]>
    template<template<typename mT, typename mU> typename C, typename U> struct tuple_all;
    template<template<typename mT, typename mU> typename C> struct tuple_all<C, std::tuple<>>: public std::true_type {};
    template<template<typename mT, typename mU> typename C, typename T, typename ... Ts> struct tuple_all<C, std::tuple<T,Ts...>> :
      public std::conjunction<C<T, std::tuple<Ts...>>, tuple_all<C, std::tuple<Ts...>>> {};
    template<template<typename mT, typename mU> typename C, typename U> inline constexpr bool tuple_all_v = tuple_all<C, U>::value;


    ///////////////////////////////////////////
    // element all
    // tuple_elall_v<F, [T1, ..., Tn]> => bool: if for all 1 <= i <= n, we have F<Ti>
    template<template<typename> typename F, typename T> struct tuple_elall {
      template<typename A, typename> using FF = F<A>;
      static constexpr bool value = tuple_all_v<FF, T>;
    };
    template<template<typename> typename F, typename T> inline constexpr bool tuple_elall_v = tuple_elall<F, T>::value;


    ///////////////////////////////////////////
    // any
    // tuple_any_v<F, [T1, ..., Tn]> => bool: if there exists 1 <= i <= n with F<Ti, [Ti+1, ..., Tn]>
    template<template<typename mT, typename mU> typename C, typename U> struct tuple_any;
    template<template<typename mT, typename mU> typename C> struct tuple_any<C, std::tuple<>>: public std::false_type {};
    template<template<typename mT, typename mU> typename C, typename T, typename ... Ts> struct tuple_any<C, std::tuple<T,Ts...>> :
      public std::disjunction<C<T, std::tuple<Ts...>>, tuple_any<C, std::tuple<Ts...>>> {};
    template<template<typename mT, typename mU> typename C, typename U> inline constexpr bool tuple_any_v = tuple_any<C, U>::value;


    ///////////////////////////////////////////
    // element any
    // tuple_elany_v<F, [T1, ..., Tn]> => bool: if there exists 1 <= i <= n with F<Ti>
    template<template<typename> typename F, typename T> struct tuple_elany {
      template<typename A, typename> using FF = F<A>;
      static constexpr bool value = tuple_any_v<FF, T>;
    };
    template<template<typename> typename F, typename T> inline constexpr bool tuple_elany_v = tuple_elany<F, T>::value;


    ///////////////////////////////////////////
    // to set
    // tuple_toset_t<[T1, ..., Tn]> => [T1, ..., Tn] without duplicates
    template<typename T> using tuple_toset = tuple_filter<tuple_ncontains, T>;
    template<typename T> using tuple_toset_t = typename tuple_toset<T>::type;


    ///////////////////////////////////////////
    // is set
    // tuple_isset_v<[T1, ..., Tn]> => bool: iff [T1, ..., Tn] contains no duplicates
    template<typename T> using tuple_isset = tuple_all<tuple_ncontains, T>;
    template<typename T> inline constexpr bool tuple_isset_v = tuple_isset<T>::value;


    ///////////////////////////////////////////
    // convert
    // tuple_convert_t<F, [T1, ..., Tn]> => F<T1, ..., Tn>
    template<template<typename ...> typename container, typename T> struct tuple_convert;
    template<template<typename ...> typename container, typename ... Ts> struct tuple_convert<container, std::tuple<Ts...>> { using type = container<Ts...>; };
    template<template<typename ...> typename container, typename T> using tuple_convert_t = typename tuple_convert<container, T>::type;


    ///////////////////////////////////////////
    // product
    // tuple_product_t<[T11, ..., T1n_1], [T21, ..., T2n_2], ... [Tm1, ..., Tmn_m]> =>
    //   [[T11, T21, ..., Tm1], [T11, T22, T31, ..., Tm1], ..., Tn, [T1n_1, T2n_2, ... Tmn_m]]
    template<typename ... Ts> struct tuple_product;
    template<typename T> struct tuple_product<T> {
      template<typename V> using F = std::tuple<V>;
      using type = tuple_map_t<F, T>;
    };
    template<typename T, typename ... Ts> struct tuple_product<T, Ts...> {
      using _TMP = typename tuple_product<Ts...>::type;
      
      template<typename V, typename R> struct S1 { // V is an element of T, S is the result list
        template<typename VV, typename RR> struct S2 { // VV is an element of _TMP
          using type = tuple_add_t<tuple_add_t<V, VV>, RR>;
        };
        template<typename VV, typename RR> using Acc = typename S2<VV, RR>::type;
        using type = tuple_fold_right_t<Acc, _TMP, R>;
      };
      template<typename V, typename R> using Acc = typename S1<V, R>::type;
      using type = tuple_fold_right_t<Acc, T, std::tuple<>>;
    };
    template<typename ... Ts> using tuple_product_t = typename tuple_product<Ts...>::type;



    /////////////////////////////////////////////////////////////////////////////
    // TYPE LIST MANIPULATION
    /////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////
    // contains
    // typelist_contains_v<T, T1, ..., Tn> => bool: T in [T1, ..., Tn]
    template<typename T, typename ... Ts> using typelist_contains = tuple_contains<T, std::tuple<Ts...>>;
    template<typename T, typename ... Ts> inline constexpr bool typelist_contains_v = typelist_contains<T, Ts...>::value;


    ///////////////////////////////////////////
    // not contains
    // typelist_ncontains_v<T, T1, ..., Tn> => bool: T not in [T1, ..., Tn]
    template<typename T, typename ... Ts> using typelist_ncontains = tuple_ncontains<T, std::tuple<Ts...>>;
    template<typename T, typename ... Ts> inline constexpr bool typelist_ncontains_v = typelist_ncontains<T, Ts...>::value;

    ///////////////////////////////////////////
    // to set
    // tuple_toset_t<F, T1, ..., Tn> => F<T1, ..., Tn> without duplicates
    template<template<typename...> typename W, typename ... Ts> using typelist_toset = tuple_convert<W, typename tuple_toset<std::tuple<Ts...>>::type>;
    template<template<typename...> typename W, typename ... Ts> using typelist_toset_t = typename typelist_toset<W, Ts...>::type;


    ///////////////////////////////////////////
    // is set
    // typelist_isset_v<T1, ..., Tn> => bool: iff [T1, ..., Tn] contains no duplicates
    template<typename ... Ts> using typelist_isset = tuple_isset<std::tuple<Ts...>>;
    template<typename ... Ts> inline constexpr bool typelist_isset_v = typelist_isset<Ts...>::value;


    ///////////////////////////////////////////
    // element filter
    // tuple_elfilter_t<F, T1, ..., Tn> => the list of all Ti such that F<Ti>
    template<template<typename> typename F, typename ... Ts> using typelist_elfilter = tuple_elfilter<F, std::tuple<Ts...>>;
    template<template<typename> typename F, typename ... Ts> using typelist_elfilter_t = typename typelist_elfilter<F, Ts...>::type;



    /////////////////////////////////////////////////////////////////////////////
    // DEFINITION GETTER AND CHECKS
    /////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////
    // getters and checks around a inner type getter
    // parameter _get: a template accessing the subtype
    // example: template<typename T> using _get = T::inner_type
    template<template<typename> typename _get>
    struct inner_type {
      ///////////////////////////////////////////
      // get
      // inner_type<_get>::get_t<C> => C::inner_type
      template<typename T> using get_t = _get<T>;

      ///////////////////////////////////////////
      // get with const forward
      // inner_type<_get>::get_fw_const_t<C> => (C is const)?(const C::inner_type):(C::inner_type)
      template<typename T> using get_fw_const_t = std::conditional_t<std::is_const<T>::value, std::add_const_t<get_t<T>>, get_t<T>>;
      
      ///////////////////////////////////////////
      // has
      // inner_type<_get>::has_v<C> => bool: if C::inner_type exists and is a type
      template<typename T, typename=void> struct _has: public std::false_type {};
      template<typename T> struct _has<T, std::void_t<get_t<T>>>: public std::true_type {};
      template<typename T> struct has: public _has<T> {};
      template<typename T> static constexpr bool has_v = has<T>::value;

      ///////////////////////////////////////////
      // extract
      // inner_type<_get>::tuple_extract_t<[T1, ..., Tn]> => returns the first Ti such that Ti::inner_type exists and is a type
      template<typename T> using tuple_extract_t = get_t<std::tuple_element_t<0, tuple_elfilter_t<has, T>>>;

      template<typename D>
      struct dflt {
        ///////////////////////////////////////////
        // get
        // inner_type<_get>::dflt::get_t<C> => (C has inner_type)?(C::inner_type):(D)
        template<typename T, bool=has_v<T>> struct get;
        template<typename T> struct get<T, false> { using type = D; };
        template<typename T> struct get<T, true>  { using type = _get<T>; };
        template<typename T> using get_t = typename get<T>::type;

        ///////////////////////////////////////////
        // get with const forward
        // inner_type<_get>::dflt::get_fw_const_t<C> => (C is const)?(const C::inner_type):(C::inner_type)
        template<typename T> using get_fw_const_t = std::conditional_t<std::is_const<T>::value, std::add_const_t<get_t<T>>, get_t<T>>;


        ///////////////////////////////////////////
        // extract
        // inner_type<_get>::dflt::tuple_extract_t<[T1, ..., Tn]> => returns the first Ti such that Ti::inner_type exists and is a type, or D if there are no such Ti
        template<typename T> struct tuple_extract {
          using tmp_t = tuple_elfilter_t<has, T>;
          template<typename TT, bool=std::is_same_v<TT, std::tuple<>>> struct tmp;
          template<typename TT> struct tmp<TT, true> { using type = D; };
          template<typename TT> struct tmp<TT, false> { using type = std::tuple_element_t<0, TT>; };
          using type = typename tmp<tmp_t>::type;
        };
        template<typename T> using tuple_extract_t = typename tuple_extract<T>::type;
      };
    };


    // has operators, few examples
    struct has_operator {
      template<typename T1, typename T2> using _left_shift = decltype(std::declval<T1>() << std::declval<T2>());
      template<typename T1, typename T2> using _assign = decltype(std::declval<T1>() = std::declval<T2>());
      template<typename T1, typename ... T2> using _call = decltype(std::declval<T1>()(std::declval<T2>() ...));
      template<typename T1, typename T2> using _equality = decltype(std::declval<T1>() == std::declval<T2>());

      template<template<typename...> typename op, typename TR, typename T, typename TP, typename=void> struct _has_operator: std::false_type {};
      template<template<typename...> typename op, typename TR, typename T, typename ... TPs> struct _has_operator<op, TR, T, std::tuple<TPs...>, std::void_t<op<T, TPs...>>>: public std::is_same<op<T, TPs...>, TR> {};


      template<typename t_res, typename t_main, typename ... t_args>
      static constexpr bool left_shift_v = _has_operator<_left_shift, t_res, t_main, std::tuple<t_args...>>::value;

      template<typename t_res, typename t_main, typename ... t_args>
      static constexpr bool assign_v = _has_operator<_assign, t_res, t_main, std::tuple<t_args...>>::value;

      template<typename t_res, typename t_main, typename ... t_args>
      static constexpr bool call_v = _has_operator<_call, t_res, t_main, std::tuple<t_args...>>::value;

      template<typename t_res, typename t_main, typename ... t_args>
      static constexpr bool equality_v = _has_operator<_equality, t_res, t_main, std::tuple<t_args...>>::value;
    };



    /////////////////////////////////////////////////////////////////////////////
    // CHECKS RANDOM PROPERTIES ON TYPES
    /////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////
    // template instance
    // is_template_instance_v<T, TT> => bool: if T is an instance of TT
    template <typename T, template <typename...> typename TT> struct is_template_instance : public std::false_type {};
    template <typename ... Args, template <typename ...> typename T>  struct is_template_instance<T<Args...>, T> : public std::true_type {};
    template <typename T, template <typename...> typename TT> constexpr bool is_template_instance_v = is_template_instance<T, TT>::value; 



    /////////////////////////////////////////////////////////////////////////////
    // TYPE SIZE MANIPULATION
    /////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////
    // nat to type
    // nat_to_type_t<size> => integer type T such that sizeof(T) is minimal with size <= sizeof(T)
    template<std::size_t size, typename=void> struct nat_to_type;

    template<std::size_t size> struct nat_to_type<size, std::enable_if_t<(ULONG_MAX < size) && (size <= ULLONG_MAX), void>> {
      using type = unsigned long long;
      static constexpr type limit = ULLONG_MAX;
    };
    template<std::size_t size> struct nat_to_type<size, std::enable_if_t<(UINT_MAX < size) && (size <= ULONG_MAX), void>> {
      using type = unsigned long;
      static constexpr type limit = ULONG_MAX;
    };
    template<std::size_t size> struct nat_to_type<size, std::enable_if_t<(USHRT_MAX < size) && (size <= UINT_MAX), void>> {
      using type = unsigned int;
      static constexpr type limit = UINT_MAX;
    };
    template<std::size_t size> struct nat_to_type<size, std::enable_if_t<(UCHAR_MAX < size) && (size <= USHRT_MAX), void>> {
      using type = unsigned short;
      static constexpr type limit = USHRT_MAX;
    };
    template<std::size_t size> struct nat_to_type<size, std::enable_if_t<size <= UCHAR_MAX, void>> {
      using type = unsigned char;
      static constexpr type limit = UCHAR_MAX;
    };
    template<std::size_t size> using nat_to_type_t = typename nat_to_type<size>::type;



    template<std::size_t N, typename T=unsigned char, std::size_t res=1, bool cond=((sizeof(T)*8*res) >= N)> struct size_to_type;
    template<std::size_t N> struct size_to_type<N, unsigned char, 1, true> {
      using type = unsigned char;
      using value_type = nat_to_type_t<1>;
      static constexpr value_type value = 1;
    };
    template<std::size_t N> struct size_to_type<N, unsigned char, 1, false>: public size_to_type<N, unsigned short, 1> {};
    template<std::size_t N> struct size_to_type<N, unsigned short, 1, true> {
      using type = unsigned short;
      using value_type = nat_to_type_t<1>;
      static constexpr value_type value = 1;
    };
    template<std::size_t N> struct size_to_type<N, unsigned short, 1, false>: public  size_to_type<N, unsigned int, 1> {};
    template<std::size_t N> struct size_to_type<N, unsigned int, 1, true> {
      using type = unsigned int;
      using value_type = nat_to_type_t<1>;
      static constexpr value_type value = 1;
    };
    template<std::size_t N> struct size_to_type<N, unsigned int, 1, false>: public size_to_type<N, unsigned long, 1> {};
    template<std::size_t N> struct size_to_type<N, unsigned long, 1, true> {
      using type = unsigned long;
      using value_type = nat_to_type_t<1>;
      static constexpr value_type value = 1;
    };
    template<std::size_t N> struct size_to_type<N, unsigned long, 1, false>: public size_to_type<N, unsigned long long, 1> {};
    template<std::size_t N, std::size_t res> struct size_to_type<N, unsigned long long, res, true> {
      using type = unsigned long long;
      using value_type = nat_to_type_t<res>;
      static constexpr value_type value = res;
    };
    template<std::size_t N, std::size_t res> struct size_to_type<N, unsigned long long, res, false>: public size_to_type<N, unsigned long long, res+1> {};

    template<typename T, std::size_t res=0, bool cond=((sizeof(T)*8) <= (1 << res))> struct type_as_bits;
    template<typename T, std::size_t res> struct type_as_bits<T, res, false>: public type_as_bits<T, res+1> {};
    template<typename T, std::size_t res> struct type_as_bits<T, res, true> {
      template<typename P, typename I> struct make {
        using t_cell = T;
        using t_nat = P;
        using t_cell_index = I;
        using t_inner_index = typename nat_to_type<sizeof(T)*8-1>::type;

        static constexpr t_inner_index l_cell = sizeof(T)*8-1;
        static constexpr std::size_t s_cell = sizeof(T)*8;

        static P left_shift(t_cell_index i) { return i << res; }
        static t_cell_index right_shift(t_nat i) { return i >> res; }
        static t_inner_index get_inner_index(t_nat i) { return (static_cast<t_inner_index>(i) & l_cell); }
        static t_cell get_mask(t_nat i) { return (t_cell(1) << get_inner_index(i)); }
      };
    };

  }
}


#endif // __HREWRITE_TYPE_TRAITS_H__

