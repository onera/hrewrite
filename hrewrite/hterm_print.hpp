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


#ifndef __HREWRITE_PRINT_H__
#define __HREWRITE_PRINT_H__

#include <unordered_map> // TODO: remove hard dependency
#include <string>

#include "hrewrite/utils.hpp"
#include "hrewrite/theory/core.hpp"

namespace hrw {

  template<typename targ_ctx_theory, typename targ_term_full>
  class t_hterm_print {
  public:
    using type = t_hterm_print<targ_ctx_theory, targ_term_full>;

    using t_term_full = targ_term_full;
    using t_term_full_ref = typename t_term_full::reference;
    using t_variable      = typename t_term_full::t_variable;
    using t_substitution  = typename t_term_full::t_substitution;

    using t_print_term = hrw::utils::print_with_ctx<type, const t_term_full>;
    using t_print_subst = hrw::utils::print_with_ctx<type, const t_substitution>;

    t_hterm_print(): m_count(0) {}

    t_print_term print(const t_term_full_ref& t) {
      return t_print_term(*this, *t);
    }

    t_print_subst print(const t_substitution& subst) {
      return t_print_subst(*this, subst);
    }

    template<typename th>
    const std::string& get_name_constructor(const t_constructor_id cid) const {
      return targ_ctx_theory::get_name(t_constructor_core<th>(cid));
    }
    const std::string& get_name_variable(const std::size_t v_id) {
      auto it = this->m_vnames.find(v_id);
      if(it == this->m_vnames.end()) {
        auto pair = this->m_vnames.insert(std::make_pair(v_id, this->new_name()));
        it = pair.first;
      }
      return it->second;
    }

  private:
    std::unordered_map<std::size_t, const std::string> m_vnames;
    unsigned int m_count;

    std::string new_name() {
      int vid = this->m_count;
      std::stringstream os;
      ++this->m_count;
      os << '\'';
      while(vid > 26) {
        os << (char)(97 + (vid % 26));
        vid = vid / 26;
      }
      os << (char)(97 + (vid % 26));
      return os.str();
    }
  };

}


#endif // __HREWRITE_PRINT_H__

