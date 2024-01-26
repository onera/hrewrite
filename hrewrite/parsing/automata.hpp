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


#ifndef __HREWRITE_PARSING_AUTOMATA_H__
#define __HREWRITE_PARSING_AUTOMATA_H__

#include <vector>
#include <string>
#include <variant>
#include <unordered_map>

#include <iostream>
#include <cassert>


#include "hrewrite/parsing/core.hpp"
#include "hrewrite/utils.hpp"


namespace hrw {
  namespace utils {


    template<typename natset, typename natset_core, typename targ_alphabet, const targ_alphabet& arg_alphabet>
    class automata {
    public:
      using type = automata<natset, natset_core, targ_alphabet, arg_alphabet>;

      using t_state = natset;
      using t_state_set = t_state;
      // using t_state = hrw::utils::natset_fixed<false>;
      // using t_state_set = hrw::utils::natset_fixed<false>;

      using t_alphabet = targ_alphabet;
      using t_letter = typename t_alphabet::t_letter;
      using t_letter_set = typename t_alphabet::t_letter_set;

      static inline constexpr const targ_alphabet& alphabet = arg_alphabet;

      // constructor
      automata(const std::string& s);
      ~automata() = default;

      // core functionalities
      t_state default_state() const {return t_state(this->m_final.size()); }
      t_state start() const { return t_state(this->m_final.size(), { 0 }); }
      bool is_final(const t_state& state) const {
        for(const auto sid: state) {
          if(this->m_final[sid]) { return true; }
        }
        return false;
      }
      bool is_error(const t_state& state) const { return state.empty(); }
      bool next(const t_letter c, const t_state& start, t_state& end) const;
      void nexts(const t_state& state, t_letter_set& set) const;

      const std::string& get_regexp() const { return this-> m_regexp; }

      // operators and friend functions
      bool operator==(const type& other) const { return this == &other; }

      friend std::ostream& operator<<(std::ostream& os, const type& parser) {
        using _w_finals = data_wrapper<const std::vector<unsigned int>&>;
        using _w_ranges = data_wrapper<const t_range_container&>;
        using _w_transitions = data_wrapper<const t_transition_container&>;
        return (os << "automata {\n"
          << "  regexp=\"" << parser.get_regexp() << "\"\n"
          << "  start=" << parser.start() << "\n"
          << "  final=" << _w_finals(parser.m_final) << "\n"
          << "  states=" << _w_ranges(parser.m_states) << "\n"
          << "  transitions=" << _w_transitions(parser.m_transitions) << "\n"
          << "}");
      }

      friend void swap(type& p1, type& p2) {
        using std::swap;
        swap(p1.m_final, p2.m_final);
        swap(p1.m_states, p2.m_states);
        swap(p1.m_transitions, p2.m_transitions);
        swap(p1.m_regexp, p2.m_regexp);
      }

      static const t_parser_trigger trigger;
      static constexpr parsing_complexity complexity = parsing_complexity::FULL;

      const t_letter& get_letter() const {
        unsigned int trans_begin = this->m_states[0];
        unsigned int trans_end = this->m_states[1];
        if(trans_end == (trans_begin+1)) {
          const auto& transition = this->m_transitions[trans_begin];
          if(this->m_final[transition.second]) { return transition.first; }
        }
        throw hrw::exception::spec_get_letter(this->get_regexp());
      }

    private:
      using t_transition = std::pair<t_letter, unsigned int>;
      using t_transition_container = std::vector<t_transition>;
      // using t_range = std::pair<unsigned int, unsigned int>;
      // using t_range_container = std::vector<t_range>;
      using t_range_container = std::vector<unsigned int>;
      // natset_core m_final;
      std::vector<unsigned int> m_final;
      t_range_container m_states;
      t_transition_container m_transitions;

      std::string m_regexp;

    };


    template<typename natset, typename natset_core> struct tt_automata {
      template<typename targ_alphabet, const targ_alphabet& arg_alphabet> using type = automata<natset, natset_core, targ_alphabet, arg_alphabet>;
    };


    /////////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION
    /////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////
    // automata

    template<typename natset, typename natset_core> class tt_automata_constructor;
    template<typename ... Args> using map = std::unordered_map<Args...>;

