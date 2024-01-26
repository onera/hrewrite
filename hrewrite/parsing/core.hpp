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


#ifndef __HREWRITE_PARSING_CORE_H__
#define __HREWRITE_PARSING_CORE_H__

#include <string>
#include <algorithm>
#include <functional>
#include <bitset>
#include <cctype>

#include <iostream>


#include "hrewrite/utils.hpp"
#include "hrewrite/exceptions/parsing.hpp"

namespace hrw {
  namespace utils {


    enum parsing_complexity { ELEMENT, SEQUENCE, FULL };


    /////////////////////////////////////////////////////////////////////////////
    // 0. TYPE OF PARSER TRIGGER FUNCTION
    /////////////////////////////////////////////////////////////////////////////

    using t_parser_trigger = std::function<bool(const std::string&)>;


    template<typename T> struct t_parser_tuple_consistency {
      using t_alphabet = typename std::tuple_element_t<0, T>::t_alphabet;
      static inline constexpr const t_alphabet& alphabet = std::tuple_element_t<0, T>::alphabet;

      template<typename TT, typename> struct tmp_check:
        public std::conjunction<std::is_same<typename TT::t_alphabet, t_alphabet>, std::bool_constant<&(TT::alphabet) == &alphabet> > {};
      
      static inline constexpr bool value = hrw::utils::tuple_all_v<tmp_check, T>;

    };

    template<typename T> struct parser_tuple_complexity;
    template<> struct parser_tuple_complexity<std::tuple<>> {
      static inline constexpr parsing_complexity complexity = parsing_complexity::ELEMENT;
    };

    template<typename T, typename ... Ts> struct parser_tuple_complexity<std::tuple<T, Ts...>> {
      static inline constexpr parsing_complexity complexity = std::max(T::complexity, parser_tuple_complexity<std::tuple<Ts...>>::complexity);
    };

    template<typename T> 
    constexpr parsing_complexity parser_tuple_complexity_v = parser_tuple_complexity<T>::complexity;




    /////////////////////////////////////////////////////////////////////////////
    // 1. SPECIAL CHAR
    /////////////////////////////////////////////////////////////////////////////

    constexpr char bnfor = '|';
    constexpr char bnfstar = '*';
    constexpr char bnfplus = '+';
    constexpr char bnfopt = '?';
    constexpr char interval_begin = '[';
    constexpr char interval_end = ']';
    constexpr char interval_comma = ',';
    constexpr char bnf_begin = '(';
    constexpr char bnf_end = ')';


    inline bool is_char_special(const char c) {
      static const std::vector<char> char_specials = {
        bnfor,
        bnfstar,
        bnfplus,
        bnfopt,
        interval_begin,
        interval_end,
        interval_comma,
        bnf_begin,
        bnf_end
      };

      return (std::find(char_specials.begin(), char_specials.end(), c) != char_specials.end());
    }

    template<typename T>
    bool contains_char_special(const T& s) {
      return (std::find_if(s.begin(), s.end(), &is_char_special) != s.end());
    }



    /////////////////////////////////////////////////////////////////////////////
    // 2. SEPARATORS
    /////////////////////////////////////////////////////////////////////////////

    constexpr char space = ' ';
    constexpr char newline = '\n';
    constexpr char tab = '\t';


    inline bool is_char_separator(const char c) {
      static const std::vector<char> char_separators = {
        space, newline, tab
      };

      return (std::find(char_separators.begin(), char_separators.end(), c) != char_separators.end());
    }


    template<typename T>
    bool contains_char_separator(const T& s) {
      return (std::find_if(s.begin(), s.end(), &is_char_separator) != s.end());
    }




    /////////////////////////////////////////////////////////////////////////////
    // 3. VALIDATES REGEXP
    /////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////
    // core functions

    template<typename T>
    typename T::size_type skip_spaces(const T& s, typename T::size_type i) {
      for(; i < s.length(); ++i) {
        if(!is_char_separator(s[i])) { return i; }
      }
      return i;
    }

    template<typename T>
    typename T::size_type get_name(const T& s, typename T::size_type i) {
      for(; i < s.length(); ++i) {
        if((!std::isalnum(s[i])) && (s[i] != '_')) { return i; }
      }
      return i;
    }

