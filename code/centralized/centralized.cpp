#include "graph.h"
#include "../util.c"
#include <unistd.h>
#include <ctype.h>
#include <queue>
#include <unordered_map>
#include <climits>
#include <unordered_set>
#include <assert.h>
#include <chrono>
#include <pthread.h>
#include <mutex>
#include <cmath>

// Threading support
typedef struct {
  node_t source;
  node_t target;
  std::shared_ptr<graph_t> graph;
  std::shared_ptr<std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo>> pq;
  std::shared_ptr<std::unordered_set<node_t, node_hash_t>> openSet;
  std::shared_ptr<std::unordered_map<node_t, node_t, node_hash_t>> cameFrom;
  std::shared_ptr<std::unordered_map<node_t, int, node_hash_t>> gScore;
  std::shared_ptr<std::unordered_map<node_t, int, node_hash_t>> fScore;
  std::shared_ptr<std::vector<node_t>> path;
  std::shared_ptr<int> pathCost;
  int threadId;
  int numThreads;
} WorkerArgs;

std::mutex muxPq;
std::mutex muxCameFrom;
std::mutex muxScore;
std::mutex muxTermCount;

int termCount = 0;

// heuristic function 
int h(node_t source, node_t target) {
  // euclidian distance
  return sqrt(std::abs(source.row - target.row)*std::abs(source.row - target.row) + std::abs(source.col - target.col)*std::abs(source.col - target.col));
}

/* 
 * returns the neighbors of the current node as a vector. Checks whether or
 * not they exist (if they are within the grid and equal to 1)
 */
std::vector<node_t> getNeighbors(node_t current, std::shared_ptr<graph_t> graph, std::vector<node_t> neighbors) {
  int i = current.row*graph->dim + current.col;
  // element above
  if (current.row != 0 && graph->grid[i - graph->dim]) {
    neighbors.push_back({current.row - 1, current.col});
  }
  // element below
  if (current.row < graph->dim - 1 && graph->grid[i + graph->dim]) {
    neighbors.push_back({current.row + 1, current.col});
  }
  // element to the left 
  if (current.col != 0 && graph->grid[i - 1]) {
    neighbors.push_back({current.row, current.col - 1});
  }
  // element to the right
  if (current.col < graph->dim - 1 && graph->grid[i + 1]) {
    neighbors.push_back({current.row, current.col + 1});
  }
  return neighbors;
}

/* 
 * uses the cameFrom map to reconstruct the path from the current (target) node
 * and returns it as a vector in reverse order of the path.
*/
std::vector<node_t> reconstructPath(std::unordered_map<node_t, node_t, node_hash_t> cameFrom, node_t current) {
  std::vector<node_t> path;
  path.emplace_back(current);
  while (cameFrom.find(current) != cameFrom.end()) {
    current = cameFrom.at(current);
    path.emplace_back(current);
  }
  return path;
}

