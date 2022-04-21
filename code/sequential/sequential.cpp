#include "../graph.h"
#include "../util.c"
#include <unistd.h>
#include <ctype.h>
#include <queue>
#include <unordered_map>
#include <climits>
#include <unordered_set>
#include <assert.h>

// heuristic function 
int h(node_t source, node_t target) {
  // manhatten distance
  return std::abs(source.row - target.row) + std::abs(source.col - target.col);
}

/* 
 * returns the neighbors of the current node as a vector. Checks whether or
 * not they exist (if they are within the grid and equal to 1)
 */
std::vector<node_t> getNeighbors(node_t current, graph_t *graph, std::vector<node_t> neighbors) {
  printf("size of neighbors at beginning of loop: %d\n", neighbors.size());
  int i = current.row*graph->dim + current.col;
  printf("current i: %d \n", i);
  // element above
  if (current.row != 0 && graph->grid[i - graph->dim]) {
    neighbors.push_back({current.row - 1, current.col});
  }
  printf("elem above\n");
  // element below
  if (current.row < graph->dim - 1 && graph->grid[i + graph->dim]) {
    neighbors.push_back({current.row + 1, current.col});
  }
  printf("elem below\n");
  // element to the left 
  printf("BEFORE\n");
  printf("%d\n", graph->grid[i - 1]);
  printf("AFTER\n");
  if (current.col != 0 && graph->grid[i - 1]) {
    neighbors.push_back({current.row, current.col - 1});
  }
  printf("elem left\n");
  // element to the right
  if (current.col < graph->dim - 1 && graph->grid[i + 1]) {
    neighbors.push_back({current.row, current.col + 1});
  }
  printf("elem right\n");
  printf("size of neighbors: %d\n", neighbors.size());
  return neighbors;
}

/* 
 * uses the cameFrom map to reconstruct the path from the current (target) node
 * and returns it as a vector in reverse order of the path.
*/
std::vector<node_t> reconstructPath(std::unordered_map<node_t, node_t, node_hash_t> cameFrom, node_t current) {
  std::vector<node_t> path;
  path.push_back(current);
  while (cameFrom.find(current) != cameFrom.end()) {
    current = cameFrom.at(current);
    path.push_back(current);
  }
  return path;
}

std::vector<node_t> aStar(node_t source, node_t target, graph_t *graph) {
  std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo> pq;
  std::unordered_set<node_t, node_hash_t> openSet;
  std::unordered_map<node_t, node_t, node_hash_t> cameFrom;
  std::unordered_map<node_t, int, node_hash_t> gScore;
  std::unordered_map<node_t, int, node_hash_t> fScore;
  std::vector<node_t> path;
  // initialize open set 
  pq.push({h(source, target), source});
  openSet.insert(source);

  // gScore represents the cost of the cheapest path from start to current node
  gScore.insert({source, 0});
  // fScore represents the total cost f(n) = g(n) + h(n) for any node
  fScore.insert({source, h(source, target)});

  std::vector<node_t> neighbors;
  while (!pq.empty()) {
    node_t current = pq.top().node;
    // find solution
    if (current == target) {
      path = reconstructPath(cameFrom, current);
      break;
    }

    pq.pop();
    openSet.erase(current);
    printf("current: %d %d\n", current.row, current.col);
    neighbors = getNeighbors(current, graph, neighbors);
    for (node_t neighbor: neighbors) { 
      int neighborScore = gScore.find(neighbor) != gScore.end() ? gScore.at(neighbor) : INT_MAX;
      // every edge has weight 1
      int currentScore = gScore.at(current) + 1;
      if (currentScore < neighborScore) {
        cameFrom.emplace(neighbor, current);
        gScore.emplace(neighbor, currentScore);
        int neighborfScore = currentScore + h(neighbor, target);
        fScore.emplace(neighbor, neighborfScore);
        if (openSet.find(neighbor) == openSet.end()) {
          openSet.emplace(neighbor);
          pq.push({neighborfScore, neighbor});
        }
      }
    }
    neighbors.clear();
    printf("size after loop: %d\n", neighbors.size());
    assert(pq.size() == openSet.size());
  }

  return path;
}

int main(int argc, char *argv[]) {
  int opt = 0;
  char *inputFilename = NULL;
  int x1 = -1;
  int y1 = -1;
  int x2 = -1;
  int y2 = -1;
  // Read command line arguments

  do {
    opt = getopt(argc, argv, "f:");
    switch (opt) {
    case 'f':
      inputFilename = optarg;
      break;
    case -1:
      break;
    default:
      break;
    }
  } while (opt != -1);

  if (inputFilename == NULL || argc < 7) {
      printf("Usage: %s -f <filename> <x1> <y1> <x2> <y2>\n", argv[0]);
      return -1;
  }
  x1 = std::stoi(argv[3]);
  y1 = std::stoi(argv[4]);
  x2 = std::stoi(argv[5]);
  y2 = std::stoi(argv[6]);

  graph_t *graph = readGraph(x1, y1, x2, y2, inputFilename);
  node_t source = {x1, y1};
  node_t target = {x2, y2};

  std::vector<node_t> ret = aStar(source, target, graph);

  writeOutput(inputFilename, ret);
}

// TODO: testing infrastructure, larger and more varied outputs 
