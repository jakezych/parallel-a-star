#ifndef _GRAPH_H
#define _GRAPH_H

#include <cstddef>
#include <functional>

typedef struct {
  int cost;   // the current path score f(n) for the node 
  int node;   // the index corresponding to the node
} node_info_t;

struct CompareNodeInfo {
    bool operator()(node_info_t const & v1, node_info_t const & v2) {
        // returns true if v1 has a larger path score f(n) than v2
        return v1.cost > v2.cost;
    }
};

struct graph_t {
  int dim;     // dimension of the grid where grids are assumed to be square in shape
  int *grid;   /* 
                  1 at position [i][j] indicates there is a node there, 0 
                  indicates there is an obstacle (node missing), 
                  edges are assumed to be the 4 adjacent elements (up, left, 
                  right, down) as long as those elements are 1s. Represented 
                  as a 1 dimensional array where [i][j] corresponds to index
                  [i*dim + j] 
                */
  graph_t(const int d, int *g) : dim(d), grid(g) {};
};

#endif 