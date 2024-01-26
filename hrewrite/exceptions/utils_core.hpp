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


#ifndef __HREWRITE_EXCEPTION_UTILS_CORE_H__
#define __HREWRITE_EXCEPTION_UTILS_CORE_H__

#include <exception>
#include <string>

namespace hrw {
  namespace exception {

    ///////////////////////////////////////////
    // single container
    static char const * single_container_full_cstr = "ERROR: single container already contained an element";
    class single_container_full: public std::exception {
    public:
      const char* what() const noexcept override {
        return single_container_full_cstr;
      }
    };

    ///////////////////////////////////////////
    // iterator
    static char const * iterator_increment_cstr = "ERROR: cannot increment a dummy iterator";
    class iterator_increment: public std::exception {
    public:
      const char* what() const noexcept override {
        return iterator_increment_cstr;
      }
    };

    static char const * iterator_access_cstr = "ERROR: cannot access a dummy iterator";
    class iterator_access: public std::exception {
    public:
      const char* what() const noexcept override {
        return iterator_access_cstr;
      }
    };

  }
}

#endif // __HREWRITE_EXCEPTION_UTILS_CORE_H__