    template<typename natset, typename natset_core, typename targ_alphabet, const targ_alphabet& alphabet>
    automata<natset, natset_core, targ_alphabet, alphabet>::automata(const std::string& s): m_regexp(s) {
      using automata_constructor = tt_automata_constructor<natset, natset_core>;
      automata_constructor working(s);
      graph<typename automata_constructor::t_state, char, set<std::string>>& wg = working.get_graph();
      natset_core& wfinal = working.get_final();

      map<typename automata_constructor::t_state, unsigned int> state_ids;
      std::vector<typename automata_constructor::t_graph::dfs_iterator> state_list;
      state_list.reserve(wg.size());

      // create the state mapping and fill the m_final set
      unsigned int sid = 0;
      unsigned int nb_transitions = 0;
      for (auto it = wg.dfs_node(working.get_init()); it != wg.end(); ++it) {
        if(it.direction() == t_direction::enter) {
          // std::cout << " found node " << it.id() << "[" << (*it).nb_nexts() << ", " << (*it).nb_nexts() << "] -> " << sid << std::endl;
          state_list.push_back(it);
          state_ids.emplace(std::make_pair(it.id(), sid));
          ++sid;
          nb_transitions += (*it).nb_nexts();
        }
      }

      unsigned int nb_states = sid;

      // allocate the transition ref table
      this->m_final.resize(nb_states);
      this->m_states.resize(nb_states + 1);
      this->m_transitions.reserve(nb_transitions);

      // create the transition table
      unsigned int current_transition = 0;
      unsigned int current_state = 0;
      this->m_states[current_state] = current_transition;
      for (auto s: state_list) {
        // std::cout << " looking at node " << s->id() << "[" << current_state << ", " << state_ids[s->id()] << "]" << std::endl;
        for (auto ite = s->begin(); ite != s->end(); ++ite) {
          for(std::string sname: ite.data()) {
            this->m_transitions.emplace_back(std::make_pair(alphabet.get_letter(sname), state_ids[ite.next()]));
            ++current_transition;
          }
        }
        if(wfinal.contains(s->id())) {
          this->m_final[current_state] = 1;
        }
        this->m_states[current_state+1] = current_transition;
        ++current_state;
      }
    }

    template<typename natset, typename natset_core, typename targ_alphabet, const targ_alphabet& alphabet>
    bool automata<natset, natset_core, targ_alphabet, alphabet>::next(const t_letter c, const t_state& start, t_state& end) const {
      end.clear();
      bool is_final = false;

      for(unsigned int sid : start) {
        unsigned int trans_begin = this->m_states[sid];
        unsigned int trans_end = this->m_states[sid+1];
        for(unsigned int idx = trans_begin; idx != trans_end; ++idx) {
          const auto& transition = this->m_transitions[idx];
          // std::cout << " transition " << transition.first << " -> " << transition.second << " vs " << c << std::endl;
          if(alphabet.is_subletter(c, transition.first)) { // WARNING: is_subletter must be reflexive
            end.add(transition.second);
            if(this->m_final[transition.second]) {
              is_final = true;
            }
          }
        }
      }
      return is_final;
    }

    template<typename natset, typename natset_core, typename targ_alphabet, const targ_alphabet& alphabet>
    void automata<natset, natset_core, targ_alphabet, alphabet>::nexts(const t_state& state, t_letter_set& set) const {
      set.clear();
      for (unsigned int sid : state) {
        unsigned int trans_begin = this->m_states[sid];
        unsigned int trans_end = this->m_states[sid+1];
        for(unsigned int idx = trans_begin; idx != trans_end; ++idx) {
          const auto& transition = this->m_transitions[idx];
          set.insert(transition.first);
        }
      }
    }

    template<typename natset, typename natset_core, typename targ_alphabet, const targ_alphabet& alphabet>
    const t_parser_trigger automata<natset, natset_core, targ_alphabet, alphabet>::trigger = &is_regexp_reduced<std::string>;




    //////////////////////////////////////////
    // tt_automata_constructor

