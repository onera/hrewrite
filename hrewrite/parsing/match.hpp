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


#ifndef __HREWRITE_PARSING_MATCH_H__
#define __HREWRITE_PARSING_MATCH_H__

#include <iterator>

namespace hrw {
  namespace utils {

    template<typename PARSER, typename inputIt>
    class match {
    public:
      match(const PARSER& p, const inputIt& begin, const inputIt& end) : m_parser(p), m_begin(begin), m_end(end) {}

      class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = inputIt;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        using const_pointer = value_type const *;
        using const_reference = value_type const &;

        iterator& operator++();
        bool operator==(const match<PARSER, inputIt>::iterator &other) const;
        bool operator!=(const match<PARSER, inputIt>::iterator &other) const;
        const inputIt& operator*() const;
      private:
        friend class match<PARSER, inputIt>;
        iterator(const match<PARSER, inputIt>& m, bool is_end) : m_match(m), m_current(m.m_begin), m_is_end(is_end), m_state(this->m_match.m_parser.start()) {
          if(!this->m_is_end) {
            if (!this->m_match.m_parser.is_final(this->m_state)) {
              ++(*this);
            }
          }
        } // go to the first recognition
        const match<PARSER, inputIt>& m_match;
        inputIt m_current;
        bool m_is_end;
        typename PARSER::t_state m_state;
      };

      iterator begin();
      iterator end();

      bool operator==(const match<PARSER, inputIt> &other) const;
      bool operator!=(const match<PARSER, inputIt> &other) const;
    private:
      const PARSER& m_parser;
      const inputIt m_begin;
      const inputIt m_end;
    };


    /////////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION
    /////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////
    // 6.1. match::iterator

    template<typename PARSER, typename inputIt>
    typename match<PARSER, inputIt>::iterator& match<PARSER, inputIt>::iterator::operator++() {
      typename PARSER::t_state tmp(this->m_match.m_parser.default_state());
      typename PARSER::t_state* st_1, * st_2, * st_exchange;

      // std::cout << "++" << std::endl;// << ((void*)(this)) << std::endl;

      st_1 = &(this->m_state);
      st_2 = &tmp;
      while(true) {
        if(this->m_current == this->m_match.m_end) {
          // std::cout << " we are at the end of the string" << std::endl;
          this->m_is_end = true;
          return *this;
        } else {
          // next letter
          const auto& c = *(this->m_current);
          // std::cout << " found letter \"" << c << "\", continuing" << " (current state " << (*st_1) << ")" << std::endl;
          ++(this->m_current);
          bool is_final = this->m_match.m_parser.next(c, *st_1, *st_2);
          // std::cout << std::boolalpha << "match::++ : state=" << (*st_2) << " is_final=" << is_final << ", is_error" << this->m_match.m_parser.is_error(*st_2) << std::endl;
          if(is_final) {
            // std::cout << " we are in a final state " << (*st_2) << " (previous state " << (*st_1) << ")" << std::endl;
            // std::cout << " this->m_state=" << (this->m_state) << ", tmp=" << (tmp) << std::endl;
            if (st_1 == &(this->m_state)) { // this means that st_1 points to tmp
              this->m_state = tmp;
              // std::cout << " switching: this->m_state=" << (this->m_state) << std::endl;
            }
            return *this;
          } else if(this->m_match.m_parser.is_error(*st_2)) {
            // std::cout << " we are in an error state" << std::endl;
            this->m_is_end = true;
            return *this;
          } else {
            st_exchange = st_1;
            st_1 = st_2;
            st_2 = st_exchange;
            // non need to reset st_2: this is done in the "next" method
          }
        }
      }
    }

    template<typename PARSER, typename inputIt>
    bool match<PARSER, inputIt>::iterator::operator==(const match<PARSER, inputIt>::iterator &other) const {
      if(this->m_is_end) {
        return other.m_is_end;
      } else if(other.m_is_end) {
        return false;
      } else {
        return (this->m_match == other.m_match) && (this->m_current == other.m_current);
      }
    }

    template<typename PARSER, typename inputIt>
    bool match<PARSER, inputIt>::iterator::operator!=(const match<PARSER, inputIt>::iterator& other) const {
      return !(*this == other);
    }

    template<typename PARSER, typename inputIt>
    const inputIt& match<PARSER, inputIt>::iterator::operator*() const {
      return this->m_current;
    }


    /////////////////////////////////////////
    // 6.1. match

    template<typename PARSER, typename inputIt>
    typename match<PARSER, inputIt>::iterator match<PARSER, inputIt>::begin() {
      return match<PARSER, inputIt>::iterator(*this, false);
    }

    template<typename PARSER, typename inputIt>
    typename match<PARSER, inputIt>::iterator match<PARSER, inputIt>::end() {
      return match<PARSER, inputIt>::iterator(*this, true);
    }

    template<typename PARSER, typename inputIt>
    bool match<PARSER, inputIt>::operator==(const match<PARSER, inputIt> &other) const {
      return this == &other;
    }

    template<typename PARSER, typename inputIt>
    bool match<PARSER, inputIt>::operator!=(const match<PARSER, inputIt> &other) const {
      return this != &other;
    }


  }
} // end namespace


#endif // __HREWRITE_PARSING_MATCH_H__

