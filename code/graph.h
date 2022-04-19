#ifndef _GRAPH_H
#define _GRAPH_H

typedef struct {
  int row; // row where the node is located
  int col; // col where the node is located
} node_t;

typedef struct {
  int cost;   // the current path score f(n) for the node 
  node_t node;   // the index corresponding to the node
} node_info_t;

struct CompareNodeInfo {
    bool operator()(node_info_t const & v1, node_info_t const & v2) {
        // returns true if v1 has a smaller path score f(n) than v2
        return v1.cost < v2.cost;
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


/* 
  Reads the graph from the input file with the inputted name. The first line of
  the file represents the dimenensions of the grid (i.e. 5 => 5x5). Graphs are
  represented as adjacency matrices where a 1 at graph[i][j] corresponds to 
  an edge with weight 1 between vertex i and j and 0 corresponds to no edge.
*/
graph_t readGraph(char *inputFilename);

#endif 