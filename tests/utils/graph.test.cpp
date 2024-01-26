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
#if ENABLE_UTILS_GRAPH

#include "doctest/doctest.h"
#include "tests/common/debug.hpp"


#include <algorithm>
#include <string>
#include <iostream>


#include "hrewrite/utils/graph.hpp"
#include "hrewrite/utils/natset.hpp"
// #include "hrewrite/utils.hpp"


using namespace hrw::utils;



void check_graph_node(graph<unsigned int, unsigned int, unsigned int>& g, std::vector<std::pair<unsigned int, unsigned int>>&& content) {
  // check node \subseteq content
  for(node<unsigned int, unsigned int, unsigned int>n: g) {
    auto data = std::make_pair(n.id(), n.data());
    OUTPUT("  looking for node (", data.first, ",", data.second, ") in content");
    CHECK(std::find(content.begin(), content.end(), data) != content.end());
  }
  // check content \subseteq node
  for(std::pair<unsigned int, unsigned int>el: content) {
    OUTPUT("  looking for data (", el.first, ",", el.second, ") in graph");
    graph<unsigned int, unsigned int, unsigned int>::node_iterator n = g.find(el.first);
    CHECK(n != g.end());
    CHECK(n.data() == el.second);
  }
}

void check_graph_edge(graph<unsigned int, unsigned int, unsigned int>& g, std::vector<std::tuple<unsigned int, unsigned int, unsigned int>>&& content) {
  // check edge \subseteq content
  for(graph<unsigned int, unsigned int, unsigned int>::edge_iterator e = g.ebegin(); e!= g.eend(); ++e) {
    CHECK(std::find(content.begin(), content.end(), std::make_tuple(e.prev(), e.next(), e.data())) != content.end());
  }
  // check content \subseteq node
  for(std::tuple<unsigned int, unsigned int, unsigned int>el: content) {
    graph<unsigned int, unsigned int, unsigned int>::edge_iterator e = g.find(std::get<0>(el), std::get<1>(el));
    CHECK(e != g.eend());
    CHECK(e.data() == std::get<2>(el));
  }
}

std::string to_string_branch(std::vector<node<unsigned int, unsigned int, unsigned int>>& branch) {
  std::string res = "[ ";
  for(node<unsigned int, unsigned int, unsigned int> n: branch) {
   res = res + std::to_string(n.id()) + " ";
  }
  res = res + "]";
  return res;
}

void check_graph_dfs(graph<unsigned int, unsigned int, unsigned int>& g) {
  for(node<unsigned int, unsigned int, unsigned int> n: g) { // test dfs starting from every node
    OUTPUT("Starting DFS from node ", n.id());
    natset visited; // no node visited yet: when going back in the graph, check that all next have been visited
    std::vector<node<unsigned int, unsigned int, unsigned int>> stack; // when going forward in the graph, check that the node was not visited before and the end of the current branch is a parent of the node
    for(auto it = g.dfs_node(n.id()); it != g.end(); ++it) {
      OUTPUT("  visiting node ", it.id(), " -> ", ((it.direction() == t_direction::enter)?"enter":"leave"), "| branch=", to_string_branch(stack));
      if(it.direction() == t_direction::enter) {
        CHECK_FALSE(visited.contains(it.id()));
        if(stack.empty()) {
          CHECK(it.id() == n.id());
        } else {
          OUTPUT("    parent=", stack.back().id(), ", nb_nexts=", stack.back().nb_nexts());
          bool is_next = false;
          for(edge<unsigned int, unsigned int, unsigned int> e: stack.back()) {
            OUTPUT("    edge -", e.data(), "-> ", e.next());
            is_next = is_next || (e.next() == it.id());
          }
          CHECK(is_next);
        }
        stack.push_back(it);
        visited.add(it.id());
      } else {
        CHECK(visited.contains(it.id()));
        CHECK(stack.back().id() == it.id());
        bool missing_next = false;
        for(edge<unsigned int, unsigned int, unsigned int> e: stack.back()) {
          missing_next = missing_next || (!visited.contains(e.next()));
        }
        CHECK_FALSE(missing_next);
        stack.pop_back();
      }
      // TODO: we need to check that the traversal follows the properties of dfs: go to every nexts if not already visited, and then going back
    }
  }
}

