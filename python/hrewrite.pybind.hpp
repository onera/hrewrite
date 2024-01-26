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


#ifndef __OPGRAPH_TALGEBRA_PYBIND_H__
#define __OPGRAPH_TALGEBRA_PYBIND_H__

#include <iostream>
#include <vector>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <typeinfo>

#include <unordered_map>
#include <unordered_set>

#define PYBIND11_DETAILED_ERROR_MESSAGES 1
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl_bind.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

namespace py = pybind11;

#include "hrewrite/utils.hpp"
#include "hrewrite/parsing.hpp"
#include "hrewrite/hterm.hpp"
#include "hrewrite/hrewrite.hpp"

#include "hrewrite/exceptions/theory_free.hpp"

using namespace hrw;


/////////////////////////////////////////////////////////////////////////////
// GENERIC API FOR REWRITING ENGINE CONSTRUCTION AND MANIPULATION
/////////////////////////////////////////////////////////////////////////////

/**
 * Structure of a th_api struct
 * 
 * struct th_api {
 *   static const std::string& name();  // the name of the theory
 *   template<typename tt, const tt& arg> using tt_theory;    // the theory
 *   template<typename t_theory, typename t_term_full_ref, typename t_term_full_wrapper> struct th_factory {
 *     using t_cargs = std::tuple<cargs...>;     // the arguments for constructor creation
 *     using t_targs = std::tuple<targs...>;     // the arguments for term creation
 *     auto t_targs_wrapper(const targs& ... args); // optional wrapper
 *     using t_term = typename t_theory::template tt_term<t_term_full_ref>;
 *     py::tuple dumps(const t_term&, std::function<py::tuple(const t_term_full_ref)>);
 *     std::pair<t_constructor_id, std::tuple<auto...>> loads(const py::tuple, std::function<t_term_full_ref(const py::tuple)>);
 *   };
 * };
  */


/////////////////////////////////////////////////////////////////////////////
// REGISTER FUNCTION BASED ON THE THEORY API
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////
// 1. term structure from a th_api list

template<typename ... Args> using my_map = std::unordered_map<Args...>;
template<typename ... Args> using unordered_set_wrapper = std::unordered_set<Args...>;
using my_term_registry = hrw::utils::registry_unique<unordered_set_wrapper, true>;


template<
  template<typename tt, const tt& arg> typename t_vparser,
  typename ... th_apis
> struct hconstruct {  
  using t_ctx_th = context_theory<int,
    hrw::context_sort<hrw::utils::natset>,
    hrw::theory::tp_theory_variable_vector<t_vparser>::template type,
    th_apis::template tt_theory ...
  >;

  using t_ctx_tm        = context_term<t_ctx_th, my_term_registry>;
  using t_term_full     = typename t_ctx_tm::t_term_full;
  using t_term_full_ref = typename t_term_full::reference;
  using t_variable      = typename t_term_full::t_variable;
  using t_substitution  = typename t_term_full::t_substitution;

  using t_print = t_hterm_print<t_ctx_th, t_term_full>;

  using t_ctx_rw = context_rw<t_ctx_tm, my_map>;

  struct t_term_full_wrapper {
    using t_term_full     = typename t_ctx_tm::t_term_full;
    using t_term_full_ref = typename t_term_full::reference;
    t_term_full_ref m_content;
  };

  struct _detail {
    template<typename th_factory> using _get = decltype(&th_factory::t_targs_wrapper);

    template<
      typename th_api,
      typename targ_theory
    > struct th_wrapper {
      static const std::string& name() { return th_api::name(); }
      using t_theory = targ_theory;
      using t_term_full_ref = typename t_term_full::reference;
      using t_term = typename t_theory::template tt_term<t_term_full>;

      using th_factory = typename th_api::template th_factory<t_theory, t_term_full_wrapper>;
      static constexpr bool has_targs_wrapper_v = hrw::utils::template inner_type<_get>::template has_v<th_factory>;
    };

    template<std::size_t, typename ...> struct th_wrappers;
    template<std::size_t i> struct th_wrappers<i> { using type = std::tuple<>; };
    template<std::size_t i, typename T, typename ... Ts> struct th_wrappers<i, T, Ts...> {
      using type = hrw::utils::tuple_concat_t<
        std::tuple< th_wrapper<T, typename t_ctx_th::template theory_element_t<i>> >,
        typename th_wrappers<i+1, Ts...>::type
      >;
    };
  };

  using th_wrappers = typename _detail::template th_wrappers<0, th_apis...>::type;
};


