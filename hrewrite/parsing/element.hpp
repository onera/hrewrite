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


#ifndef __HREWRITE_PARSING_ELEMENT_H__
#define __HREWRITE_PARSING_ELEMENT_H__


#include <iostream>

#include "hrewrite/parsing/core.hpp"
#include "hrewrite/utils.hpp"


namespace hrw {
  namespace utils {

    template<typename targ_alphabet, const targ_alphabet& arg_alphabet>
    class element {
    public:
      using type = element<targ_alphabet, arg_alphabet>;

      using t_state = unsigned int;
      using t_state_set = hrw::utils::container_single<t_state>;

      using t_alphabet = targ_alphabet;
      using t_letter = typename t_alphabet::t_letter;
      using t_letter_set = typename t_alphabet::t_letter_set;

      static inline constexpr const targ_alphabet& alphabet = arg_alphabet;

      // constructor
      element(const std::string& s);
      ~element() = default;
      // core functionalities
      t_state default_state() const {return 0; }
      t_state start() const { return 0; }
      bool is_final(const t_state& state) const { return (state == 1); }
      bool is_error(const t_state& state) const { return (state > 1); }
      bool next(const t_letter c, const t_state& start, t_state& end) const;
      void nexts(const t_state& state, t_letter_set& set) const;

      const std::string& get_regexp() const { return this-> m_regexp; }

      // operators and friend functions
      bool operator==(const type& other) const { return this == &other; }

      friend std::ostream& operator<<(std::ostream& os, const type& parser) {
        return (os << "element[" << parser.m_content << "]");
      }

      friend void swap(type& p1, type& p2) {
        using std::swap;
        swap(p1.m_content, p2.m_content);
        swap(p1.m_regexp, p2.m_regexp);
      }

      static const t_parser_trigger trigger;
      static constexpr parsing_complexity complexity = parsing_complexity::ELEMENT;

      const t_letter& get_letter() const { return this->m_content; }

    private:
      t_letter m_content;

      std::string m_regexp;
    };



    /////////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION
    /////////////////////////////////////////////////////////////////////////////

    template<typename targ_alphabet, const targ_alphabet& alphabet>
    element<targ_alphabet, alphabet>::element(const std::string& s): m_regexp(s) {
      unsigned int begin = 0, len = 0;
      unsigned int state = 0; // 0 = before word, 1 = during word, 2 = after word
      for(const char c: s) {
        if(is_char_special(c)) {
          throw hrw::exception::spec_invalid_element(s);
        } else if(is_char_separator(c) && (state != 1)) { // normal spaces
          ++begin;
        } else if(is_char_separator(c)) { // end of a word
          this->m_content = alphabet.get_letter(s.substr(begin, len));
          begin += len;
          state = 2;
        } else if(state == 0) { // begins a word
          state = 1;
          len = 1;
        } else if(state == 1) { // in a word
          ++len;
        } else {
           throw hrw::exception::spec_invalid_element(s);
        }
      }
      if(state == 1) { // the end of the string finished a word
        this->m_content = alphabet.get_letter(s.substr(begin, len));
      } else {
        throw hrw::exception::spec_invalid_element(s);
      }
    }


    template<typename targ_alphabet, const targ_alphabet& alphabet>
    bool element<targ_alphabet, alphabet>::next(const t_letter c, const t_state& start, t_state& end) const {
      // std::cout << "element::next = (" << start << ", " << this->m_max << ", " << end << ") -> " << std::boolalpha << ((start+1) == this->m_max) << std::endl;
      if(start >= 1) {
        end = 2;
        return false;
      } else {
        if(alphabet.is_subletter(c, this->m_content)) { // matches
          end = 1;
          // std::cout << "  => matched letter " << c << ": old state=" << start << ", new state=" << end << std::endl;
          return this->is_final(end);
        } else { // error
          // std::cout << "  => error letter: expected letter=" << c_local << ", gotten letter=" << c << std::endl;
          end = 2;
          return false;
        }
      }
    }

    template<typename targ_alphabet, const targ_alphabet& alphabet>
    void element<targ_alphabet, alphabet>::nexts(const t_state& state, t_letter_set& set) const {
      set.clear();
      if(state == 0) {
        set.insert(this->m_content);
      }
    }


    template<typename targ_alphabet, const targ_alphabet& alphabet>
    const t_parser_trigger element<targ_alphabet, alphabet>::trigger = &is_element<std::string>;

  }
} // end namespace


#endif // __HREWRITE_PARSING_ELEMENT_H__

