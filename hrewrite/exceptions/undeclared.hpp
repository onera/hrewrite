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


#ifndef __HREWRITE_EXCEPTION_UNDECLARED_H__
#define __HREWRITE_EXCEPTION_UNDECLARED_H__

#include <exception>
#include <string>

#include "hrewrite/exceptions/common.hpp"

namespace hrw {
  namespace exception {

    ///////////////////////////////////////////
    // Sort
    class ndeclared_sort: public abstract_error {
    public:
      ndeclared_sort(std::string const & name): m_name(name) {}
    private:
      std::string m_name;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          this->m_msg = "ERROR: the sort \"" + this->m_name + "\" is not declared";
        }
      }
    };

}}



#endif // __HREWRITE_EXCEPTION_UNDECLARED_H__

