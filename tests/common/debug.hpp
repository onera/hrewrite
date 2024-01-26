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

#ifndef __HREWRITE_TEST_DEBUG_H__
#define __HREWRITE_TEST_DEBUG_H__


#include <iostream>

template<typename ... Ts>
void _detail_print(const Ts& ... args) {
  (std::cout << ... << args);
  std::cout << std::endl;
}

// #define OUTPUT(...) _detail_print(__VA_ARGS__)
#define OUTPUT(...) INFO(__VA_ARGS__)



#endif // __HREWRITE_TEST_DEBUG_H__

