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

#include "tests/common/activation.hpp"
#if ENABLE_UTILS_ITERATOR

#include "doctest/doctest.h"
#include "tests/common/debug.hpp"


#include "hrewrite/utils/iterator.hpp"

using namespace hrw::utils;


TEST_CASE("iterator_single") {

  int i = 0;
  iterator_single<int> it(&i);
  (*it) = (*it) + 1;
  CHECK(i == 1);
}


#endif

