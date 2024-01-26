#
# This file is part of the hrewrite library.
# Copyright (c) 2021 ONERA.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

# Author: Michael Lienhardt
# Maintainer: Michael Lienhardt
# email: michael.lienhardt@onera.fr


from . import libhrewrite_python as hrw


###############################################################################
# SORT DECLARATION
###############################################################################

def sort(arg): hrw.add_sort(arg)

def sorts(*args):
  for sort in args:
    hrw.add_sort(sort)

def subsort(*args):
  p = args[0]
  p_iterable = isinstance(p, (tuple, list, set))
  for n in args[1:]:
    n_iterable = isinstance(n, (tuple, list, set))
    if(p_iterable and n_iterable):
      for p_sub in p:
        for n_sub in n:
          hrw.add_subsort(p_sub, n_sub)
    elif(p_iterable):
      for p_sub in p:
        hrw.add_subsort(p_sub, n)
    elif(p_iterable):
      for n_sub in n:
        hrw.add_subsort(p, n_sub)
    else:
      hrw.add_subsort(p, n)
    p = n
    p_iterable = n_iterable


###############################################################################
# CONSTRUCTOR DECLARATION
###############################################################################

class spec_ext_cls(object):
  __slots__ = ("m_sort__", "m_spec__",)
  def __init__(self, sort, spec):
    self.m_sort__ = sort
    self.m_spec__ = spec

class free(object):
  __slots__ = ("m_spec__", "m_tags__",)
  def __init__(self, spec="", **tags):
    self.m_spec__ = spec
    self.m_tags__ = tags
  def __rshift__(self, sort):
    return spec_ext_cls(sort, self)

class lit(object):
  __slots__ = ("m_tags__",)
  def __init__(self, **tags):
    self.m_tags__ = tags
  def __rshift__(self, sort):
    return spec_ext_cls(sort, self)


_reg_terms_ = hrw.term_registry
_reg_cs_ = {}

class cs_wrapper(object):
  __slots__ = ("m_name__", "m_sort__", "m_spec__", "m_cs_core__", "m_data__",)
  def __init__(self, cs_name, spec_ext):
    global _reg_terms_
    global _reg_cs_

    self.m_name__ = cs_name
    self.m_sort__ = spec_ext.m_sort__
    self.m_spec__ = spec_ext.m_spec__

    if(isinstance(self.m_spec__, lit)):
      self.m_cs_core__ = hrw.add_constructor_literal(self.m_sort__, self.m_name__)
      def m_fun(obj):
        # print(f"_reg_terms_.create_sterm({self.m_name__}[{type(self.m_cs_core__)}], {obj}[{type(obj)}])")
        # if(obj < 0):
        #   my_res = _reg_terms_.create_sterm(obj, obj)
        my_res = _reg_terms_.create_sterm(self.m_cs_core__, obj)
        return my_res
      self.m_data__ = m_fun
    else:
      if(len(self.m_spec__.m_spec__) == 0):
        self.m_cs_core__ = hrw.add_constructor_leaf(self.m_sort__, self.m_name__)
        self.m_data__ = _reg_terms_.create_sterm(self.m_cs_core__)
      else:
        self.m_cs_core__ = hrw.add_constructor_free(self.m_sort__, self.m_name__, self.m_spec__.m_spec__)
        def m_fun(*args):
          args = tuple((el.data if(isinstance(el, cs_wrapper)) else el) for el in args)
          my_res = _reg_terms_.create_sterm(self.m_cs_core__, args)
          return my_res
        self.m_data__ = m_fun
    _reg_cs_[hrw.get_cs_key(self.m_cs_core__)] = self

  @property
  def tags(self):
    return self.m_spec__.m_tags__
  @property
  def data(self):
    return self.m_data__
  @property
  def name(self):
    return self.m_name__

  def get_sort(self):
    return hrw.get_cs_sort(self.m_cs_core__)

  def get_spec(self):
    if(self.m_spec__.__class__ == free):
      return self.m_spec__.m_spec__
    else: return None

  def get_constructor_key(self):
    return hrw.get_cs_key(self.m_cs_core__)

  def __call__(self, *args):
    return self.m_data__(*args)

  def __str__(self): return str(self.data)
  def __repr__(self): return str(self.data)

  def __eq__(self, rhs):
    if(is_term(rhs)):
      return self.data == rhs
    else:
      return object.__eq__(self, rhs)
  def __hash__(self):
    if(is_term(self.data)): # if it is a leaf
      return hash(self.data)
    else:
      return object.__hash__(self)


