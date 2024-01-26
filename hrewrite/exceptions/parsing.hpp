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


#ifndef __HREWRITE_EXCEPTION_PARSING_H__
#define __HREWRITE_EXCEPTION_PARSING_H__

#include <exception>
#include <optional>
#include <string>
#include <sstream>

#include "hrewrite/exceptions/common.hpp"

namespace hrw {
  namespace exception {


    ///////////////////////////////////////////
    // No parser
    class spec_no_parser: public abstract_error {
    public:
      spec_no_parser(std::string const & spec): m_spec(spec) {}
    private:
      std::string m_spec;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          this->m_msg = "ERROR: no valid parser for regexp \"" + this->m_spec + "\"";
        }
      }
    };

    ///////////////////////////////////////////
    // Invalid spec
    class spec_invalid_element: public abstract_error {
    public:
      spec_invalid_element(std::string const & spec): m_spec(spec) {}
    private:
      std::string m_spec;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          this->m_msg = "ERROR: the string in parameter is not a valid specification (expected a single word, found \"" + this->m_spec + "\")";
        }
      }
    };

    class spec_invalid_sequence: public abstract_error {
    public:
      spec_invalid_sequence(std::string const & spec): m_spec(spec) {}
    private:
      std::string m_spec;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          this->m_msg = "ERROR: the string in parameter is not a valid specification (expected a word sequence, found \"" + this->m_spec + "\")";
        }
      }
    };

    class spec_invalid_automata: public abstract_error {
    public:
      spec_invalid_automata(std::string const & spec): m_spec(spec) {}
    private:
      std::string m_spec;
    protected:
      void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          this->m_msg = "ERROR: the string in parameter is not a valid specification (expected a regular expression, found \"" + this->m_spec + "\")";
        }
      }
    };

    class spec_invalid_char_pos: public abstract_error {
    public:
      spec_invalid_char_pos(std::string const & spec, char expected, unsigned int pos): m_spec(spec), m_expected(expected), m_pos(pos) {}
    private:
      std::string m_spec;
      char m_expected;
      unsigned int m_pos;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          std::stringstream res;
          res << "ERROR: in specification \"" << this->m_spec << "\", expected character \"" << this->m_expected << "\" at position " << this->m_pos;
          this->m_msg = std::move(res.str());
        }
      }
    };

    using spec_invalid = spec_invalid_automata;


    ///////////////////////////////////////////
    // Get letter error
    class spec_get_letter: public abstract_error {
    public:
      spec_get_letter(std::string const & spec): m_spec(spec) {}
    private:
      std::string m_spec;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          this->m_msg = "ERROR: the regular expression \"" + this->m_spec + "\" does not contain a single word";
        }
      }
    };

    static char const * spec_get_letter_no_parser_cstr = "ERROR: empty parser combination do not contain a letter";
    class spec_get_letter_no_parser: public std::exception {
    public:
      char const * what() const noexcept override {
        return spec_get_letter_no_parser_cstr;
      }
    };


}}

#endif // __HREWRITE_EXCEPTION_PARSING_H__

