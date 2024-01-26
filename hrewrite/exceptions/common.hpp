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


#ifndef __HREWRITE_EXCEPTION_COMMON_H__
#define __HREWRITE_EXCEPTION_COMMON_H__

#include <exception>
#include <string>

namespace hrw {
  namespace exception {

    ///////////////////////////////////////////
    // Generic error
    class generic: public std::exception {
    public:
      generic(const std::string& msg): msg(msg) {}
      char const * what() const noexcept override {
        return this->msg.c_str();
      }
    private:
      std::string msg;
    };

    ///////////////////////////////////////////
    // Internal error
    static char const * exception_internal_cstr = "INTERNAL ERROR";
    class internal: public std::exception {
    public:
      char const * what() const noexcept override {
        return exception_internal_cstr;
      }
    };

    ///////////////////////////////////////////
    // Not implemented error
    static char const * exception_unimplemented_cstr = "ERROR: not yet implemented";
    class unimplemented: public std::exception {
    public:
      char const * what() const noexcept override {
        return exception_unimplemented_cstr;
      }
    };


    ///////////////////////////////////////////
    // Abstract error
    class abstract_error: public std::exception {
    public:
      abstract_error(): m_msg(std::nullopt) {}
      char const * what() const noexcept override {
        this->ensure_msg();
        return this->m_msg.value().c_str();
      }
    protected:
      mutable std::optional<std::string> m_msg;
      virtual void ensure_msg() const = 0;
    };

  }
}

#endif // __HREWRITE_EXCEPTION_COMMON_H__

