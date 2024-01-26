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
#if ENABLE_UTILS_HASH

#include "doctest/doctest.h"
#include "tests/common/debug.hpp"

#include "hrewrite/utils/hash.hpp"

using namespace hrw::utils;



TEST_CASE("hash wrapper - mutable") {
  OUTPUT("==================================================================");
  OUTPUT("= hash wrapper - mutable");

  using H1 = hash<int>;
  using H2 = hash<double>;
  using H3 = hash<std::string>;

  const int         data_int    = 0;
  const double      data_double = 0.0;
  const std::string data_string = "0";


  H1()(data_int);
  H2()(data_double);
  H3()(data_string);

  using H4 = hash_combine<std::pair<H1, H2>>;
  H4()(std::make_pair(data_int, data_double));

  using H5 = hash_combine<std::vector<H3>>;
  H5()(std::vector({data_string}));

  using H6 = hash_combine<std::tuple<H1, H2, H3>>;
  H6()(std::make_tuple(data_int, data_double, data_string));

  using H7 = hash_combine<std::variant<H1, H2, H3>>;
  H7()(data_string);
}

TEST_CASE("hash wrapper - immutable") {
  OUTPUT("==================================================================");
  OUTPUT("= hash wrapper - immutable");

  using H1 = hash<int>;
  using H2 = hash<double>;
  using H3 = hash<std::string>;

  const int         data_int    = 0;
  const double      data_double = 0.0;
  const std::string data_string = "0";


  H1()(data_int);
  H2()(data_double);
  H3()(data_string);

  using H4 = hash_combine<std::pair<const H1, const H2>>;
  H4()(std::make_pair(data_int, data_double));

  // using H5 = hash_combine<std::vector<H3>>;
  // H5()(std::vector({data_string}));

  using H6 = hash_combine<std::tuple<H1, H2, H3>>;
  H6()(std::make_tuple(data_int, data_double, data_string));

  using H7 = hash_combine<std::variant<H1, H2, H3>>;
  H7()(data_string);

}


#endif