//////////////////////////////////////////
// 2. register all theories

template<typename> struct t_wrapper {};
template<typename th, typename ... Args> using tf_constructor = t_constructor_core<th> (*)(const std::string& sort, Args& ... args);

template<typename t_ctx_th, typename t_ctx_tm, typename t_term_full_wrapper, typename thw, typename ... Args>
void register_cdecl(py::module &m, t_wrapper<std::tuple<t_ctx_th, t_ctx_tm, t_term_full_wrapper, thw, Args...>>) {
  using th = typename thw::t_theory;
  using t_term = typename thw::t_term;

  const std::string& th_name = thw::name();
  const std::string name_class = "t_constructor_"   + th_name;
  const std::string name_add   = "add_constructor_" + th_name;
  const std::string name_test  = "is_term_"         + th_name;
  const std::string name_index = "theory_index_"    + th_name;

  py::class_<t_constructor_core<th>>(m, name_class.c_str(), py::module_local())
    .def(py::init<t_constructor_id>())
    .def("get_id", [](t_constructor_core<th> c) { return c.id(); });

  m.def(name_add.c_str(), static_cast<tf_constructor<th, Args...>>(&t_ctx_th::add_constructor), py::return_value_policy::automatic);  
  m.def(name_test.c_str(), [](t_term_full_wrapper t){ return (std::get_if<t_term>(&(t.m_content->m_content)) != nullptr); });

  m.def("get_cs_sort", &t_ctx_th::template get_sort<th>, py::return_value_policy::reference);
  m.def("get_cs_key" , &t_ctx_th::template get_key<th> , py::return_value_policy::reference);
  m.def("get_cs_name", &t_ctx_th::template get_name<th>, py::return_value_policy::reference);
  if constexpr(hrw::has_spec_v<th>) {
    m.def("get_cs_spec", [](t_constructor_core<th> cs) { return t_ctx_th::template get_spec<th>(cs).get_regexp(); }, py::return_value_policy::reference);
  } else {
    m.def("get_cs_spec", [](t_constructor_core<th>) { return t_ctx_th::template theory_index_v<th>; });
  }

  m.def("theory_index", [](t_constructor_core<th>&) { return t_ctx_th::template theory_index_v<th>; }, py::return_value_policy::automatic);
  m.attr(name_index.c_str()) = t_ctx_th::template theory_index_v<th>;

  if constexpr(has_value_v<t_term>) {
    m.def("get_value", [](t_term_full_wrapper t){ return std::get<t_term>(t.m_content->m_content).get_value(); });
  }
}

template<typename py_class, typename t_term_full_wrapper, typename t_ctx_tm, typename thw, typename ... Args>
void register_tdecl(py_class& c, t_wrapper<std::tuple<t_term_full_wrapper, t_ctx_tm, thw, Args...>>) {
  using th = typename thw::t_theory;
  c.def("create_sterm", [](t_ctx_tm* _this, const t_constructor_core<th> c, Args& ... args) {
    if constexpr(thw::has_targs_wrapper_v) {
      return t_term_full_wrapper{_this->create_sterm(c, thw::th_factory::t_targs_wrapper(args...))};
    } else {
      return t_term_full_wrapper{_this->create_sterm(c, args...)};
    }
  }, py::return_value_policy::automatic);
}

template<typename py_class, typename hrw_all, typename thw>
void register_theory(py::module &m, py_class& py_ctx_tm, hrw_all, thw) {
  using t_ctx_th = typename hrw_all::t_ctx_th;
  using t_ctx_tm = typename hrw_all::t_ctx_tm;
  using t_term_full_wrapper = typename hrw_all::t_term_full_wrapper;
  using CAnnex = std::tuple<t_ctx_th, t_ctx_tm, t_term_full_wrapper, thw>;
  using CArgs = t_wrapper<hrw::utils::tuple_concat_t<CAnnex, typename thw::th_factory::t_cargs>>;
  using TAnnex = std::tuple<t_term_full_wrapper, t_ctx_tm, thw>;
  using TArgs = t_wrapper<hrw::utils::tuple_concat_t<TAnnex, typename thw::th_factory::t_targs>>;

  register_cdecl(m, CArgs());
  register_tdecl(py_ctx_tm, TArgs());
}