void* aStar(void *threadArgs) {
  WorkerArgs* args = static_cast<WorkerArgs*>(threadArgs);  

  auto graph = args->graph;
  auto pq = args->pq;
  auto openSet = args->openSet;
  auto cameFrom = args->cameFrom;
  auto gScore = args->gScore;
  auto fScore = args->fScore;
  auto path = args->path;
  auto pathCost = args->pathCost;

  std::vector<node_t> neighbors;
  bool waiting = false;
  while (true) {
    muxTermCount.lock();
    // check whether all threads have terminated 
    if (termCount == args->numThreads) {
      printf("term count: %d\n", termCount);
      muxTermCount.unlock();
      break;
    }
    muxTermCount.unlock();

    muxPq.lock();
    if (pq->empty() || pq->top().cost >= *pathCost) {
      muxPq.unlock();
      waiting = true;
      muxTermCount.lock();
      termCount++;
      muxTermCount.unlock();
      continue;
    } else if (waiting) {
      waiting = false;
      muxTermCount.lock();
      termCount--;
      muxTermCount.unlock();
    }
    
    // find solution
    node_info_t current = pq->top();
    printf("(%d, %d)\n", current.node.row, current.node.col);
    pq->pop();
    openSet->erase(current.node);
    muxPq.unlock();

    if (current.node == args->target && current.cost < *pathCost) {
      muxCameFrom.lock();
      *path = reconstructPath(*cameFrom, current.node);
      *pathCost = current.cost;
      muxCameFrom.unlock();
      continue;
    }

    neighbors = getNeighbors(current.node, graph, neighbors);
    for (node_t neighbor: neighbors) { 
      muxScore.lock();
      int neighborScore = gScore->find(neighbor) != gScore->end() ? gScore->at(neighbor) : INT_MAX;
      // every edge has weight 1
      int currentScore = gScore->at(current.node) + 1;
      if (currentScore < neighborScore) {
        gScore->emplace(neighbor, currentScore);
        int neighborfScore = currentScore + h(neighbor, args->target);
        fScore->emplace(neighbor, neighborfScore);
        muxScore.unlock();

        muxCameFrom.lock();
        cameFrom->emplace(neighbor, current.node);
        muxCameFrom.unlock();
        
        muxPq.lock();
        if (openSet->find(neighbor) == openSet->end()) {
          openSet->emplace(neighbor);
          pq->push({neighborfScore, neighbor});
        }
        muxPq.unlock();
      }
      muxScore.unlock();
    }
    neighbors.clear();
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  using namespace std::chrono;
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::duration<double> dsec;

  auto init_start = Clock::now();
  double init_time = 0;

  int opt = 0;
  char *inputFilename = NULL;
  int numThreads;
  int x1 = -1;
  int y1 = -1;
  int x2 = -1;
  int y2 = -1;
  // Read command line arguments

  do {
    opt = getopt(argc, argv, "f:n:");
    switch (opt) {
    case 'f':
      inputFilename = optarg;
      break;
    case 'n':
      numThreads = atoi(optarg);
      break;
    case -1:
      break;
    default:
      break;
    }
  } while (opt != -1);

  if (inputFilename == NULL || argc < 8) {
      printf("Usage: %s -f <filename> -n <num_threads> <x1> <y1> <x2> <y2>\n", argv[0]);
      return -1;
  }
  x1 = std::stoi(argv[5]);
  y1 = std::stoi(argv[6]);
  x2 = std::stoi(argv[7]);
  y2 = std::stoi(argv[8]);

  std::shared_ptr<graph_t> graph = readGraph(x1, y1, x2, y2, inputFilename);
  
  const static int MAX_THREADS = 32;

  if (numThreads > MAX_THREADS) {
    fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
    exit(1);
  }

  pthread_t workers[MAX_THREADS];
  WorkerArgs args[MAX_THREADS];

  std::shared_ptr<std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo>> pq =  
    std::make_shared<std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo>>();
  std::shared_ptr<std::unordered_set<node_t, node_hash_t>> openSet =
    std::make_shared<std::unordered_set<node_t, node_hash_t>>();
  std::shared_ptr<std::unordered_map<node_t, node_t, node_hash_t>> cameFrom =
    std::make_shared<std::unordered_map<node_t, node_t, node_hash_t>>();
  std::shared_ptr<std::unordered_map<node_t, int, node_hash_t>> gScore =
    std::make_shared<std::unordered_map<node_t, int, node_hash_t>>();
  std::shared_ptr<std::unordered_map<node_t, int, node_hash_t>> fScore =
    std::make_shared<std::unordered_map<node_t, int, node_hash_t>>();
  std::shared_ptr<std::vector<node_t>> path =
    std::make_shared<std::vector<node_t>>();
  std::shared_ptr<int> pathCost = std::make_shared<int>(INT_MAX);

  for (int i = 0; i < numThreads; i++) {
    args[i].threadId = i;
    args[i].numThreads = numThreads;
    args[i].source = {x1, y1};
    args[i].target = {x2, y2};
    args[i].graph = graph;
    args[i].pq = pq;
    args[i].openSet = openSet;
    args[i].cameFrom = cameFrom;
    args[i].gScore = gScore;
    args[i].fScore = fScore;
    args[i].path = path;
    args[i].pathCost = pathCost;
  }

  node_t source = {x1, y1};
  node_t target = {x2, y2};
  pq->push({h(source, {x2, y2}), source});
  openSet->insert(source);

  // gScore represents the cost of the cheapest path from start to current node
  gScore->insert({source, 0});
  // fScore represents the total cost f(n) = g(n) + h(n) for any node
  fScore->insert({source, h(source, target)});

  init_time += duration_cast<dsec>(Clock::now() - init_start).count();
  printf("Initialization Time: %lf.\n", init_time);

  auto compute_start = Clock::now();
  double compute_time = 0;
  
  for (int i = 1; i < numThreads; i++)
    pthread_create(&workers[i], NULL, aStar, &args[i]);

  aStar(&args[0]);

  compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);

  for (int i = 1; i < numThreads; i++)
    pthread_join(workers[i], NULL);
  printf("pthread join done\n");
  free(graph->grid);
  writeOutput(inputFilename, *path);
  printf("write output done");
}
