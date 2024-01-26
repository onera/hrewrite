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


#ifndef __HREWRITE_EXCEPTION_REWRITE_H__
#define __HREWRITE_EXCEPTION_REWRITE_H__

#include <exception>
#include <string>
#include <sstream>

#include "hrewrite/exceptions/common.hpp"

namespace hrw {
  namespace exception {

    ///////////////////////////////////////////
    // Ground term error
    static char const * exception_gterm_cstr = "ERROR: rewriting is implemented only on ground terms";
    class rw_gterm: public std::exception {
    public:
      char const * what() const noexcept override {
        return exception_gterm_cstr;
      }
    };

    ///////////////////////////////////////////
    // Pattern term error
    static char const * exception_pattern_cstr = "ERROR: the pattern of a rewriting rule must be a structured term";
    class rw_pattern: public std::exception {
    public:
      char const * what() const noexcept override {
        return exception_pattern_cstr;
      }
    };


    ///////////////////////////////////////////
    // Rewriting rule error
    template<typename t_term_full_ref>
    class rw_rule: public abstract_error {
    public:
      rw_rule(t_term_full_ref pattern, t_term_full_ref image): m_pattern(pattern), m_image(image) {}
    private:
      t_term_full_ref m_pattern;
      t_term_full_ref m_image;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          std::stringstream res;
        res << "ERROR: the image sort (\"" << this->m_image->get_sort() << "\") is not a subsort of the pattern sort (\"" << this->m_pattern->get_sort() << "\")";
          this->m_msg = std::move(res.str());
        }
      }
    };

}}

#endif // __HREWRITE_EXCEPTION_REWRITE_H__