template<typename py_class, typename hrw_all>
void register_theories(py::module &m, py_class& py_ctx_tm, hrw_all w) {
  // register the theories in the module in parameter
  std::apply(
    [&m, &py_ctx_tm, &w]( auto const&... thws) {
      ((register_theory(m, py_ctx_tm, w, thws)), ...);
    }, typename hrw_all::th_wrappers());
}



//////////////////////////////////////////
// 3. pickle functions

template<typename hrw_all_arg> class __attribute__ ((visibility("hidden"))) tt_term_dumps {
public:
  using type = tt_term_dumps<hrw_all_arg>;
  using hrw_all = hrw_all_arg;
  using t_ctx_th = typename hrw_all::t_ctx_th;
  using t_term_full = typename hrw_all::t_term_full;
  using t_term_full_ref = typename hrw_all::t_term_full_ref;
  using t_variable = typename hrw_all::t_variable;
  using th_wrappers = typename hrw_all::th_wrappers;

  tt_term_dumps(): m_map(), m_varcount(0), m_transfert(std::bind(&type::translate, std::ref(*this), std::placeholders::_1)) {}

  tt_term_dumps(const tt_term_dumps&) = delete;

  const py::tuple translate(const t_term_full_ref t) {
    auto it = this->m_map.find(t);
    // std::cout << "it == this->m_map.end() = " << (it == this->m_map.end()) << std::endl;
    if(it == this->m_map.end()) {
      // std::cout << "    this = " << this << ", t = " << t << std::endl;
      py::tuple res = std::visit(*this, t->m_content); // need to deal with variables too
      // this->m_map.insert(std::make_pair(std::move(t), std::move(res)));
      this->m_map.insert(std::make_pair(t, res));
      it = this->m_map.find(t);
    }
    return it->second;
  }

  template<typename T>
  const py::tuple operator()(const T& t_inner) {
    using t_core = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr(std::is_same_v<t_core, t_variable>) {
      return py::make_tuple(0, t_inner.get_spec().get_regexp());
    } else {
      constexpr std::size_t idx = t_term_full::template sterm_index_v<t_core>;
      using th = typename t_ctx_th::template theory_element_t<idx>; // ok this is indeed the theory
      return py::make_tuple(idx+1, std::tuple_element_t<idx, th_wrappers>::th_factory::dumps(t_inner, this->m_transfert));
    }
  }

private:
  using t_term_to_tuple = std::unordered_map<t_term_full_ref, py::tuple>;
  t_term_to_tuple m_map;
  std::size_t m_varcount;
  std::function<const py::tuple(const t_term_full_ref)> m_transfert;
};


template<typename hrw_all_arg> class __attribute__ ((visibility("hidden"))) tt_term_loads {
public:
  using type = tt_term_loads<hrw_all_arg>;
  using hrw_all = hrw_all_arg;
  using t_ctx_th = typename hrw_all::t_ctx_th;
  using t_ctx_tm = typename hrw_all::t_ctx_tm;
  using t_term_full = typename hrw_all::t_term_full;
  using t_term_full_ref = typename hrw_all::t_term_full_ref;
  using t_variable = typename hrw_all::t_variable;
  using th_wrappers = typename hrw_all::th_wrappers;


  tt_term_loads(t_ctx_tm& reg): m_map(), m_reg(reg) {}

  t_term_full_ref translate(const py::tuple tuple) {
    auto it = this->m_map.find(tuple);
    if(it == this->m_map.end()) {
      t_term_full_ref res;
      std::size_t idx = tuple[0].cast<std::size_t>();
      if(idx == 0) { // variable
        res = this->m_reg.create_vterm(tuple[1].cast<std::string>());
      } else {
        const py::tuple& t2 = tuple[1].cast<py::tuple>();
        std::apply(
          [&](auto const& ... args) {
            std::size_t n = 1;
            (this->manage_theory(args, n++, idx, t2, res), ...);
          }, th_wrappers());
      }
      this->m_map.insert(std::make_pair(tuple,res));
      it = this->m_map.find(tuple);
    }
    return it->second;
  }

private:
  struct my_hash { std::size_t operator()(const py::object& o) const { return py::hash(o); } };
  using t_tuple_to_term = std::unordered_map<py::tuple, t_term_full_ref, my_hash>;
  t_tuple_to_term m_map;
  t_ctx_tm& m_reg;

  template<typename th_wrapper>
  void manage_theory(th_wrapper, std::size_t idx_local, std::size_t idx_real, const py::tuple tuple, t_term_full_ref& res) {
    using th = typename th_wrapper::t_theory;

    if(idx_local == idx_real) { // we are on the correct theory
      auto params = th_wrapper::th_factory::loads(tuple, std::bind(&type::translate, *this, std::placeholders::_1));
      std::apply([&](auto& ... args) {
        res = this->m_reg.create_sterm(t_constructor_core<th>(params.first), args...); 
      }, params.second); 
    }
  }
};


