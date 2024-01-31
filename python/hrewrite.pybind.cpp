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


#include <iostream>
#include <vector>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <type_traits>

#include <pybind11/pybind11.h>

#include "python/hrewrite.pybind.hpp"

#include "hrewrite/theory/theory_variable.hpp"
#include "hrewrite/theory/theory_free.hpp"
#include "hrewrite/theory/theory_literal.hpp"
#include "hrewrite/theory/theory_leaf.hpp"


/////////////////////////////////////////////////////////////////////////////
// APPLICATION OF THE GENERIC API
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////
// 1. parsers
template<typename targ_alphabet, const targ_alphabet& arg_alphabet>
using my_automata = utils::automata<utils::natset, utils::natset, targ_alphabet, arg_alphabet>;

template<typename t_alphabet, const t_alphabet& alphabet>
using t_vparser = utils::combine_variant<t_alphabet, alphabet, utils::element, my_automata>;

template<typename t_alphabet, const t_alphabet& alphabet>
using t_sparser = utils::combine_variant<t_alphabet, alphabet, utils::sequence, my_automata>;


//////////////////////////////////////////
// 2. theories
// solution: instead of using targs, write functions directly. would work. Need to add template arguments, that's all
struct th_api_free {
  static const std::string& name() { static const std::string res = "free"; return res; }
  template<typename tt, const tt& arg> using tt_theory = hrw::theory::tp_theory_free<std::vector, t_sparser>::template type<tt, arg>;
  template<typename t_theory, typename t_term_full_wrapper> struct th_factory {
    using t_term_full = typename t_term_full_wrapper::t_term_full;
    using t_term_full_ref = typename t_term_full_wrapper::t_term_full_ref;
    using t_term = typename t_theory::template tt_term<t_term_full>;
    using t_container = typename t_term::t_container;

    using t_cargs = std::tuple<const std::string, const std::string>;     // the arguments for constructor creation
    using t_targs = std::tuple<py::tuple>;     // the arguments for term creation -> should be a py::list from which we extract the term wrappers and the term

    static t_container t_targs_wrapper(const py::tuple& l) {
      t_container res;
      for(auto o: l) {
        res.push_back(o.cast<t_term_full_wrapper>().m_content);
      }
      return res;
    }
    static py::tuple dumps(const t_term& t, std::function<const py::tuple(const t_term_full_ref)> translate) {
      py::list subterms;
      for (auto subterm : t) { subterms.append(translate(subterm)); }
      std::size_t size = subterms.size() + 1;
      py::tuple res(size);
      res[0] = t.get_constructor();
      for(std::size_t i = 1; i < size; ++i) {
        res[i] = subterms[i-1];
      }
      return res;
    }
    static std::pair<t_constructor_id, std::tuple<t_container>> loads(const py::tuple t, std::function<t_term_full_ref(const py::tuple)> translate) {
      t_constructor_id cid = t[0].cast<t_constructor_id>();
      t_container c;
      auto it = t.begin();
      ++it;
      for(; it != t.end(); ++it) {
        c.push_back(translate((*it).cast<py::tuple>()));
      }
      return std::make_pair(cid, std::make_tuple(c));
    }
  };
};



struct pyobj_hash {
  using value_type = py::object;
  std::size_t operator()(py::object const obj) const { return static_cast<std::size_t>(obj.attr("__hash__")().cast<std::make_signed_t<std::size_t>>()); }
};
struct pyobj_eq {
  using value_type = py::object;
  bool operator()(py::object const left, py::object const right) const { return left.is(right); }
};

struct th_api_lit_obj {
  static const std::string& name() { static const std::string res = "literal"; return res; }
  template<typename tt, const tt& arg> using tt_theory = hrw::theory::tp_theory_literal<py::object, pyobj_hash, pyobj_eq>::template type<tt, arg>;
  template<typename t_theory, typename t_term_full_wrapper> struct th_factory {
    using t_term_full = typename t_term_full_wrapper::t_term_full;
    using t_term_full_ref = typename t_term_full_wrapper::t_term_full_ref;
    using t_term = typename t_theory::template tt_term<t_term_full>;

    using t_cargs = std::tuple<const std::string>;     // the arguments for constructor creation
    using t_targs = std::tuple<py::object>;     // the arguments for term creation
    static py::tuple dumps(const t_term& t, std::function<const py::tuple(const t_term_full_ref)>) { return py::make_tuple(t.get_constructor(), t.get_value()); }
    static std::pair<t_constructor_id, t_targs> loads(const py::tuple t, std::function<t_term_full_ref(const py::tuple)>) {
      t_constructor_id cid = t[0].cast<t_constructor_id>();
      py::object value = t[1];
      return std::make_pair(cid, std::make_tuple(value));
    }
  };
};


struct th_api_leaf {
  static const std::string& name() { static const std::string res = "leaf"; return res; }
  template<typename tt, const tt& arg> using tt_theory = hrw::theory::tp_theory_leaf::template type<tt, arg>;
  template<typename t_theory, typename t_term_full_wrapper> struct th_factory {
    using t_term_full = typename t_term_full_wrapper::t_term_full;
    using t_term_full_ref = typename t_term_full_wrapper::t_term_full_ref;
    using t_term = typename t_theory::template tt_term<t_term_full>;

    using t_cargs = std::tuple<const std::string>;     // the arguments for constructor creation
    using t_targs = std::tuple<>;     // the arguments for term creation -> should be a py::list from which we extract the term wrappers and the term
    static py::tuple dumps(const t_term& t, std::function<py::tuple(const t_term_full_ref)>) { return py::make_tuple(t.get_constructor()); }
    static std::pair<t_constructor_id, t_targs> loads(const py::tuple t, std::function<t_term_full_ref(const py::tuple)>) {
      t_constructor_id cid = t[0].cast<t_constructor_id>();
      return std::make_pair(cid, std::make_tuple());
    }
  };
};


//////////////////////////////////////////
// 3. module creation
PYBIND11_MODULE(libhrewrite_python, m) {
  m.doc() = "hrewrite wrapper for python";
  register_hrw<
    false,
    hrw::e_strategy::STY_INNER,
    t_vparser,
    th_api_free, th_api_leaf, th_api_lit_obj
  >(m);
}

