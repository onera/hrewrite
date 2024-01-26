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


#ifndef __HREWRITE_PARSING_COMBINE_VARIANT_H__
#define __HREWRITE_PARSING_COMBINE_VARIANT_H__

#include <string>
#include <variant>
#include <utility>

#include <iostream>

#include "hrewrite/parsing/core.hpp"
#include "hrewrite/utils.hpp"
#include "hrewrite/exceptions/common.hpp"

namespace hrw {
  namespace utils {

    template<typename T> class combine_core;

    namespace _parsing_detail {

      // flatten the types
      template<typename T> struct _flatten { using type = std::tuple<T>; };
      template<typename ... Ps>
      struct _flatten<combine_core<std::tuple<Ps...>>> { using type = hrw::utils::tuple_concat_t<typename _flatten<Ps>::type...>; };
      template<typename ... Ts>
      struct _flatten_list { using type = hrw::utils::tuple_toset_t<hrw::utils::tuple_concat_t<typename _flatten<Ts>::type...>>; };



      template<typename ... Ps> struct _combine_variant_inner {
        using type = typename _flatten_list<Ps...>::type;
        using t_info = t_parser_tuple_consistency<type>;
        static_assert(t_info::value);
      };
      template<typename ... Ts> using _combine_variant_inner_t = typename _combine_variant_inner<Ts...>::type;


      template<typename ... Ps> struct _combine_sequence_inner {
        using tmp_type1 = typename _flatten_list<Ps...>::type;

        using t_info = t_parser_tuple_consistency<tmp_type1>;
        static_assert(t_info::value);

        using t_alphabet = typename t_info::t_alphabet;
        static inline constexpr const t_alphabet& alphabet = t_info::alphabet;
        using tmp_element = element<t_alphabet, alphabet>;
        using tmp_sequence = sequence<t_alphabet, alphabet>;
        // using tmp_automata = automata<t_alphabet, alphabet>;


        using tmp_type2 = hrw::utils::tuple_remove_t<tmp_element, tmp_type1>;
        template<typename P, typename> using F = std::integral_constant<bool, P::complexity >= hrw::utils::parsing_complexity::SEQUENCE>;
        static constexpr bool value = hrw::utils::tuple_any_v<F, tmp_type1>;
        // static constexpr bool value = hrw::utils::tuple_contains_v<tmp_automata, tmp_type1> || hrw::utils::tuple_contains_v<tmp_sequence, tmp_type1>;

        using type = std::conditional_t<value, tmp_type2, hrw::utils::tuple_concat_t<tmp_type2, std::tuple<tmp_sequence>> >;
      };
      template<typename ... Ts> using _combine_sequence_inner_t = typename _combine_sequence_inner<Ts...>::type;

    }


    /////////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION
    /////////////////////////////////////////////////////////////////////////////

    template<typename ... PARSERS>
    class combine_core<std::tuple<PARSERS...>> {
    public:

      using type = combine_core<std::tuple<PARSERS...>>;
      using t_content_core_tuple = std::tuple<PARSERS...>;
      using t_content_tuple = hrw::utils::tuple_add_t<std::monostate, t_content_core_tuple>;
      using t_alphabet = typename std::tuple_element_t<0, t_content_core_tuple>::t_alphabet;
      static inline constexpr const t_alphabet& alphabet = std::tuple_element_t<0, t_content_core_tuple>::alphabet;

      using t_content = hrw::utils::tuple_convert_t<std::variant, t_content_tuple>;
      template<typename P> using t_state_get = typename P::t_state;
      using t_state =  hrw::utils::tuple_convert_t<std::variant, hrw::utils::tuple_toset_t<
        hrw::utils::tuple_map_t<t_state_get, t_content_core_tuple>
      >>;

      using t_letter = typename t_alphabet::t_letter;
      using t_letter_set = typename t_alphabet::t_letter_set;

      explicit combine_core(const std::string& s): m_content() {
        this->_set_content(std::in_place_type<type>, s);
        if(std::holds_alternative<std::monostate>(this->m_content)) { throw hrw::exception::spec_no_parser(s); }
      }

