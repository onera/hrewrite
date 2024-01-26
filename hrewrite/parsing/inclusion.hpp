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


#ifndef __HREWRITE_PARSING_INCLUSION_H__
#define __HREWRITE_PARSING_INCLUSION_H__

#include <iterator>
#include <vector>
#include <string>
#include <variant>
#include <unordered_set>

#include <iostream>

#include "hrewrite/parsing/core.hpp"
#include "hrewrite/utils.hpp"


namespace hrw {
  namespace utils {
    template<typename ... Args>
    using my_set = std::unordered_set<Args...>;


    template<typename P1, typename P2> struct t_inclusion;




    /////////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION
    /////////////////////////////////////////////////////////////////////////////

    template<typename P1, typename P2>
    struct t_inclusion {
      using t_info = t_parser_tuple_consistency<std::tuple<P1, P2>>;
      static_assert(t_info::value);

      using t_alphabet = typename t_info::t_alphabet;
      using t_letter = typename t_alphabet::t_letter;
      using t_letter_set = typename t_alphabet::t_letter_set;
      // using t_letter_map = typename t_alphabet::t_letter_map;

      using P1_state = typename P1::t_state;
      using P2_state = typename P2::t_state;

      using t_element = std::pair<P1_state, P2_state>;
      using t_set = my_set<t_element, hrw::utils::get_hash_t<t_element>, hrw::utils::get_eq_t<t_element>>;

      // check that p1 is included in p2
      static bool check(const P1& p1, const P2& p2) {
        t_set todo({std::make_pair(p1.start(), p2.start())});
        t_set done;

        while(!todo.empty()) {
          // 1. take an element from the todo set
          typename t_set::iterator it = todo.begin();
          auto insert_res = done.insert(std::move(*it));
          todo.erase(it);

          const P1_state& p1_state = insert_res.first->first;
          const P2_state& p2_state = insert_res.first->second;

          // 2. check that if p1_state is final, then so is p2_states
          if(p1.is_final(p1_state) && (!p2.is_final(p2_state))) { return false; }


          // 3. check that the nexts of p1_state are included in the nexts of p2_states, and register next checking
          t_letter_set p1_letter_nexts;
          t_element next(p1.default_state(), p2.default_state());
          p1.nexts(p1_state, p1_letter_nexts);
          for(t_letter l1: p1_letter_nexts) {
            p1.next(l1, p1_state, next.first);
            p2.next(l1, p2_state, next.second);
            if(p2.is_error(next.second)) { return false; }
            if(done.find(next) == done.end()) { todo.insert(std::move(next)); }
          }
        }
        return true;
      }
    };


    /////////////////////////////////////////////////////////////////////////////
    // API
    /////////////////////////////////////////////////////////////////////////////

    template<typename P1, typename P2>
    bool inclusion(P1& p1, P2& p2) { return t_inclusion<P1, P2>::check(p1, p2); }


  }

}


#endif // __HREWRITE_PARSING_INCLUSION_H__