void check_graph_node_content(
    graph<unsigned int, unsigned int, unsigned int>& g,unsigned int id, unsigned int data,
    std::vector<std::pair<unsigned int, unsigned int>>&& prev, std::vector<std::pair<unsigned int, unsigned int>>&& next) {
  node<unsigned int, unsigned int, unsigned int> n = g[id];
  CHECK(n.data() == data);

  // check next edge \subseteq next
  for(node<unsigned int, unsigned int, unsigned int>::edge_iterator e = n.begin(); e!= n.end(); ++e) {
    CHECK(std::find(next.begin(), next.end(), std::make_pair(e.next(), e.data())) != next.end());
  }
  // check next \subseteq next edge
  for(std::pair<unsigned int, unsigned int>el: next) {
    node<unsigned int, unsigned int, unsigned int>::edge_iterator e = n.find(el.first);
    CHECK(e != n.end());
    CHECK(e.data() == el.second);
  }

  // check prev edge \subseteq prev
  for(node<unsigned int, unsigned int, unsigned int>::edge_iterator e = n.rbegin(); e!= n.rend(); ++e) {
    CHECK(std::find(prev.begin(), prev.end(), std::make_pair(e.next(), e.data())) != prev.end());
  }
  // check preev \subseteq prev
  for(std::pair<unsigned int, unsigned int>el: prev) {
    node<unsigned int, unsigned int, unsigned int>::edge_iterator e = n.rfind(el.first);
    CHECK(e != n.end());
    CHECK(e.data() == el.second);
  }
}

TEST_CASE("test_graph") {
  graph<unsigned int, unsigned int, unsigned int> g;
  OUTPUT("Testing graph: Empty");
  check_graph_node(g, {});
  check_graph_edge(g, {});

  g.add_node(0, 0);
  OUTPUT("Testing graph: (0,0)");
  check_graph_node(g, { {0,0} });
  check_graph_edge(g, {});

  g.add_node(1, 1);
  OUTPUT("Testing graph: (0,0) (1,1)");
  check_graph_node(g, { {0,0}, {1,1} });
  check_graph_edge(g, {});

  g.add_node(2, 2);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2)");
  check_graph_node(g, { {0,0}, {1,1}, {2,2} });
  check_graph_edge(g, {});
  check_graph_node_content(g, 2, 2, {}, {});

  g.add_node(3, 3);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3)");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3} });
  check_graph_edge(g, {});

  g.add_node(4, 4);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4)");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4} });
  check_graph_edge(g, {});

  g.add_node(5, 5);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5)");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, {});
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {}, {});

  g.add_edge(0, 1, 1);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) | 0-1->1");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, { {0,1,1} });
  check_graph_dfs(g);

  g.add_edge(0, 2, 2);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) | 0-1->1 0-2->2");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2} });
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {{0,2}}, {});

  g.add_edge(1, 3, 4);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) | 0-1->1 0-2->2 1-4->3");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2}, {1,3,4} });
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {{0,2}}, {});

  g.add_edge(1, 4, 5);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) | 0-1->1 0-2->2 1-4->3 1-5->4");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2}, {1,3,4}, {1,4,5} });
  check_graph_dfs(g);

  g.add_edge(2, 4, 6);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) | 0-1->1 0-2->2 1-4->3 1-5->4 2-6->4");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2}, {1,3,4}, {1,4,5}, {2,4,6} });
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {{0,2}}, {{4,6}});

  g.add_edge(2, 5, 7);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) | 0-1->1 0-2->2 1-4->3 1-5->4 2-6->4 2-7->5");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2}, {1,3,4}, {1,4,5}, {2,4,6}, {2,5,7} });
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {{0,2}}, {{4,6},{5,7}});

  g.add_edge(3, 0, 3);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) | 0-1->1 0-2->2 1-4->3 1-5->4 2-6->4 2-7->5 3-3->0");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2}, {1,3,4}, {1,4,5}, {2,4,6}, {2,5,7}, {3,0,3} });
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {{0,2}}, {{4,6},{5,7}});

  g.rem_edge(2, 4);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) | 0-1->1 0-2->2 1-4->3 1-5->4 2-7->5 3-3->0");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2}, {1,3,4}, {1,4,5}, {2,5,7}, {3,0,3} });
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {{0,2}}, {{5,7}});

  g.rem_node(4);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (5,5) | 0-1->1 0-2->2 1-4->3 2-7->5 3-3->0");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2}, {1,3,4}, {2,5,7}, {3,0,3} });
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {{0,2}}, {{5,7}});

  g.rem_edge(2, 5);
  OUTPUT("Testing graph: (0,0) (1,1) (2,2) (3,3) (5,5) | 0-1->1 0-2->2 1-4->3 3-3->0");
  check_graph_node(g, { {0,0}, {1,1}, {2,2}, {3,3}, {5,5} });
  check_graph_edge(g, { {0,1,1}, {0,2,2}, {1,3,4}, {3,0,3} });
  check_graph_dfs(g);
  check_graph_node_content(g, 2, 2, {{0,2}}, {});

}


#endif