      explicit combine_core(std::string&& s): m_content() {
        this->_set_content(std::in_place_type<type>, std::move(s));
        if(std::holds_alternative<std::monostate>(this->m_content)) { throw hrw::exception::spec_no_parser(s); }
      }

      template<typename ... Ps>
      combine_core(const combine_core<Ps...>& p) {
        static_assert(hrw::utils::tuple_subset_v<typename combine_core<Ps...>::t_content_core_tuple, t_content_core_tuple>);
        _constructor_helper obj(*this);
        VISIT_SINGLE(obj, p.m_content);
      }
      
    
      ~combine_core() = default;
      // core functionalities
      t_state default_state() const { return VISIT_SINGLE(_default_state_helper(), this->m_content); }
      t_state start() const { return VISIT_SINGLE(_start_helper(), this->m_content); }
      bool is_final(const t_state& state) const { return VISIT_SINGLE(_is_final_helper(state), this->m_content); }
      bool is_error(const t_state& state) const { return VISIT_SINGLE(_is_error_helper(state), this->m_content); }
      bool next(const t_letter c, const t_state& start, t_state& end) const { return VISIT_SINGLE(_next_helper(c, start, end), this->m_content); }
      void nexts(const t_state& state, t_letter_set& set) const { VISIT_SINGLE(_nexts_helper(state, set), this->m_content); }

      const std::string& get_regexp() const { return VISIT_SINGLE(_get_regexp_helper(), this->m_content); }

      // operators and friend functions
      bool operator==(const type& other) const { return this == &other; }

      friend std::ostream& operator<<(std::ostream& os, const type& parser) {
        using t_wrapper = data_wrapper<const t_content&>;
        return (os << t_wrapper(parser.m_content));
      }
      friend void swap(type& p1, type& p2) { using std::swap; swap(p1.m_content, p2.m_content); }

      static const t_parser_trigger trigger;
      static constexpr parsing_complexity complexity = parser_tuple_complexity_v<t_content_core_tuple>;

      const t_letter& get_letter() const {
        return VISIT_SINGLE(hrw::utils::visit_helper {
          [](const std::monostate&) -> const t_letter& { throw hrw::exception::spec_get_letter_no_parser(); },
          [](auto& el) -> const t_letter& { return el.get_letter(); }
        }, this->m_content);
      }


    private:
      t_content m_content;

      template<typename P>
      bool _set_content_inner(std::in_place_type_t<P>, const std::string& s) {
        bool res = P::trigger(s);
        // std::cout << "_manage_string<" << type_name<P>() << "> -> " << std::boolalpha << res << "\n";
        if(res) {
          //this->m_content = t_content(std::in_place_type<typename W::template type<P>>, s);
          this->m_content = t_content(std::in_place_type<P>, s);
        }
        return res;
      }

      template<typename P>
      bool _set_content_inner(std::in_place_type_t<P>, std::string&& s) {
        bool res = P::trigger(s);
        // std::cout << "_manage_string<" << type_name<P>() << "> -> " << std::boolalpha << res << "\n";
        if(res) {
          //this->m_content = t_content(std::in_place_type<typename W::template type<P>>, s);
          this->m_content = t_content(std::in_place_type<P>, s);
        }
        return res;
      }

      template<typename ... _Ps>
      void _set_content(std::in_place_type_t<combine_core<std::tuple<_Ps...>>>, const std::string& s) {
        // std::cout << "_set_content - 2 -<" << type_name<combine_variant<targ_alphabet, _Ps...>>() << ">\n";
        (this->_set_content_inner(std::in_place_type<_Ps>, s) || ...);
      }

      template<typename ... _Ps>
      void _set_content(std::in_place_type_t<combine_core<std::tuple<_Ps...>>>, std::string&& s) {
        // std::cout << "_set_content - 2 -<" << type_name<combine_variant<targ_alphabet, _Ps...>>() << ">\n";
        (this->_set_content_inner(std::in_place_type<_Ps>, std::move(s)) || ...);
      }