//////////////////////////////////////////
// 4. ensure correct rw rules definition

std::size_t _get_length(std::size_t s) {
  std::size_t res= s >> 3;
  if((s & 7) > 0) { res += 1; } 
  return res;
}

template<typename t_variable>
struct t_vbind_check_lhs_visitor {
  std::vector<bool>& m_content;
  t_vbind_check_lhs_visitor(std::vector<bool>& content): m_content(content) {}

  template<typename T>
  void operator()(T const & tc)  {
    using t_core = typename std::decay_t<decltype(tc)>;
    // std::cout << "LHS parsing " << hrw::utils::type_name<t_core>() << std::endl;
    if constexpr(has_container_v<t_core>) {
      for(auto tsub: tc) {
        tsub->visit(*this);
      }
    } else if constexpr(std::is_same_v<t_core, t_variable>) {
      // std::cout << "LHS found: " << tc.get_id() << std::endl;
      this->m_content[tc.get_id()] = false;
    }
  }
};

template<typename t_variable>
struct t_vbind_check_rhs_visitor {
  std::vector<bool>& m_content;
  t_vbind_check_rhs_visitor(std::vector<bool>& content): m_content(content) {}

  template<typename T>
  void operator()(T const & tc)  {
    using t_core = typename std::decay_t<decltype(tc)>;
    // std::cout << "RHS parsing " << hrw::utils::type_name<t_core>() << std::endl;
    if constexpr(has_container_v<t_core>) {
      for(auto tsub: tc) {
        tsub->visit(*this);
      }
    } else if constexpr(std::is_same_v<t_core, t_variable>) {
      // std::cout << "RHS found: " << tc.get_id() << std::endl;
      this->m_content[tc.get_id()] = true;
    }
  }
};


