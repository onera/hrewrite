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


#ifndef __HREWRITE_C_CONSTRUCTOR_H__
#define __HREWRITE_C_CONSTRUCTOR_H__

#include <string>
#include <vector>

#include "hrewrite/utils.hpp"
#include "hrewrite/theory/core.hpp"

namespace hrw {

  template<typename targ_spec>
  class context_constructor {
  public:
    using type = context_constructor<targ_spec>;
    using t_spec = targ_spec;

    context_constructor() = default;
    context_constructor(const context_constructor&) = delete;
    context_constructor& operator=(const context_constructor&) = delete;
    ~context_constructor() = default;

    t_constructor_id add_constructor(const t_sort_id sort, const std::string& name, const std::string& spec);
    t_constructor_id add_constructor(const t_sort_id sort, std::string&& name, std::string&& spec);
    t_sort_id get_sort(t_constructor_id c) const;
    const std::string& get_name(t_constructor_id c) const;
    const t_spec& get_spec(t_constructor_id c) const;

    bool contains(const t_constructor_id c) const;
    bool contains(const std::string& name) const;

    void clear() { this->m_constructors.clear(); }

  private:
    struct container_constructor {
      container_constructor(const t_sort_id sort, const std::string& name, const t_spec& spec): m_sort(sort), m_name(name), m_spec(spec) {}
      container_constructor(const t_sort_id sort, const std::string& name, t_spec&& spec): m_sort(sort), m_name(name), m_spec(spec) {}
      container_constructor(const t_sort_id sort, std::string&& name, const t_spec& spec): m_sort(sort), m_name(name), m_spec(spec) {}
      container_constructor(const t_sort_id sort, std::string&& name, t_spec&& spec): m_sort(sort), m_name(name), m_spec(spec) {}
      t_sort_id sort() const { return this->m_sort; }
      const std::string& name() const { return this->m_name; }
      const t_spec& spec() const { return this->m_spec; }

      const t_sort_id m_sort;
      const std::string m_name;
      const t_spec m_spec;
    };
    std::vector<container_constructor> m_constructors;
  };


  template<>
  class context_constructor<void> {
  public:
    using type = context_constructor<void>;

    context_constructor() = default;
    context_constructor(const context_constructor&) = delete;
    context_constructor& operator=(const context_constructor&) = delete;
    ~context_constructor() = default;

    t_constructor_id add_constructor(const t_sort_id sort, const std::string& name);
    t_sort_id get_sort(t_constructor_id c) const;
    const std::string& get_name(t_constructor_id c) const;

    bool contains(const t_constructor_id c) const;
    bool contains(const std::string& name) const;

    void clear() { this->m_constructors.clear(); }

  private:
    struct container_constructor {
      container_constructor(const t_sort_id sort, const std::string& name): m_sort(sort), m_name(name) {}
      container_constructor(const t_sort_id sort, std::string&& name): m_sort(sort), m_name(name) {}
      t_sort_id sort() const { return this->m_sort; }
      const std::string& name() const { return this->m_name; }

      const t_sort_id m_sort;
      const std::string m_name;
    };
    std::vector<container_constructor> m_constructors;
  };


  /////////////////////////////////////////////////////////////////////////////
  // IMPLEMENTATION
  /////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////
  // for targ_spec

  template<typename targ_spec>
  t_constructor_id context_constructor<targ_spec>::add_constructor(const t_sort_id sort, const std::string& name, const std::string& spec) {
    this->m_constructors.emplace_back(container_constructor(sort, name, t_spec(spec)));
    // std::cout << "add_constructor[spec][" << this << "](" << sort << ", " << name << ", " << spec << ") -> " << (this->m_constructors.size() - 1) << std::endl;
    return (this->m_constructors.size() - 1);
  }

  template<typename targ_spec>
  t_constructor_id context_constructor<targ_spec>::add_constructor(const t_sort_id sort, std::string&& name, std::string&& spec) {
    this->m_constructors.emplace_back(container_constructor(sort, std::forward<std::string>(name), t_spec(spec)));
    // std::cout << "add_constructor[spec][" << this << "](" << sort << ", " << name << ", " << spec << ") -> " << (this->m_constructors.size() - 1) << std::endl;
    return (this->m_constructors.size() - 1);
  }

  template<typename targ_spec>
  t_sort_id context_constructor<targ_spec>::get_sort(t_constructor_id c) const {
    return this->m_constructors[c].sort();
  }

  template<typename targ_spec>
  const std::string& context_constructor<targ_spec>::get_name(t_constructor_id c) const {
    return this->m_constructors[c].name();
  }

  template<typename targ_spec>
  const typename context_constructor<targ_spec>::t_spec& context_constructor<targ_spec>::get_spec(t_constructor_id c) const {
    return this->m_constructors[c].spec();
  }

  template<typename targ_spec>
  bool context_constructor<targ_spec>::contains(const t_constructor_id c) const {
    // std::cout << "contains[spec][" << this << "](" << c << ") vs " << (this->m_constructors.size()) << std::endl;
    return (this->m_constructors.size() > c);
  }
  template<typename targ_spec>
  bool context_constructor<targ_spec>::contains(const std::string& name) const {
    for(auto cdata: this-> m_constructors) {
      if(name == cdata.m_name) {
        return true;
      }
    }
    return false;
  }


  //////////////////////////////////////////
  // for void

  inline t_constructor_id context_constructor<void>::add_constructor(const t_sort_id sort, const std::string& name) {
    this->m_constructors.emplace_back(container_constructor(sort, name));
    // std::cout << "add_constructor[void][" << this << "](" << sort << ", " << name << ") -> " << (this->m_constructors.size() - 1) << std::endl;
    return (this->m_constructors.size() - 1);
  }

  inline t_sort_id context_constructor<void>::get_sort(t_constructor_id c) const {
    return this->m_constructors[c].sort();
  }

  inline const std::string& context_constructor<void>::get_name(t_constructor_id c) const {
    return this->m_constructors[c].name();
  }

  inline bool context_constructor<void>::contains(const t_constructor_id c) const {
    // std::cout << "contains[void][" << this << "](" << c << ") vs " << (this->m_constructors.size()) << std::endl;
    return (this->m_constructors.size() > c);
  }
  inline bool context_constructor<void>::contains(const std::string& name) const {
    for(auto cdata: this-> m_constructors) {
      if(name == cdata.m_name) {
        return true;
      }
    }
    return false;
  }


}


#endif // __HREWRITE_C_CONSTRUCTOR_H__