def constructor(cs_name, spec_ext): return cs_wrapper(cs_name, spec_ext)

def constructors(**kwargs):
  return tuple(cs_wrapper(cs_name, spec_ext) for cs_name, spec_ext in kwargs.items())


###############################################################################
# VARIABLE DECLARATION
###############################################################################

def var(spec):
  global _reg_terms_
  return _reg_terms_.create_variable(spec)

def vars(*specs):
  global _reg_terms_
  return tuple(_reg_terms_.create_variable(spec) for spec in specs)


###############################################################################
# MANIPULATION API
###############################################################################

##########################################
# get info from sorts

is_subsort = hrw.is_subsort

##########################################
# get info from constructors

is_cs_free = lambda cs: ((cs.spec.__class__ is free) and (len(cs.spec.m_spec__) != 0))
is_cs_lit  = lambda cs: (cs.spec.__class__ is lit)
is_cs_leaf = lambda cs: ((cs.spec.__class__ is free) and (len(cs.spec.m_spec__) == 0))

##########################################
# get info from terms

get_sort = lambda g: hrw.get_sort_name(g.get_sort())
get_spec = lambda g: g.get_spec()
get_constructor_key = lambda g: g.get_constructor_key()

get_subterms = hrw.get_subterms
get_value    = hrw.get_value

is_term_core = lambda t: isinstance(t, hrw.t_term)
is_term_wrapper = lambda t: (isinstance(t, cs_wrapper) and is_term_core(t.data))
is_term = lambda t: (is_term_core(t) or is_term_wrapper(t))

is_term_variable = lambda t: (is_term_core(t) and hrw.is_term_variable(t))
is_term_literal  = lambda t: (is_term_core(t) and hrw.is_term_literal(t))
is_term_leaf     = lambda t: ((is_term_core(t) and hrw.is_term_leaf(t)) or is_term_wrapper(t))
is_term_free     = lambda t: (is_term_core(t) and hrw.is_term_free(t))

is_term_ground = lambda t: ((is_term_core(t) and t.is_ground()) or is_term_wrapper(t))
is_term_structured = lambda t: ((is_term_literal_int(t)) or (is_term_leaf(t)) or (is_term_free(t)))

def get_constructor(t):
  global _reg_cs_
  key = get_constructor_key(t)
  return reg_talgebra_cs[key]

def cs_unwrap(t):
  if(is_term_wrapper(t)): return t.data
  else: return t

def walk_dfs(t, fun_enter, fun_exit):
  t = cs_unwrap(t)
  s = set()
  def walk_dfs_rec(t):
    go_on = fun_enter(t)
    if(go_on and is_term_free(t)):
      for st in get_subterms(t):
        if(st not in s):
          s.add(st)
          walk_dfs_rec(st)
    if(go_on): fun_exit(t)
  walk_dfs_rec(t)

##########################################
# substitution

substitution = hrw.t_substitution

def instantiate(t, substitution):
  if(isinstance(t, cs_wrapper) and is_cs_leaf(t)): t = t.data
  elif(not is_term(t)): raise ValueError(f"ERROR: value {t} (of type {type(t)}) is not a term")
  return hrw.term_registry.instantiate(t, substitution)


###############################################################################
# REWRITE ENGINE
###############################################################################

class rw_engine_cls(object):
  __slots__ = ("m_rw__",)
  def __init__(self):
    global _reg_terms_
    self.m_rw__ = hrw.context_rw(_reg_terms_)
  def add(self, pattern, image, guard=None):
    pattern = cs_unwrap(pattern)
    image = cs_unwrap(image)
    if(guard is None): self.m_rw__.add(pattern, image)
    else: self.m_rw__.add(pattern, image, guard)
  def rewrite(self, t):
    t = cs_unwrap(t)
    return self.m_rw__.rewrite(t)

  def clear_nf(self): self.m_rw__.clear_nf()
  def clear(self): self.m_rw__.clear()


###############################################################################
# CLEAN UP
###############################################################################

def clear():
  global _reg_terms_
  _reg_terms_.clear()
  hrw.clear()