    template<typename T>
    typename T::size_type get_number(const T& s, typename T::size_type i) {
      //if((i < s.length()-1) && (s[i] == '-') && (std::isdigit(s[i+1]))) { i += 2; } // <- negative numbers are not valid
      for(; i < s.length(); ++i) {
        if(!std::isdigit(s[i])) { return i; }
      }
      return i;
    }


    //////////////////////////////////////////
    // check functions

    template<typename T>
    bool is_element(const T& s) {
      using t_nat = typename T::size_type;
      t_nat start = skip_spaces(s, 0);
      t_nat end = get_name(s, start);
      if(start == end) { return false; }
      if(skip_spaces(s, end) != s.length()) { return false; }
      return true;
    }


    template<typename T>
    bool is_sequence(const T& s) {
      using t_nat = typename T::size_type;
      t_nat start = skip_spaces(s, 0);
      t_nat end = get_name(s, start);
      while(start != end) {
        start = skip_spaces(s, end);
        end = get_name(s, start);
      }
      if(end != s.length()) { return false; }
      return true;
    }



    template<typename T>
    bool is_regexp(const T& s) {
      using t_nat = typename T::size_type;
      t_nat current = 0, next;
      t_nat limit = s.length();
      t_nat nb_paren = 0;
      bool valid_group = false;

      unsigned int val1, val2;
      while(true) {
        current = skip_spaces(s, current);
        if(current == limit) {
          return (nb_paren == 0);
        } else {
          // std::cout << "reading character '" << s[current] << "'\n";
          switch(s[current]) {
            case bnfor:
              // std::cout << " -> bnfor (" << std::boolalpha << (!valid_group) << ")\n";
              if(!valid_group) { return false; }
              else { valid_group = false; ++current; }
              break;
            case bnfstar:
              // std::cout << " -> bnfstar (" << std::boolalpha << (!valid_group) << ")\n";
              if(!valid_group) { return false; }
              ++current;
              break;
            case bnfplus:
              // std::cout << " -> bnfplus (" << std::boolalpha << (!valid_group) << ")\n";
              if(!valid_group) { return false; }
              ++current;
              break;
            case bnfopt:
              // std::cout << " -> bnfopt (" << std::boolalpha << (!valid_group) << ")\n";
              if(!valid_group) { return false; }
              ++current;
              break;
            case interval_begin:
              // std::cout << " -> interval_begin (" << std::boolalpha << (!valid_group) << ")\n";
              if(!valid_group) { return false; }
              else {
                // entering interval mode
                ++current;
                next = get_number(s, current);
                if((next == current) || (next == limit)) { return false; }
                val1 = std::atoi(s.c_str() + current);
                if(s[next] == interval_comma) {
                  current = next + 1;
                  next = get_number(s, current);
                  if((next == current) || (next == limit) || (s[next] != interval_end)) { return false; }
                  val2 = std::atoi(s.c_str() + current);
                  if(val1 > val2) { return false; }
                } else if(s[next] != interval_end) { return false; }
                current = next + 1;
              }
              break;
            case interval_end:
              // std::cout << " -> interval_end (" << std::boolalpha << (!valid_group) << ")\n";
              return false;
              break;
            case interval_comma:
              // std::cout << " -> interval_comma (" << std::boolalpha << (!valid_group) << ")\n";
              return false;
              break;
            case bnf_begin:
              // std::cout << " -> bnf_begin (" << std::boolalpha << (!valid_group) << ")\n";
              valid_group = false;
              ++nb_paren;
              ++current;
              break;
            case bnf_end:
              // std::cout << " -> bnf_end (" << std::boolalpha << (!valid_group) << ")\n";
              if((!valid_group) || (nb_paren == 0)) {
                return false;
              } else {
                --nb_paren;
                ++current;
              }
              break;
            default: // should be a name
              // std::cout << " -> default (" << std::boolalpha << (!valid_group) << ")\n";
              next = get_name(s, current);
              if(next == current) { return false; }
              else { valid_group = true; current = next; }
          }
        }
      }
      return false;
    }

    template<typename T>
    bool is_regexp_reduced(const T& s) {
      if(is_regexp(s)) {
        return (std::find(s.begin(), s.end(), interval_begin) == s.end());
      }
      return false;
    }


  }
} // end namespace


#endif // __HREWRITE_PARSING_CORE_H__

