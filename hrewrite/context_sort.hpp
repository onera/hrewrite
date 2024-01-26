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


#ifndef __HREWRITE_C_SORT_H__
#define __HREWRITE_C_SORT_H__

// #include <unordered_map> // TODO: remove hard dependency
#include <string>
#include <vector>
#include <algorithm>

#include "hrewrite/utils.hpp"
#include "hrewrite/theory/core.hpp"
#include "hrewrite/exceptions/undeclared.hpp"

namespace hrw {

  // NOTE: this implementation supposes that there isn't a lot of sorts, which is true in 100% of the tested cases

  template<typename targ_natset>
  class context_sort {
  public:
    using type = context_sort<targ_natset>;
    using natset = targ_natset;

    //////////////////////////////////////////
    // 1. adding sorts
    t_sort_id add_sort(const std::string& name) {
      auto it = this->find(name);
      if(it == this->end()) {
        t_sort_id res = this->m_sorts.size();
        this->m_sorts.emplace_back(container_sort(name));
        return res;
      } else {
        return (it - this->begin());
      }
    }

    t_sort_id add_sort(std::string&& name) {
      auto it = this->find(name);
      if(it == this->end()) {
        t_sort_id res = this->m_sorts.size();
        this->m_sorts.emplace_back(container_sort(std::move(name)));
        // this->m_sort_names[name] = res;
        return res;
      } else {
        return (it - this->begin());
      }
    }

    //////////////////////////////////////////
    // 2. adding order
    void add_subsort(const t_sort_id subsort, const t_sort_id supsort)  {
      if(subsort != supsort) {
        context_sort::container_sort& sub_data = this->m_sorts[subsort];
        context_sort::container_sort& sup_data = this->m_sorts[supsort];
        if(!sup_data.m_subsorts.contains(subsort)) {
          // sub relation
          sup_data.m_subsorts.insert(subsort);
          sup_data.m_subsorts.insert(sub_data.m_subsorts);
          for(t_sort_id ssupsort: sup_data.m_supsorts) {
            this->m_sorts[ssupsort].m_subsorts.insert(subsort);
            this->m_sorts[ssupsort].m_subsorts.insert(sub_data.m_subsorts);
          }
          // sup relation
          sub_data.m_supsorts.insert(supsort);
          sub_data.m_supsorts.insert(sup_data.m_supsorts);
          for(t_sort_id ssubsort: sub_data.m_subsorts) {
            this->m_sorts[ssubsort].m_supsorts.insert(supsort);
            this->m_sorts[ssubsort].m_supsorts.insert(sup_data.m_supsorts);
          }
        }
      }
    }

    void add_subsort(const std::string& subsort, const std::string& supsort) {
      this->add_subsort(this->get_letter(subsort), this->get_letter(supsort));
    }

    //////////////////////////////////////////
    // 3. getters
    std::string const& get_name(const t_sort_id sort) const { return this->m_sorts[sort].m_name; }
    natset const& get_subsorts(const t_sort_id sort)  const { return this->m_sorts[sort].m_subsorts; }
    natset const& get_supsorts(const t_sort_id sort)  const { return this->m_sorts[sort].m_supsorts; }

    natset const& get_subsorts(const std::string& sort)  const {
      auto it = this->ensure(sort);
      return it->m_subsorts;
    }
    natset const& get_supsorts(const std::string& sort)  const {
      auto it = this->ensure(sort);
      return it->m_supsorts;
    }

    //////////////////////////////////////////
    // 4. testing
    bool contains(const std::string& sort) { return (this->find(sort) != this->end()); }
    bool contains(const t_sort_id sort) { return (sort < this->m_sorts.size()); }

    bool is_subsort(const t_sort_id s1, const t_sort_id s2) const { return (s1 == s2) || (this->m_sorts[s2].m_subsorts.contains(s1)); }
    bool is_subsort(const std::string& s1, const std::string& s2) const {
      return this->is_subsort(this->get_letter(s1), this->get_letter(s2));
    }

    void clear() { this->m_sorts.clear(); }


    //////////////////////////////////////////
    // implementation of parsing::c_alphabet
    using t_letter = t_sort_id;
    using t_letter_set = natset;

    t_sort_id get_letter(const std::string& sort_name) const {
      auto it = this->ensure(sort_name);
      return (it - this->begin());
    }

    bool is_subletter(const t_sort_id p_subletter, const t_sort_id p_supletter) const { return this->is_subsort(p_subletter, p_supletter); }
    const natset& get_subletters(const t_sort_id letter) const { return this->m_sorts[letter].m_subsorts; }
    const natset& get_supletters(const t_sort_id letter) const { return this->m_sorts[letter].m_supsorts; }

  private:
    struct container_sort {
      container_sort(const std::string& name): m_name(name), m_subsorts(), m_supsorts() {}
      container_sort(std::string&& name): m_name(name), m_subsorts(), m_supsorts() {}
      std::string m_name;
      natset m_subsorts;
      natset m_supsorts;
    };
    std::vector<container_sort> m_sorts;
    using const_iterator = typename std::vector<container_sort>::const_iterator;

    const_iterator begin() const { return this->m_sorts.begin(); }
    const_iterator end() const { return this->m_sorts.end(); }
    const_iterator find(const std::string& name) const {
      return std::find_if(this->begin(), this->end(), [&name](auto& c) { return c.m_name == name; });
    }
    const_iterator ensure(const std::string& name) const {
      auto it = this->find(name);
        if(it == this->end()) {
        throw hrw::exception::ndeclared_sort(name); 
      } else {
        return it;
      }
    }
  };


}


#endif // __HREWRITE_C_SORT_H__

