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


#ifndef __HREWRITE_PARSING_SEQUENCE_H__
#define __HREWRITE_PARSING_SEQUENCE_H__

#include <vector>
#include <string>
#include <iostream>

#include "hrewrite/parsing/core.hpp"
#include "hrewrite/utils.hpp"


namespace hrw {
  namespace utils {

    template<typename targ_alphabet, const targ_alphabet& arg_alphabet>
    class sequence {
    public:
      using type = sequence<targ_alphabet, arg_alphabet>;

      using t_state = unsigned int;
      using t_state_set = hrw::utils::container_single<t_state>;

      using t_alphabet = targ_alphabet;
      using t_letter = typename t_alphabet::t_letter;
      using t_letter_set = typename t_alphabet::t_letter_set;

      static inline constexpr const targ_alphabet& alphabet = arg_alphabet;

      // constructor
      sequence(const std::string& s);
      ~sequence() = default;
      // core functionalities
      t_state default_state() const {return 0; }
      t_state start() const { return 0; }
      bool is_final(const t_state& state) const { return (state == this->m_max); }
      bool is_error(const t_state& state) const { return (state > this->m_max); }
      bool next(const t_letter c, const t_state& start, t_state& end) const;
      void nexts(const t_state& state, t_letter_set& set) const;

      const std::string& get_regexp() const { return this-> m_regexp; }

      // operators and friend functions
      bool operator==(const type& other) const { return this == &other; }

      friend std::ostream& operator<<(std::ostream& os, const type& parser) {
        using _w_content = data_wrapper<const t_content&>;
        return (os << "sequence" << _w_content(parser.m_content));
      }

      friend void swap(type& p1, type& p2) {
        using std::swap;
        swap(p1.m_content, p2.m_content);
        swap(p1.m_max, p2.m_max);
        swap(p1.m_regexp, p2.m_regexp);
      }

      static const t_parser_trigger trigger;
      static constexpr parsing_complexity complexity = parsing_complexity::SEQUENCE;

      const t_letter& get_letter() const {
        if(this->m_content.size() == 1) { return this->m_content[0]; }
        else { throw hrw::exception::spec_get_letter(this->get_regexp()); }
      }

    private:
      using t_content = std::vector<t_letter>;
      t_content m_content;
      unsigned int m_max;

      std::string m_regexp;
    };


    /////////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION
    /////////////////////////////////////////////////////////////////////////////

    template<typename targ_alphabet, const targ_alphabet& alphabet>
    sequence<targ_alphabet, alphabet>::sequence(const std::string& s): m_regexp(s) {
      unsigned int begin = 0, current = 0;
      for(const char c: s) {
        if(is_char_separator(c)) {
          if(begin != current) { // we finished a word
            this->m_content.push_back(alphabet.get_letter(s.substr(begin, current-begin)));
          }
          ++current;
          begin = current;
        } else if(is_char_special(c)) {
          throw hrw::exception::spec_invalid_sequence(s);
        } else {
          ++current;
        }
      }
      if(begin != current) { // the end of the string finished a word
        this->m_content.push_back(alphabet.get_letter(s.substr(begin, current-begin)));
      }
      this->m_max = this->m_content.size();
    }



    template<typename targ_alphabet, const targ_alphabet& alphabet>
    bool sequence<targ_alphabet, alphabet>::next(const t_letter c, const t_state& start, t_state& end) const {
      // std::cout << "sequence::next = (" << start << ", " << this->m_max << ", " << end << ") -> " << std::boolalpha << ((start+1) == this->m_max) << std::endl;
      if(start >= this->m_max) {
        end = this->m_max+1;
        return false;
      } else {
        const t_letter& c_local = this->m_content[start];
        if(alphabet.is_subletter(c, c_local)) { // matches
          end = start + 1;
          // std::cout << "  => matched letter " << c << ": old state=" << start << ", new state=" << end << std::endl;
          return this->is_final(end);
        } else { // error
          // std::cout << "  => error letter: expected letter=" << c_local << ", gotten letter=" << c << std::endl;
          end = this->m_max+1;
          return false;
        }
      }
    }

    template<typename targ_alphabet, const targ_alphabet& alphabet>
    void sequence<targ_alphabet, alphabet>::nexts(const t_state& state, t_letter_set& set) const {
      set.clear();
      if(state < this->m_max) {
        set.insert(this->m_content[state]);
      }
    }


    template<typename targ_alphabet, const targ_alphabet& alphabet>
    const t_parser_trigger sequence<targ_alphabet, alphabet>::trigger = &is_sequence<std::string>;

  }
} // end namespace


#endif // __HREWRITE_PARSING_SEQUENCE_H__

