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


#ifndef __HREWRITE_EXCEPTION_TH_FREE_H__
#define __HREWRITE_EXCEPTION_TH_FREE_H__

#include <exception>
#include <string>

#include "hrewrite/theory/core.hpp"
#include "hrewrite/exceptions/common.hpp"

namespace hrw {
  namespace exception {

    ///////////////////////////////////////////
    // Term construction error
    template<typename t_term_full_ref>
    class th_free_construct: public abstract_error {
    public:
      th_free_construct(t_constructor_id const c, std::string const & spec_expected, std::string const & spec_got):
        m_c(c), m_spec_expected(spec_expected), m_spec_got(spec_got) {}

      t_constructor_id get_constructor_id() const { return this->m_c; }
      std::string const & get_spec_expected() const { return this->m_spec_expected; }
      std::string const & get_spec_got() const { return this->m_spec_got; }
    private:
      t_constructor_id const m_c;
      std::string const & m_spec_expected;
      std::string const m_spec_got;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          this->m_msg = "ERROR in construction of " + std::to_string(this->m_c) + ": regexp \"" + this->m_spec_expected + "\" does not contain \"" + this->m_spec_got + "\"";
        }
      }
    };

}}



#endif // __HREWRITE_EXCEPTION_TH_FREE_H__