    // utility class for the automata construction: hand written regexp grammar -_-'
    template<typename natset, typename natset_core> class tt_automata_constructor {
    public:
      using type = tt_automata_constructor<natset, natset_core>;
      using t_state = unsigned int;
      using wdecl = std::pair<unsigned int, natset>;
      using t_graph = graph<t_state, char, set<std::string>>;

      tt_automata_constructor(const std::string s): m_next_id(0), m_string(s), m_max(s.length()), m_idx(0), _expected(0) {
        this->_skip_blanks();
        if (this->_is_end()) { // empty automata
          t_state n_id = this->new_state();
          this->m_graph.add_node(n_id, 0);
          wdecl d = std::make_pair(n_id, natset{n_id});
          this->m_init = d.first;
          this->m_final = static_cast<natset_core>(d.second);
        } else {
          wdecl d = this->_regexp_or();
          if (this->m_idx < this->m_max) {
            throw hrw::exception::spec_invalid_char_pos(s, this->_expected, this->m_idx);
          }
          this->m_init = d.first;
          this->m_final = static_cast<natset_core>(d.second);
        }
      }

      ~tt_automata_constructor() = default;

      t_graph& get_graph() { return this->m_graph; }

      t_state get_init() { return this->m_init; }
      // natset& get_final() { return this->m_final; }
      natset_core& get_final() { return this->m_final; }

    private:
      t_graph m_graph;
      t_state m_init;
      natset_core m_final;

      t_state m_next_id;
      t_state new_state() { return this->m_next_id++; }

      std::string m_string;
      unsigned int m_max;
      unsigned int m_idx;
      char _expected;
      char current() { return this->m_string[this->m_idx]; }
      void next() { ++(this->m_idx); }

      wdecl _regexp_or() {
        // std::cout << "tt_automata_constructor::_regexp_or" << std::endl;
        wdecl a1 = this->_regexp_concat();
        if (!this->_is_end_or()) {
          this->next(); // pass the bnfor character
          this->_skip_blanks();
          wdecl a2 = this->_regexp_or();
          for (auto e : this->m_graph[a2.first]) {
            this->add(a1.first, e.next(), e.data());
          }
          a1.second.add(a2.second);
        }
        return a1;
      }

      wdecl _regexp_concat() {
        // std::cout << "tt_automata_constructor::_regexp_concat" << std::endl;
        wdecl a1 = this->_regexp_postfix();
        // std::cout << "   => is_end: " << this->_is_end() << "vs " << this->m_idx << " < " << this->m_max << std::endl;
        if (!this->_is_end_concat()) { // we start a new postfix right away
          wdecl a2 = this->_regexp_concat();
          for (unsigned int final : a1.second) {
            for (auto e : this->m_graph[a2.first]) {
              this->add(final, e.next(), e.data());
            }
          }
          if(a2.second.contains(a2.first)) { a2.second.add(a1.second); }
          a1.second = a2.second;
        }
        return a1;
      }

      wdecl _regexp_postfix() {
        // std::cout << "tt_automata_constructor::_regexp_postfix" << std::endl;
        wdecl res = this->_regexp_core(); // should be _regexp_interval
        while (!this->_is_end_concat()) {
          if (this->current() == bnfstar) {
            for (unsigned int final : res.second) {
              for (auto e : this->m_graph[res.first]) {
                this->add(final, e.next(), e.data());
              }
            }
            res.second.add(res.first);
          } else if (this->current() == bnfplus) {
            for (unsigned int final : res.second) {
              for (auto e : this->m_graph[res.first]) {
                this->add(final, e.next(), e.data());
              }
            }
          } else if (this->current() == bnfopt) {
            res.second.add(res.first);
          } else { // no postfix
            break;
          }
          this->next();
          this->_skip_blanks();
        }
        return res;
      }

      wdecl _regexp_core() {
        // std::cout << "tt_automata_constructor::_regexp_core" << std::endl;
        wdecl res;
        this->_skip_blanks();
        if (this->current() == bnf_begin) { // inner automata
          this->next();
          res = this->_regexp_or();
          if (this->current() == bnf_end) {
            this->next();
            this->_skip_blanks();
          }
          else { // problem
            this->_expected = bnf_end;
          }
        }
        else { // atom expected
          if (!is_char_special(this->current())) {
          // if (isalpha(this->current()) || (this->current() == '_')) {
            // construct the automata
            std::string atom = this->_get_atom();
            unsigned int a_start = this->new_state();
            unsigned int a_end = this->new_state();
            this->add(a_start, a_end, { atom });
            res = std::make_pair(a_start, natset(a_end+1, { a_end }));
          }
          else { // problem
            this->_expected = bnf_begin;
          }
        }
        return res;
      }

      void _skip_blanks() {
        while ((!this->_is_end()) && is_char_separator(this->current())) {
          this->next();
        }
      }

      bool _is_end() { return (this->m_idx >= this->m_max) || (this->_expected != 0); }
      bool _is_end_or() { return this->_is_end() || (this->current() == bnf_end); }
      bool _is_end_concat() { return this->_is_end_or() || (this->current() == bnfor); }

      int _get_int()  {
        unsigned int atom_start = this->m_idx;
        unsigned int atom_len = 0;
        while ((this->m_idx < this->m_max) && (isdigit(this->current()) || (this->current() == '-'))) {
          this->next();
          atom_len++;
        }
        this->_skip_blanks();
        if (this->m_idx == atom_start) { // probably an error
          return 0;
        }
        else {
          return std::stoi(this->m_string.substr(atom_start, atom_len));
        }
      }

      std::string _get_atom()  {
        // std::cout << "tt_automata_constructor::_get_atom" << std::endl;
        unsigned int atom_start = this->m_idx;
        unsigned int atom_len = 0;
        while ((!this->_is_end()) && (isalnum(this->current()) || (this->current() == '_'))) {
          this->next();
          ++atom_len;
        }
        this->_skip_blanks();
        // std::cout << "found atom: begin=" << atom_start << ", end=" << this->m_idx << ", length=" << atom_len << std::endl;
        return this->m_string.substr(atom_start, atom_len);
      }

      void add(const unsigned int nid1, const unsigned int nid2, set<std::string>labels) {
        // std::cout << "ADD transition (" << nid1 << "," << nid2 << ") -> { ";
        // for (std::string s : labels) {
        //   std::cout << s << " ";
        // }
        // std::cout << "}" << std::endl;

        auto it_n1 = this->m_graph.find(nid1);
        if (it_n1 == this->m_graph.end()) {
          this->m_graph.add_node(nid1, 0);
        }
        auto it_n2 = this->m_graph.find(nid2);
        if (it_n2 == this->m_graph.end()) {
          this->m_graph.add_node(nid2, 0);
        }
        auto it = this->m_graph.find(nid1, nid2);
        if (it == this->m_graph.eend()) {
          this->m_graph.add_edge(nid1, nid2, labels);
        }
        else {
          it.data().insert(labels.begin(), labels.end());
        }
      }

    };



  }
} // end namespace




#endif // __HREWRITE_PARSING_AUTOMATA_H__