      struct _constructor_helper {
        _constructor_helper(type& r): m_r(r) {}
        template<typename T>
        void operator()(T& t) {
          if constexpr(hrw::utils::tuple_contains_v<T, t_content_tuple>) {
            this->m_r.m_content = t;
          }
        }
        type& m_r;
      };

      struct _default_state_helper {
        t_state operator()(std::monostate) { throw hrw::exception::internal(); }
        template<typename P>
        t_state operator()(const P& parser) { return parser.default_state(); }
      };

      struct _start_helper {
        t_state operator()(std::monostate) { throw hrw::exception::internal(); }
        template<typename P>
        t_state operator()(const P& parser) { return parser.start(); }
      };

      struct _is_final_helper {
        _is_final_helper(const t_state& data): m_data(data) {}
        const t_state& m_data;
        bool operator()(std::monostate) { throw hrw::exception::internal(); }
        template<typename P>
        bool operator()(const P& parser) {
          return parser.is_final(std::get<typename P::t_state>(this->m_data));
        }
      };

      struct _is_error_helper {
        _is_error_helper(const t_state& data): m_data(data) {}
        const t_state& m_data;
        bool operator()(std::monostate) { throw hrw::exception::internal(); }
        template<typename P>
        bool operator()(const P& parser) {
          return parser.is_error(std::get<typename P::t_state>(this->m_data));
        }
      };

      struct _next_helper {
        _next_helper(const t_letter c, const t_state& start, t_state& end): m_c(c), m_start(start), m_end(end) {}
        const t_letter m_c;
        const t_state& m_start;
        t_state& m_end;

        bool operator()(std::monostate) { throw hrw::exception::internal(); }

        template<typename P>
        bool operator()(const P& parser) {
          // std::cout << "  called next on combo" << std::endl;
          const typename P::t_state& start = std::get<typename P::t_state>(this->m_start);
          // std::cout << "    first state : " << start << std::endl;
          if(std::get_if<typename P::t_state>(&(this->m_end)) == nullptr) {
            this->m_end = parser.default_state(); // setup the state
          }
          typename P::t_state& end = std::get<typename P::t_state>(this->m_end); // since m_end is a
          // std::cout << "    end state : " << end << std::endl;
          return parser.next(this->m_c, start, end);
        }
      };

      struct _nexts_helper {
        _nexts_helper(const t_state& state, t_letter_set& set): m_state(state), m_set(set) {}
        const t_state& m_state;
        t_letter_set& m_set;

        void operator()(std::monostate) { throw hrw::exception::internal(); }

        template<typename P>
        void operator()(const P& parser) {
          parser.nexts(std::get<typename P::t_state>(this->m_state), this->m_set);
        }
      };

      struct _get_regexp_helper {
        _get_regexp_helper() = default;
        const std::string& operator()(std::monostate) { throw hrw::exception::internal(); }
        template<typename P>
        const std::string& operator()(const P& parser) {
          return parser.get_regexp();
        }
      };

    };

    template<typename ... Ps>
    const t_parser_trigger combine_core<std::tuple<Ps...>>::trigger = [](const std::string& s) { return (Ps::trigger(s) || ...); };



    /////////////////////////////////////////////////////////////////////////////
    // API
    /////////////////////////////////////////////////////////////////////////////

    template<typename targ_alphabet, const targ_alphabet& alphabet, template<typename tt, const tt&> typename ... PARSERS>
    using combine_variant = combine_core<_parsing_detail::_combine_variant_inner_t<PARSERS<targ_alphabet, alphabet>...>>;

    template<typename targ_alphabet, const targ_alphabet& alphabet, template<typename tt, const tt&> typename ... PARSERS>
    using combine_sequence = combine_core<_parsing_detail::_combine_sequence_inner_t<PARSERS<targ_alphabet, alphabet>...>>;

    template<typename ... PARSERS>
    using combine_variant_from_existing = combine_core<_parsing_detail::_combine_variant_inner_t<PARSERS...>>;

    template<typename ... PARSERS>
    using combine_sequence_from_existing = combine_core<_parsing_detail::_combine_sequence_inner_t<PARSERS...>>;




  }
} // end namespace


#endif // __HREWRITE_PARSING_COMBINE_VARIANT_H__

