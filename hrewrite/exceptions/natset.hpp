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


#ifndef __HREWRITE_EXCEPTION_NATSET_H__
#define __HREWRITE_EXCEPTION_NATSET_H__

#include <exception>
#include <optional>
#include <string>
#include <sstream>

#include "hrewrite/utils/print.hpp"
#include "hrewrite/exceptions/common.hpp"

namespace hrw {
  namespace exception {

    ///////////////////////////////////////////
    // natset
    template<typename type>
    class natset_cannot_contain: public abstract_error {
    public:
      using t_nat = typename type::t_nat;
      natset_cannot_contain(t_nat value): m_value(value) {}
    private:
      t_nat m_value;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          std::stringstream res;
          res << "ERROR: " << (hrw::utils::type_name<type>()) << " cannot contain value " << this->m_value;
          this->m_msg = std::move(res.str());
        }
      }
    };

    template<typename type>
    class natset_limit: public abstract_error {
    public:
      using size_type = typename type::size_type;
      natset_limit(size_type value): m_value(value) {}
    private:
      size_type m_value;
    protected:
      virtual void ensure_msg() const {
        if(not this->m_msg.has_value()) {
          std::stringstream res;
          res << "ERROR: " << (hrw::utils::type_name<type>()) << " cannot contain " << this->m_value << " values";
          this->m_msg = std::move(res.str());
        }
      }
    };


  }
}

#endif // __HREWRITE_EXCEPTION_NATSET_H__

