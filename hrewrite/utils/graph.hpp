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

/*
 * It may be better to have a node class that contains their own prev and succ, so the iterators would be simpler to implement
 * For edges, I can have the same wrapper as currently, but I guess it would be better to distinguish between local and global edge iterators
 * Finally, for node iterator, I guess it would be better to have a base class for node_iterator (with no real constructor, and no ++ -- but access yes --, just for position, mainly for 'end'), and the ++ is implemented in sub classes (begin, dfs, etc).
 */


#ifndef __HREWRITE_GRAPH_H__
#define __HREWRITE_GRAPH_H__

#include <iostream>
#include <iterator>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace hrw {
  namespace utils {
    template<typename ... Args>
    using map = std::unordered_map<Args...>;
    template<typename ... Args>
    using map_iterator = typename map<Args...>::iterator;

    template<typename ... Args>
    using set = std::unordered_set<Args...>;
    template<typename ... Args>
    using set_iterator = typename set<Args...>::iterator;


    enum class t_direction { enter, leave }; // for traversal


    template<typename NID, typename NDATA, typename EDATA>
    class graph;

    template<typename NID, typename NDATA, typename EDATA>
    class node;

    template<typename NID, typename NDATA, typename EDATA>
    class edge {
    public:
      const NID& prev() const;
      const NID& next() const;
      EDATA& data();

      bool operator==(edge<NID, NDATA, EDATA> other) const;
      bool operator!=(edge<NID, NDATA, EDATA> other) const;
    private:
      friend class graph<NID, NDATA, EDATA>;
      friend class node<NID, NDATA, EDATA>;
      edge(graph<NID, NDATA, EDATA>& g, map_iterator<NID, map<NID, EDATA>> it_node, map_iterator<NID, EDATA> it_edge) : m_g(g), m_it_node(it_node), m_it_edge(it_edge) {}
      graph<NID, NDATA, EDATA>& m_g;
      map_iterator<NID, map<NID, EDATA>> m_it_node;
      map_iterator<NID, EDATA> m_it_edge;
    };


    template<typename NID, typename NDATA, typename EDATA>
    class node {
    public:
      const NID& id() const;
      NDATA& data();
      bool operator==(node<NID, NDATA, EDATA> other) const;
      bool operator!=(node<NID, NDATA, EDATA> other) const;

      // connexions
      class edge_iterator : public edge<NID, NDATA, EDATA> {
        public:
          using iterator_category = std::input_iterator_tag;
          using value_type = edge<NID, NDATA, EDATA>;
          using difference_type = std::ptrdiff_t;
          using pointer = value_type*;
          using reference = value_type&;
          using const_pointer = value_type const *;
          using const_reference = value_type const &;
          edge_iterator& operator++();
          const_reference operator*() const { return *this; }
          reference operator*() { return *this; }
          const_pointer operator->() const {return this; }
          pointer operator->() { return this; }
        private:
          friend class node;
          edge_iterator(graph<NID, NDATA, EDATA>& g, map_iterator<NID, map<NID, EDATA>> it_node, map_iterator<NID, EDATA> it_edge) : edge<NID, NDATA, EDATA>(g, it_node, it_edge) {}
      };

      unsigned int nb_nexts();
      unsigned int nb_prevs();

      edge_iterator begin();
      edge_iterator find(const NID& nid);
      edge_iterator end();
      edge<NID, NDATA, EDATA> operator[](const NID& nid);

      edge_iterator rbegin();
      edge_iterator rfind(NID& nid);
      edge_iterator rend();

    protected:
      friend class graph<NID, NDATA, EDATA>;
      node(graph<NID, NDATA, EDATA>& g, map_iterator<NID, NDATA> it) : m_g(g), m_it(it) {}
      graph<NID, NDATA, EDATA>& m_g;
      map_iterator<NID, NDATA> m_it;
    };


    template<typename NID, typename NDATA, typename EDATA>
    class traverse_node : public node<NID, NDATA, EDATA> {
    public:
      t_direction direction() const { return this->m_d; }
    protected:
      friend class graph<NID, NDATA, EDATA>;
      traverse_node(graph<NID, NDATA, EDATA>& g, map_iterator<NID, NDATA> it) : node<NID, NDATA, EDATA>(g, it), m_d(t_direction::enter) {}
      t_direction m_d;
    };




    template<typename NID, typename NDATA, typename EDATA>
    class graph {
    public:
      graph();
      graph(graph& g) = delete;

      // graph modification
      void add_node(const NID& nid, const NDATA& data);
      /*
      void add_node(NID&& nid, NDATA& data);
      void add_node(const NID& nid, NDATA&& data);
      void add_node(NID&& nid, NDATA&& data);*/

      void rem_node(const NID& nid);
      
      void add_edge(const NID& prev, const NID& next, const EDATA& data);
      void rem_edge(const NID& prev, const NID& next);

      // graph lookup
      class node_iterator : public node<NID, NDATA, EDATA> {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = node<NID, NDATA, EDATA>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        using const_pointer = value_type const *;
        using const_reference = value_type const &;
        node_iterator& operator++();
        const_reference operator*() const { return *this; }
        reference operator*() { return *this; }
        const_pointer operator->() const {return this; }
        pointer operator->() { return this; }
      private:
        friend class graph<NID, NDATA, EDATA>;
        node_iterator(graph<NID, NDATA, EDATA>& g, map_iterator<NID, NDATA> it) : node<NID, NDATA, EDATA>(g, it) {}
      };

      class dfs_iterator : public traverse_node<NID, NDATA, EDATA> {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = traverse_node<NID, NDATA, EDATA>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        using const_pointer = value_type const *;
        using const_reference = value_type const &;
        graph<NID, NDATA, EDATA>::dfs_iterator& operator++();
        const_reference operator*() const { return *this; }
        reference operator*() { return *this; }
        const_pointer operator->() const {return this; }
        pointer operator->() {return this; }
      private:
        friend class graph;
        dfs_iterator(graph& g, map_iterator<NID, NDATA> it) :
            // traverse_node<NID, NDATA, EDATA>(g, it), m_stack(), m_visited({ it.key() }) {
            traverse_node<NID, NDATA, EDATA>(g, it), m_stack(), m_visited({ it->first }) {
          map_iterator<NID, map<NID, EDATA>> it_node = g.m_nexts.find(this->id());
          map_iterator<NID, EDATA> it_edge;
          if(it_node != g.m_nexts.end()) {
            // it_edge = it_node.value().begin();
            it_edge = it_node->second.begin();
          }
          this->m_stack.push_back({ it_node, it_edge });
        }
        struct TMP { map_iterator<NID, map<NID, EDATA>> it_node; map_iterator<NID, EDATA> it_edge; };
        std::vector<TMP> m_stack;
        set<NID> m_visited;
      };

      class edge_iterator : public edge<NID, NDATA, EDATA> {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = edge<NID, NDATA, EDATA>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        using const_pointer = value_type const *;
        using const_reference = value_type const &;
        edge_iterator& operator++();
        const_reference operator*() const { return *this; }
        reference operator*() { return *this; }
        const_pointer operator->() const {return this; }
        pointer operator->() {return this; }
      private:
        friend class graph;
        edge_iterator(graph<NID, NDATA, EDATA>& g, map_iterator<NID, map<NID, EDATA>> it_node, map_iterator<NID, EDATA> it_edge) : edge<NID, NDATA, EDATA>(g, it_node, it_edge) {}
      };


      int size() const;

      node<NID, NDATA, EDATA> operator[](const NID& nid);
      node_iterator begin();
      node_iterator find(const NID& nid);
      dfs_iterator dfs_node(const NID& source);
      // dfs_iterator dfs_end();
      //dfs_iterator dfs_node(NID&& source);
      node_iterator end();

      edge_iterator ebegin();
      edge_iterator find(const NID& prev, const NID& succ);
      edge_iterator eend();

    private:
      friend class node<NID, NDATA, EDATA>;
      friend class edge<NID, NDATA, EDATA>;
      map<NID, NDATA> m_node_data;
      map<NID, map<NID, EDATA> > m_nexts;
      map<NID, map<NID, EDATA> > m_prevs;
    };



    /////////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION
    /////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////
    // 1. edge
    /////////////////////////////////////////


    template<typename NID, typename NDATA, typename EDATA>
    const NID& edge<NID, NDATA, EDATA>::prev() const {
      // return this->m_it_node.key();
      return this->m_it_node->first;
    }

    template<typename NID, typename NDATA, typename EDATA>
    const NID& edge<NID, NDATA, EDATA>::next() const {
      // return this->m_it_edge.key();
      return this->m_it_edge->first;
    }

    template<typename NID, typename NDATA, typename EDATA>
    EDATA& edge<NID, NDATA, EDATA>::data() {
      // return this->m_it_edge.value();
      return this->m_it_edge->second;
    }


    template<typename NID, typename NDATA, typename EDATA>
    bool edge<NID, NDATA, EDATA>::operator==(edge<NID, NDATA, EDATA> other) const {
      if (this->m_it_node == other.m_it_node) {
        if (this->m_it_node == this->m_g.m_nexts.end()) { // WARNING: when reaching the end, we need to be sure to set m_it_node correctly
          return true;
        } else {
          return this->m_it_edge == other.m_it_edge;
        }
      } else {
        return false;
      }
    }

    template<typename NID, typename NDATA, typename EDATA>
    bool edge<NID, NDATA, EDATA>::operator!=(edge<NID, NDATA, EDATA> other) const {
      if (this->m_it_node == other.m_it_node) {
        if (this->m_it_node == this->m_g.m_nexts.end()) {
          return false;
        } else {
          return this->m_it_edge != other.m_it_edge;
        }
      } else {
        return true;
      }
    }


    /////////////////////////////////////////
    // 2. node
    /////////////////////////////////////////
    
    /////////////////////////////////////////
    // 2.1. core node

    template<typename NID, typename NDATA, typename EDATA>
    const NID& node<NID, NDATA, EDATA>::id() const {
      // return this->m_it.key();
      return this->m_it->first;
    }

    template<typename NID, typename NDATA, typename EDATA>
    NDATA& node<NID, NDATA, EDATA>::data() {
      // return this->m_it.value();
      return this->m_it->second;
    }

    template<typename NID, typename NDATA, typename EDATA>
    bool node<NID, NDATA, EDATA>::operator==(node<NID, NDATA, EDATA> other) const {
      return this->m_it == other.m_it;
    }

    template<typename NID, typename NDATA, typename EDATA>
    bool node<NID, NDATA, EDATA>::operator!=(node<NID, NDATA, EDATA> other) const {
      return this->m_it != other.m_it;
    }

    template<typename NID, typename NDATA, typename EDATA>
    unsigned int node<NID, NDATA, EDATA>::nb_nexts() {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_nexts.find(this->id());
      if (it_node == this->m_g.m_nexts.end()) {
        return 0;
      } else {
        // return it_node.value().size();
        return it_node->second.size();
      }
    }

    template<typename NID, typename NDATA, typename EDATA>
    unsigned int node<NID, NDATA, EDATA>::nb_prevs() {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_prevs.find(this->id());
      if (it_node == this->m_g.m_prevs.end()) {
        return 0;
      }
      else {
        // return it_node.value().size();
        return it_node->second.size();
      }
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename node<NID, NDATA, EDATA>::edge_iterator node<NID, NDATA, EDATA>::begin() {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_nexts.find(this->id());
      map_iterator<NID, EDATA> it_edge;
      if (it_node != this->m_g.m_nexts.end()) {
        // it_edge = it_node.value().begin();
        it_edge = it_node->second.begin();
      }
      return node<NID, NDATA, EDATA>::edge_iterator(this->m_g, it_node, it_edge);
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename node<NID, NDATA, EDATA>::edge_iterator node<NID, NDATA, EDATA>::find(const NID& nid) {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_nexts.find(this->id());
      map_iterator<NID, EDATA> it_edge;
      if (it_node != this->m_g.m_nexts.end()) {
        // it_edge = it_node.value().find(nid);
        it_edge = it_node->second.find(nid);
      }
      return node<NID, NDATA, EDATA>::edge_iterator(this->m_g, it_node, it_edge);
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename node<NID, NDATA, EDATA>::edge_iterator node<NID, NDATA, EDATA>::end() {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_nexts.find(this->id());
      map_iterator<NID, EDATA> it_edge;
      if (it_node != this->m_g.m_nexts.end()) {
        // it_edge = it_node.value().end();
        it_edge = it_node->second.end();
      }
      return node<NID, NDATA, EDATA>::edge_iterator(this->m_g, it_node, it_edge);
    }

    template<typename NID, typename NDATA, typename EDATA>
    edge<NID, NDATA, EDATA> node<NID, NDATA, EDATA>::operator[](const NID& nid) {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_nexts.find(this->id());
      map_iterator<NID, EDATA> it_edge;
      bool failed = false;
      if (it_node != this->m_g.m_nexts.end()) {
        // it_edge = it_node.value().find(nid);
        // failed = (it_edge == it_node.value().end());
        it_edge = it_node->second.find(nid);
        failed = (it_edge == it_node->second.end());
      } else {
        failed = true;
      }
      if (failed) {
        std::cerr << "edge (" << this->id() << ", " << nid << ") does not exist" << std::endl;
        std::terminate();
      }
      return edge<NID, NDATA, EDATA>(this->m_g, it_node, it_edge);
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename node<NID, NDATA, EDATA>::edge_iterator node<NID, NDATA, EDATA>::rbegin() {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_prevs.find(this->id());
      map_iterator<NID, EDATA> it_edge;
      if (it_node != this->m_g.m_prevs.end()) {
        // it_edge = it_node.value().begin();
        it_edge = it_node->second.begin();
      }
      return node<NID, NDATA, EDATA>::edge_iterator(this->m_g, it_node, it_edge);
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename node<NID, NDATA, EDATA>::edge_iterator node<NID, NDATA, EDATA>::rfind(NID& nid) {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_prevs.find(this->id());
      map_iterator<NID, EDATA> it_edge;
      if (it_node != this->m_g.m_prevs.end()) {
        // it_edge = it_node.value().find(nid);
        it_edge = it_node->second.find(nid);
      }
      return node<NID, NDATA, EDATA>::edge_iterator(this->m_g, it_node, it_edge);
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename node<NID, NDATA, EDATA>::edge_iterator node<NID, NDATA, EDATA>::rend() {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_prevs.find(this->id());
      map_iterator<NID, EDATA> it_edge;
      if (it_node != this->m_g.m_prevs.end()) {
        // it_edge = it_node.value().end();
        it_edge = it_node->second.end();
      }
      return node<NID, NDATA, EDATA>::edge_iterator(this->m_g, it_node, it_edge);
    }


    /////////////////////////////////////////
    // 2.2. node::edge_iterator


    template<typename NID, typename NDATA, typename EDATA>
    inline typename node<NID, NDATA, EDATA>::edge_iterator& node<NID, NDATA, EDATA>::edge_iterator::operator++() {
      ++(this->m_it_edge);
      return *this;
    }


    /////////////////////////////////////////
    // 3. graph
    /////////////////////////////////////////


    /////////////////////////////////////////
    // 3.1. graph modification

    template<typename NID, typename NDATA, typename EDATA>
    graph<NID, NDATA, EDATA>::graph() : m_node_data(), m_nexts(), m_prevs() {}

    template<typename NID, typename NDATA, typename EDATA>
    inline void graph<NID, NDATA, EDATA>::add_node(const NID& nid, const NDATA& data) {
      this->m_node_data[nid] = data;
    }

    template<typename NID, typename NDATA, typename EDATA>
    void graph<NID, NDATA, EDATA>::rem_node(const NID& nid) {
      this->m_node_data.erase(nid);
      // update input edges
      auto prevs = this->m_prevs.find(nid);
      if (prevs != this->m_prevs.end()) {
        // for (auto prev : prevs.value()) {
        for (auto prev : prevs->second) {
          this->m_nexts[prev.first].erase(nid);
          if (this->m_nexts[prev.first].empty()) {
            this->m_nexts.erase(prev.first);
          }
        }
        this->m_prevs.erase(prevs);
      }
      // update output edges
      auto nexts = this->m_nexts.find(nid);
      if (nexts != this->m_nexts.end()) {
        // for (auto next : nexts.value()) {
        for (auto next : nexts->second) {
          this->m_prevs[next.first].erase(nid);
          if (this->m_prevs[next.first].empty()) {
            this->m_prevs.erase(next.first);
          }
        }
        this->m_nexts.erase(nexts);
      }
    }

    template<typename NID, typename NDATA, typename EDATA>
    void graph<NID, NDATA, EDATA>::add_edge(const NID& prev, const NID& next, const EDATA& data) {
      // add the edge as output of prev (or update it if already there)
      auto nexts = this->m_nexts.find(prev);
      if (nexts != this->m_nexts.end()) {
        // nexts.value()[next] = data;
        nexts->second[next] = data;
      }
      else {
        this->m_nexts[prev] = { {next, data} };
      }
      // add the edge as input of next (or update it if already there)
      auto prevs = this->m_prevs.find(next);
      if (prevs != this->m_prevs.end()) {
        // prevs.value()[prev] = data;
        prevs->second[prev] = data;
      }
      else {
        this->m_prevs[next] = { {prev, data} };
      }
    }
 
    template<typename NID, typename NDATA, typename EDATA>
    void graph<NID, NDATA, EDATA>::rem_edge(const NID& prev, const NID& next) {
      // remove the edge from prev
      this->m_nexts[prev].erase(next);
      if (this->m_nexts[prev].empty()) {
        this->m_nexts.erase(prev);
      }

      // remove the edge from next
      this->m_prevs[next].erase(prev);
      if (this->m_prevs[next].empty()) {
        this->m_prevs.erase(next);
      }
    }

    /////////////////////////////////////////
    // 3.2. graph::node_iterator

    template<typename NID, typename NDATA, typename EDATA>
    inline typename graph<NID, NDATA, EDATA>::node_iterator& graph<NID, NDATA, EDATA>::node_iterator::operator++() {
      ++(this->m_it);
      return *this;
    }


    /////////////////////////////////////////
    // 3.3. graph::dfs_iterator

    template<typename NID, typename NDATA, typename EDATA>
    typename graph<NID, NDATA, EDATA>::dfs_iterator& graph<NID, NDATA, EDATA>::dfs_iterator::operator++() {
      bool go_next;
      if (this->m_d == t_direction::enter) {
        go_next = true;
      } else {
        this->m_stack.pop_back();
        if (!this->m_stack.empty()) {
          auto& el = this->m_stack.back();
          // this->m_it = this->m_g.m_node_data.find(el.it_node.key());
          this->m_it = this->m_g.m_node_data.find(el.it_node->first);
          ++(el.it_edge); // look at the next edge
          this->m_d = t_direction::enter;
          go_next = true;
        } else {
          this->m_it = this->m_g.m_node_data.end(); // close the iterator
          go_next = false;
        }
      }

      if (go_next) {
        auto& el = this->m_stack.back();
        bool go_child = (el.it_node != this->m_g.m_nexts.end()); // we have some edges
        if (go_child) {
          // go_child = (el.it_edge != el.it_node.value().end());
          go_child = (el.it_edge != el.it_node->second.end());
          // while (go_child && (this->m_visited.contains(el.it_edge.key()))) {
          while (go_child && (this->m_visited.find(el.it_edge->first)) != this->m_visited.end()) {
            ++(el.it_edge);
            // go_child = (el.it_edge != el.it_node.value().end());
            go_child = (el.it_edge != el.it_node->second.end());
          }
        }
        if (go_child) { // go to child node
          // NID& nid = el.it_edge.key();
          const NID& nid = el.it_edge->first;
          this->m_visited.insert(nid);
          this->m_it = this->m_g.m_node_data.find(nid);
          map_iterator<NID, map<NID, EDATA>> it_node = this->m_g.m_nexts.find(nid);
          map_iterator<NID, EDATA> it_edge;
          if (it_node != this->m_g.m_nexts.end()) {
            // it_edge = it_node.value().begin();
            it_edge = it_node->second.begin();
          }
          this->m_stack.push_back({ it_node, it_edge });
        } else {
          this->m_d = t_direction::leave;
        }
      }
      return *this;
    }


    /////////////////////////////////////////
    // 3.4. graph::edge_iterator


    template<typename NID, typename NDATA, typename EDATA>
    inline typename graph<NID, NDATA, EDATA>::edge_iterator& graph<NID, NDATA, EDATA>::edge_iterator::operator++() {
      ++(this->m_it_edge);
      // if (this->m_it_edge == this->m_it_node.value().end()) {
      if (this->m_it_edge == this->m_it_node->second.end()) {
        // std::cout << " g::edge::++ -> reaching end of node " << this->m_it_node.key() << std::endl;
        ++(this->m_it_node);
        if (this->m_it_node != this->m_g.m_nexts.end()) {
          // std::cout << " g::edge::++ -> start node " << this->m_it_node.key() << std::endl;
          // this->m_it_edge = this->m_it_node.value().begin();
          this->m_it_edge = this->m_it_node->second.begin();
        } else {
          // std::cout << " g::edge::++ -> reaching end of graph " << this->m_it_node.key() << std::endl;
        }
      }
      return *this;
    }


    /////////////////////////////////////////
    // 3.5. graph lookup


    template<typename NID, typename NDATA, typename EDATA>
    inline int graph<NID, NDATA, EDATA>::size() const {
      return this->m_node_data.size();
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename graph<NID, NDATA, EDATA>::node_iterator graph<NID, NDATA, EDATA>::begin() {
      return graph<NID, NDATA, EDATA>::node_iterator(*this, this->m_node_data.begin());
    }

    template<typename NID, typename NDATA, typename EDATA>
    node<NID, NDATA, EDATA> graph<NID, NDATA, EDATA>::operator[](const NID& nid) {
      map_iterator<NID, NDATA> it = this->m_node_data.find(nid);
      if (it == this->m_node_data.end()) {
        std::cerr << "node[" << nid << "] does not exist" << std::endl;
        std::terminate();
      }
      else {
        return node<NID, NDATA, EDATA>(*this, it);
      }
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename graph<NID, NDATA, EDATA>::node_iterator graph<NID, NDATA, EDATA>::find(const NID& nid) {
      map_iterator<NID, NDATA> it = this->m_node_data.find(nid);
      return graph<NID, NDATA, EDATA>::node_iterator(*this, it);
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename graph<NID, NDATA, EDATA>::dfs_iterator graph<NID, NDATA, EDATA>::dfs_node(const NID& source) {
      return graph<NID, NDATA, EDATA>::dfs_iterator(*this, this->m_node_data.find(source));
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename graph<NID, NDATA, EDATA>::node_iterator graph<NID, NDATA, EDATA>::end() {
      return graph<NID, NDATA, EDATA>::node_iterator(*this, this->m_node_data.end());
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename graph<NID, NDATA, EDATA>::edge_iterator graph<NID, NDATA, EDATA>::ebegin() {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_nexts.begin();
      map_iterator<NID, EDATA> it_edge;
      if (it_node != this->m_nexts.end()) {
        // it_edge = it_node.value().begin();
        it_edge = it_node->second.begin();
      }
      return graph<NID, NDATA, EDATA>::edge_iterator(*this, it_node, it_edge);
    }

    template<typename NID, typename NDATA, typename EDATA>
    typename graph<NID, NDATA, EDATA>::edge_iterator graph<NID, NDATA, EDATA>::eend() {
      return graph<NID, NDATA, EDATA>::edge_iterator(*this, this->m_nexts.end(), map_iterator<NID, EDATA>());
    }



    template<typename NID, typename NDATA, typename EDATA>
    typename graph<NID, NDATA, EDATA>::edge_iterator graph<NID, NDATA, EDATA>::find(const NID& prev, const NID& next) {
      map_iterator<NID, map<NID, EDATA>> it_node = this->m_nexts.find(prev);
      map_iterator<NID, EDATA> it_edge;
      if (it_node != this->m_nexts.end()) {
        // it_edge = it_node.value().find(next);
        // if(it_edge == it_node.value().end()) {
        it_edge = it_node->second.find(next);
        if(it_edge == it_node->second.end()) {
          it_node = this->m_nexts.end();
        }
      }
      return graph<NID, NDATA, EDATA>::edge_iterator(*this, it_node, it_edge);
    }


  } // namespace graph
} // namespace hrw


#endif // __HREWRITE_GRAPH_H__