template<typename t_variable, typename t_term_full_ref>
bool vbind_check(t_term_full_ref lhs, t_term_full_ref rhs) {
  // std::cout << "CALLING vbind_check" << std::endl;
  // std::size_t bsize = _get_length(t_variable::get_counter());
  std::size_t bsize = t_variable::get_counter();
  std::vector<bool> vars(bsize, false);

  t_vbind_check_lhs_visitor<t_variable> lhs_visitor(vars);
  t_vbind_check_rhs_visitor<t_variable> rhs_visitor(vars);

  rhs->visit(rhs_visitor);
  lhs->visit(lhs_visitor);

  for(bool b: vars) {
    if(b) { return false; }
  }
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// MAIN API
/////////////////////////////////////////////////////////////////////////////

template<typename t_ctx_th>
py::list translate_sortset(const typename t_ctx_th::t_context_sort::natset& s) {
  py::list res;
  for(auto sid: s) {
    // res.append(t_ctx_th::get_sort_name(sid));
    res.append(sid);
  }
  return res;
}

template<
  bool rw_strict_sorting,
  e_strategy rw_strategy,
  template<typename tt, const tt& arg> typename t_vparser,
  typename ... th_apis
>
void register_hrw(py::module &m) {
  using hrw_all = hconstruct<t_vparser, th_apis...>;
  using t_term_dumps = tt_term_dumps<hrw_all>;
  using t_term_loads = tt_term_loads<hrw_all>;

  using t_ctx_th = typename hrw_all::t_ctx_th;
  using t_ctx_tm = typename hrw_all::t_ctx_tm;
  using t_ctx_rw = typename hrw_all::t_ctx_rw;
  using t_variable = typename hrw_all::t_variable;

  using t_term_full         = typename hrw_all::t_term_full;
  using t_term_full_ref     = typename hrw_all::t_term_full_ref;
  using t_term_full_wrapper = typename hrw_all::t_term_full_wrapper;

  using t_substitution         = typename hrw_all::t_substitution;
  using t_substitution_hash    = typename t_substitution::template t_hash<false>;
  using t_substitution_equal   = typename t_substitution::template t_eq<false>;

  using t_print = typename hrw_all::t_print;

  static t_ctx_tm term_registry;
  // static t_term_dumps term_dumps;
  // static t_term_loads term_loads(term_registry);


  //////////////////////////////////////////
  // 0. sorts
  m.def("add_sort", static_cast<t_sort_id (*)(const std::string&)>(&t_ctx_th::add_sort));
  m.def("get_sort_name", static_cast<const std::string& (*)(t_sort_id)>(&t_ctx_th::get_sort_name));
  m.def("add_subsort", static_cast<void (*)(const std::string&, const std::string&)>(&t_ctx_th::add_subsort));
  m.def("is_subsort", static_cast<bool (*)(const std::string&, const std::string&)>(&t_ctx_th::is_subsort));
  m.def("is_subsort", static_cast<bool (*)(const t_sort_id, const t_sort_id)>(&t_ctx_th::is_subsort));

  m.def("contains_sort", static_cast<bool (*)(const std::string&)>(&t_ctx_th::contains_sort));
  m.def("get_sort_id", static_cast<t_sort_id (*)(const std::string&)>(&t_ctx_th::get_sort_id));
  m.def("get_subsorts", [](const std::string& sort) { return translate_sortset<t_ctx_th>(t_ctx_th::get_subsorts(sort)); });
  m.def("get_subsorts", [](const t_sort_id sort) { return translate_sortset<t_ctx_th>(t_ctx_th::get_subsorts(sort)); });

  // clearing all declaration: WARNING: all terms in ctx_rw are invalid pointers after calling this function
  m.def("clear", []() { t_ctx_th::clear(); term_registry.clear(); });


  //////////////////////////////////////////
  // 1. term
  py::class_<t_term_full_wrapper> (m, "t_term")
  .def("__repr__", [](t_term_full_wrapper _this) {
    t_print p;
    std::ostringstream oss;
    oss << p.print(_this.m_content);
    return oss.str();
  })
  .def("is_ground", [](t_term_full_wrapper _this) { return _this.m_content->is_ground(); })
  .def("get_sort", [](t_term_full_wrapper _this) { return _this.m_content->get_sort(); })
  .def("get_spec", [](t_term_full_wrapper _this) { return _this.m_content->get_spec(); })
  .def("get_constructor", [](t_term_full_wrapper _this) { return _this.m_content->get_constructor(); })
  .def("get_constructor_key", [](t_term_full_wrapper _this) { return _this.m_content->get_constructor_key(); })
  .def("match", [](t_term_full_wrapper _this, t_term_full_wrapper t) { return _this.m_content->match(t.m_content); })
  .def("__eq__", [](t_term_full_wrapper _this, py::object t) { // need to define equality with any object
    using t_eq_ref = typename t_term_full::template t_eq_ref<false>;
    std::string cls_name = t.attr("__class__").attr("__name__").cast<std::string>();
    if(cls_name == "cs_wrapper") {// ad-hoc hook with an equivalent pure python class
      t = t.attr("m_data__");
      cls_name = t.attr("__class__").attr("__name__").cast<std::string>();
    }

    if(cls_name == "t_term") { // equality between terms
      return t_eq_ref()(_this.m_content, t.cast<t_term_full_wrapper>().m_content);
    } else {
      return false;
    }
  })
  .def("__hash__", [](t_term_full_wrapper _this) -> std::size_t {
    using t_hash_ref = typename t_term_full::template t_hash_ref<false>;
    return t_hash_ref()(_this.m_content);
  })
  .def("get_gid", [](t_term_full_wrapper _this) -> std::size_t {
    using t_hash_ref = typename t_term_full::template t_hash_ref<true>;
    return t_hash_ref()(_this.m_content);
  })
  .def(py::pickle(
    [](t_term_full_wrapper _this) { return t_term_dumps().translate(_this.m_content); },
    [](py::tuple t) { return t_term_full_wrapper{t_term_loads(term_registry).translate(t)}; }
  ));

  m.def("is_term_variable", [](t_term_full_wrapper _this) { return (std::get_if<t_variable>(&(_this.m_content->m_content)) != nullptr); });
  m.def("get_subterms", [](t_term_full_wrapper _this) {
    py::list subterms;
    std::visit([&subterms](auto& t) {
      using t_term = decltype(t);
      using t_term_core = std::remove_cv_t<std::remove_reference_t<t_term>>;
      if constexpr(has_container_v<t_term_core>) {
        for (auto subterm : t) {
          subterms.append(t_term_full_wrapper{subterm});
        }      
      }
    }, _this.m_content->m_content);
    return subterms;
  });


  //////////////////////////////////////////
  // 2. substitution
  py::class_<t_substitution> (m, "t_substitution")
  .def(py::init<>())
  .def(py::init<t_substitution const &>())
  .def("__repr__", [](t_substitution& _this) {
    t_print p;
    std::ostringstream oss;
    oss << p.print(_this);
    return oss.str();
  })
  .def("add", [](t_substitution& _this, t_term_full_wrapper var, py::object im) {
    t_variable const * v = var.m_content->template get_if<t_variable>();
    if(v != nullptr) {
      std::string cls_name = im.attr("__class__").attr("__name__").cast<std::string>();
      if(cls_name == "_cs_wrapper") {// ad-hoc hook with an equivalent pure python class
        im = im.attr("m_data__");
        cls_name = im.attr("__class__").attr("__name__").cast<std::string>();
      }
      if(cls_name == "t_term") {
        using iterator = typename hrw_all::t_term_full::single_iterator;
        _this.insert(v, iterator(im.cast<t_term_full_wrapper>().m_content), iterator());
      } else {
        throw hrw::exception::generic("ERROR: image is not a term (found type \"" + cls_name +"\")");
      }
    } else {
      t_print p;
      std::ostringstream oss;
      oss << "ERROR: term \"" << (p.print(var.m_content)) << "\" is not a variable";
      throw hrw::exception::generic(oss.str());
    }
  })
  .def("__eq__", [](t_substitution& _this, t_substitution& other) { return t_substitution_equal()(_this, other); })
  .def("__hash__", [](t_substitution& _this) { return size_t(t_substitution_hash()(_this)); })
  ;


  //////////////////////////////////////////
  // 3. term context
  auto py_ctx_tm = py::class_<t_ctx_tm> (m, "context_term")
  .def(py::init<>())
  // .def("__repr__", [](t_ctx_tm* _this) {
  //   std::ostringstream oss;
  //   oss << *_this;
  //   return oss.str();
  // })
  // terms
  .def("create_variable", [](t_ctx_tm* _this, std::string& s) { return t_term_full_wrapper{_this->create_vterm(s)}; })
  .def("instantiate", [](t_ctx_tm* _this, t_term_full_wrapper t, t_substitution & subst) {
    return t_term_full_wrapper{_this->instantiate(t.m_content, subst)};
  })
  .def("clear", &t_ctx_tm::clear)
  ;

  m.attr("term_registry") = &term_registry;


  //////////////////////////////////////////
  // 4. rewrite context
  m.def("check_rule", [](t_term_full_wrapper pattern, t_term_full_wrapper image) { return vbind_check<t_variable, t_term_full_ref>(pattern.m_content, image.m_content); });

  auto py_ctx_rw = py::class_<t_ctx_rw> (m, "context_rw")
    .def(py::init<t_ctx_tm&>())
    .def("add", [](t_ctx_rw* _this, t_term_full_wrapper pattern, t_term_full_wrapper image) { _this->template add<rw_strict_sorting>(pattern.m_content, image.m_content); })
    .def("add", [](t_ctx_rw* _this, t_term_full_wrapper pattern, t_term_full_wrapper image, typename t_ctx_rw::t_guard guard) { _this->template add<rw_strict_sorting>(pattern.m_content, image.m_content, guard); })
    .def("add", [](t_ctx_rw* _this, t_ctx_rw const & other) { _this->add(other); })
    .def("clear", &t_ctx_rw::clear)
    .def("clear_nf", &t_ctx_rw::clear_nf)
    .def("rewrite", [](t_ctx_rw* _this, t_term_full_wrapper t) { return t_term_full_wrapper{_this->template rewrite<rw_strategy>(t.m_content)}; })
    .def("get_count", &t_ctx_rw::get_rw_count)
    ;


  //////////////////////////////////////////
  // 5. conclude
  using th_free_construct = hrw::exception::th_free_construct<t_term_full_ref>;
  static py::exception<th_free_construct> term_error_exception(m, "TermError");
  py::register_exception_translator([](std::exception_ptr p) {
    try {
      if(p) { std::rethrow_exception(p); }
    } catch (th_free_construct const & e) {
      term_error_exception(e.get_spec_got().c_str());
    }
  });

  register_theories(m, py_ctx_tm, hrw_all());
}


#endif // __OPGRAPH_TALGEBRA_PYBIND_H__

