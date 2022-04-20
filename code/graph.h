#ifndef _GRAPH_H
#define _GRAPH_H

#include <boost/functional/hash.hpp>

struct node_t {
  int row; // row where the node is located
  int col; // col where the node is located

  bool operator==(const node_t &other) const
  { return (row == other.row
            && col == other.col);
  }
};

struct node_hash_t {
  std::size_t operator()(const node_t& n) const
  {
    // Start with a hash value of 0
    std::size_t seed = 0;

    // Modify 'seed' by XORing and bit-shifting
    boost::hash_combine(seed,boost::hash_value(n.row));
    boost::hash_combine(seed,boost::hash_value(n.col));

    return seed;
  }
};

typedef struct {
  int cost;   // the current path score f(n) for the node 
  node_t node;   // the index corresponding to the node
} node_info_t;

struct CompareNodeInfo {
    bool operator()(node_info_t const & v1, node_info_t const & v2) {
        // returns true if v1 has a larger path score f(n) than v2
        return v1.cost > v2.cost;
    }
};

typedef struct {
  int dim;     // dimension of the grid where grids are assumed to be square in shape
  int *grid;   /* 
                  1 at position [i][j] indicates there is a node there, 0 
                  indicates there is an obstacle (node missing), 
                  edges are assumed to be the 4 adjacent elements (up, left, 
                  right, down) as long as those elements are 1s. Represented 
                  as a 1 dimensional array where [i][j] corresponds to index
                  [i*dim + j] 
                */
} graph_t;

#endif 