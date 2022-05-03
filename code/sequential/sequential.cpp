#include "../graph.h"
#include "../util.c"
#include <unistd.h>
#include <ctype.h>
#include <queue>
#include <unordered_map>
#include <climits>
#include <unordered_set>
#include <assert.h>
#include <chrono>
#include <cmath>

std::shared_ptr<graph_t> graph;

// heuristic function 
int h(int source, int target) {
  int sourceR = source / graph->dim;
  int sourceC = source % graph->dim;
  int targetR = target / graph->dim;
  int targetC = target % graph->dim;
  // euclidian distance
  return sqrt(std::abs(sourceR - targetR)*std::abs(sourceR - targetR) + std::abs(sourceC - targetC)*std::abs(sourceC - targetC));
}

/* 
 * returns the neighbors of the current node as a vector. Checks whether or
 * not they exist (if they are within the grid and equal to 1)
 */
std::vector<int> getNeighbors(int current, std::shared_ptr<graph_t> graph, std::vector<int> neighbors) {
  int currentR = current / graph->dim;
  int currentC = current % graph->dim;
  // element above
  if (currentR != 0 && graph->grid[current - graph->dim]) {
    neighbors.push_back(current - graph->dim);
  }
  // element below
  if (currentR < graph->dim - 1 && graph->grid[current + graph->dim]) {
    neighbors.push_back(current + graph->dim);
  }
  // element to the left 
  if (currentC != 0 && graph->grid[current - 1]) {
    neighbors.push_back(current - 1);
  }
  // element to the right
  if (currentC < graph->dim - 1 && graph->grid[current + 1]) {
    neighbors.push_back(current + 1);
  }
  return neighbors;
}

/* 
 * uses the cameFrom map to reconstruct the path from the current (target) node
 * and returns it as a vector in reverse order of the path.
*/
std::vector<int> reconstructPath(std::unordered_map<int, int> cameFrom, int current) {
  std::vector<int> path;
  path.emplace_back(current);
  while (cameFrom.find(current) != cameFrom.end()) {
    current = cameFrom.at(current);
    path.emplace_back(current);
  }
  return path;
}

std::vector<int> aStar(int source, int target, std::shared_ptr<graph_t> graph) {
  std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo> pq;
  std::unordered_set<int> openSet;
  std::unordered_map<int, int> cameFrom;
  std::unordered_map<int, int> gScore;
  std::vector<int> path;
  // initialize open set 
  pq.push({h(source, target), source});
  openSet.insert(source);

  // gScore represents the cost of the cheapest path from start to current node
  gScore.insert({source, 0});

  // initialize all other nodes to have an inf g score 
  for (int i = 0; i < graph->dim; i++) {
    for (int j = 0; j < graph->dim; j++) {
      if (i != source / graph->dim || j != source % graph->dim) {
        gScore.insert({i*graph->dim + j, INT_MAX});
      }
    }
  }

  std::vector<int> neighbors;
  while (!pq.empty()) {
    int current = pq.top().node;
    // find solution
    if (current == target) {
      path = reconstructPath(cameFrom, current);
      break;
    }

    pq.pop();
    openSet.erase(current);
    neighbors = getNeighbors(current, graph, neighbors);
    for (int neighbor: neighbors) { 
      int neighborScore = gScore.at(neighbor);
      // every edge has weight 1
      int currentScore = gScore.at(current) + 1;
      if (currentScore < neighborScore) {
        if (cameFrom.find(current) != cameFrom.end()) {
          if (cameFrom.at(current) != neighbor) {
            cameFrom.emplace(neighbor, current);
          }
        } else {
          cameFrom.emplace(neighbor, current);
        }
        gScore.erase(neighbor);
        gScore.emplace(neighbor, currentScore);
        int neighborfScore = currentScore + h(neighbor, target);
        if (openSet.find(neighbor) == openSet.end()) {
          openSet.emplace(neighbor);
          pq.push({neighborfScore, neighbor});
        }
      }
    }
    neighbors.clear();
    assert(pq.size() == openSet.size());
  }

  return path;
}

int main(int argc, char *argv[]) {
  using namespace std::chrono;
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::duration<double> dsec;

  auto init_start = Clock::now();
  double init_time = 0;

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

  graph = readGraph(x1, y1, x2, y2, inputFilename);
  int source = x1*graph->dim + y1;
  int target = x2*graph->dim + y2;

  init_time += duration_cast<dsec>(Clock::now() - init_start).count();
  printf("Initialization Time: %lf.\n", init_time);

  auto compute_start = Clock::now();
  double compute_time = 0;
  
  std::vector<int> ret = aStar(source, target, graph);

  compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);

  free(graph->grid);
  writeOutput(inputFilename, ret, graph);
}