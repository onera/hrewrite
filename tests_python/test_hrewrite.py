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

import python as hrw


def test_lit():
  hrw.sorts("lit")
  lit = hrw.constructor("lit", hrw.lit() >> "lit")

  obj = object()
  t1 = lit(obj)
  print (hrw.get_value(t1) is obj)

  t2 = lit("hello world")
  print (hrw.get_value(t2) == "hello world")

  t3 = lit(1)
  print (hrw.get_value(t3) == 1)

  # t4 = lit(-1)
  # print (hrw.get_value(t4) == -1)


def test_num():

  ##########################################
  # sort declaration
  hrw.sorts("val", "num")
  hrw.subsort("val", "num")

  ##########################################
  # constructor declaration
  val   = hrw.constructor("val", hrw.lit() >> "val")
  s     = hrw.constructor("s", hrw.free("num") >> "num")
  p     = hrw.constructor("p", hrw.free("num") >> "num")
  inv   = hrw.constructor("p", hrw.free("num") >> "num")
  plus  = hrw.constructor("plus", hrw.free("num num") >> "num")
  minus = hrw.constructor("minus", hrw.free("num num") >> "num")
  sum   = hrw.constructor("sum", hrw.free("num+") >> "num")

  ##########################################
  # rw rules
  rw_eng = hrw.rw_engine_cls()

  alpha = hrw.var("num")
  beta = hrw.var("num")
  gamma = hrw.var("num*")
  valpha, vbeta, vgamma = hrw.vars("val", "val", "val")

  # rules on values
  def val_incr(rw_eng, subst):
    value = hrw.get_value(hrw.instantiate(valpha, subst))
    # print(f"incr({value})")
    subst.add(vbeta, val(value + 1))
    return True
  rw_eng.add(s(valpha), vbeta, val_incr)

  def val_decr(rw_eng, subst):
    value = hrw.get_value(hrw.instantiate(valpha, subst))
    # print(f"decr({value})")
    subst.add(vbeta, val(value - 1))
    return True
  rw_eng.add(p(valpha), vbeta, val_decr)

  def val_inv(rw_eng, subst):
    value = hrw.get_value(hrw.instantiate(valpha, subst))
    # print(f"inv({value})")
    subst.add(vbeta, val(-value))
    return True
  rw_eng.add(inv(valpha), vbeta, val_inv)

  def val_plus(rw_eng, subst):
    value_1 = hrw.get_value(hrw.instantiate(valpha, subst))
    value_2 = hrw.get_value(hrw.instantiate(vbeta, subst))
    # print(f"plus({value_1}, {value_2})")
    subst.add(vgamma, val(value_1 + value_2))
    return True
  rw_eng.add(plus(valpha, vbeta), vgamma, val_plus)

  # rules for the constructors
  rw_eng.add(p(s(alpha)), alpha)
  rw_eng.add(s(p(alpha)), alpha)

  rw_eng.add(inv(p(alpha)), s(inv(alpha)))
  rw_eng.add(inv(s(alpha)), p(inv(alpha)))

  rw_eng.add(plus(s(alpha), beta), plus(alpha, s(beta)))
  rw_eng.add(plus(p(alpha), beta), plus(alpha, p(beta)))

  rw_eng.add(minus(alpha, beta), plus(alpha, inv(beta)))

  rw_eng.add(sum(alpha), alpha)
  rw_eng.add(sum(alpha, beta, gamma), sum(plus(alpha, beta), gamma))

  # t1 = sum(s(p(val(1))), plus(s(val(1)), val(3)), minus(val(2), p(p(s(p(val(0)))))))
  t1 = sum(s(p(val(1))), plus(p(val(3)), val(3)))
  t2 = rw_eng.rewrite(t1)

  assert (str(t1) == "sum(s(p(val[1])), plus(p(val[3]), val[3]))")
  assert (str(t2) == "val[6]")

  assert (hrw.get_sort(sum) == "num")
  assert (hrw.get_sort(t1) == "num")
  assert (hrw.get_spec(gamma) == "num*")
  assert (hrw.get_spec(sum) == "num+")
  assert (hrw.get_spec(val) == None)


  hrw.clear()


def test_interable():
  hrw.sorts("v")
  c, e, l = hrw.constructors(
    c = hrw.free("v*") >> "v",
    e = hrw.free() >> "v",
    l = hrw.lit() >> "v"
  )


  rw_eng = hrw.rw_engine_cls()

  alpha, beta, gamma1, gamma2 = hrw.vars("v*", "v*", "v*", "v*")
  matchings = []

  def guard(rw_eng, subst):
    l1 = hrw.get_subterms(hrw.instantiate(c(alpha), subst))
    l2 = hrw.get_subterms(hrw.instantiate(c(beta), subst))
    res = ((len(l1) == 2) and (len(l2) == 0))
    matchings.append( (l1, l2, res,) )
    return res

  obj = object()

  pattern = c(gamma1, c(alpha, beta), gamma2)
  image = c(alpha)
  rw_eng.add(pattern, image, guard)

  t1 = c(c(e), l(obj), c(e, e, e, e, e), c(e,e), e)
  matchings_expected = [
    ([], [e], False),
    ([e], [], False),
    ([], [e, e, e, e, e], False),
    ([e], [e, e, e, e], False),
    ([e, e], [e, e, e], False),
    ([e, e, e], [e, e], False),
    ([e, e, e, e], [e], False),
    ([e, e, e, e, e], [], False),
    ([], [e, e], False),
    ([e], [e], False),
    ([e, e], [], True)
  ]

  t2 = rw_eng.rewrite(t1)

  assert (matchings == matchings_expected)

  hrw.clear()

