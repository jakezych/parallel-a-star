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
#include <pthread.h>
#include <mutex>
#include <cmath>

// Threading support
typedef struct {
  int source;
  int target;
  int threadId;
  int numThreads;
} WorkerArgs;

std::mutex muxPq;
std::mutex muxCameFrom;
std::mutex muxScore;
std::mutex muxTermCount;
std::mutex muxPath;

std::shared_ptr<graph_t> graph;

int termCount = 0;
int pathCost = INT_MAX;

std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo> pq;
std::unordered_set<int> openSet;
std::unordered_set<int> closedSet;
std::unordered_map<int, int> cameFrom;
std::unordered_map<int, int> gScore;
std::vector<int> path;


// heuristic function 
int h(int source, int target) {
  int sourceR = source / graph->dim;
  int sourceC = source % graph->dim;
  int targetR = target / graph->dim;
  int targetC = target % graph->dim;
  // euclidian distance
  // return sqrt(std::abs(sourceR - targetR)*std::abs(sourceR - targetR) + std::abs(sourceC - targetC)*std::abs(sourceC - targetC));
  // manhatten distance 
  return std::abs(sourceR - targetR) + std::abs(sourceC - targetC);
}

/* 
 * returns the neighbors of the current node as a vector. Checks whether or
 * not they exist (if they are within the grid and equal to 1)
 */
std::vector<int> getNeighbors(int current, std::vector<int> neighbors) {
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
std::vector<int> reconstructPath(int current) {
  std::vector<int> path;
  path.emplace_back(current);
  while (cameFrom.find(current) != cameFrom.end()) {
    current = cameFrom.at(current);
    path.emplace_back(current);
  }
  return path;
}

void* aStar(void *threadArgs) {
  WorkerArgs* args = static_cast<WorkerArgs*>(threadArgs);  

  std::vector<int> neighbors;
  bool waiting = false;
  while (true) {
    muxTermCount.lock();
    // check whether all threads have terminated 
    if (termCount == args->numThreads) {
      muxTermCount.unlock();
      break;
    }
    muxTermCount.unlock();
  
    muxPath.lock();
    int curPathCost = pathCost;
    muxPath.unlock();
    // if a thread sees an empty queue, enters waiting state
    muxPq.lock();
    if ((pq.empty() || pq.top().cost >= curPathCost)) {
      muxPq.unlock();
      if (!waiting) {
        waiting = true;
        muxTermCount.lock();
        termCount++;
        muxTermCount.unlock();
      }
      continue;
    } 

    node_info_t current = pq.top();
    pq.pop();
    openSet.erase(current.node);
    closedSet.insert(current.node);
    // printf("%d processing %d\n", args->threadId, current.node);
    muxPq.unlock();

    // if waiting and pq is no longer empty
    if (waiting) {
      waiting = false;
      muxTermCount.lock();
      termCount--;
      muxTermCount.unlock();
    }
    
    muxPath.lock();
    curPathCost = pathCost;
    muxPath.unlock();

    // solution found
    if (current.node == args->target) {
      muxCameFrom.lock();
      std::vector<int> newPath = reconstructPath(current.node);
      int newPathCost = current.cost;
      muxCameFrom.unlock();
      while (newPath.back() != args->source) {
        muxCameFrom.lock();
        std::vector<int> newPath = reconstructPath(current.node);
        int newPathCost = current.cost;
        muxCameFrom.unlock();
      }
      if (newPathCost < pathCost) {
        path = newPath;
        pathCost = newPathCost;
      }
    }

    neighbors = getNeighbors(current.node, neighbors);
    for (int neighbor: neighbors) { 
      muxScore.lock();
      int neighborScore = gScore.at(neighbor);
      int currentScore = gScore.at(current.node) + 1;
      muxScore.unlock();
      muxPq.lock();
      bool inClosed = closedSet.find(neighbor) != closedSet.end();
      bool inOpen = openSet.find(neighbor) != openSet.end();
      muxPq.unlock();
      int neighborfScore = currentScore + h(neighbor, args->target);
      if (inClosed) {
        if (currentScore < neighborScore) {
          muxScore.lock();
          gScore.erase(neighbor);
          gScore.emplace(neighbor, currentScore);
          muxScore.unlock();  
          muxPq.lock();
          closedSet.erase(neighbor);
          openSet.insert(neighbor);
          pq.push({neighborfScore, neighbor});
          muxPq.unlock();
        } else {
          continue;
        }
      } else {
          if (!inOpen) {
            muxScore.lock();
            gScore.erase(neighbor);
            gScore.emplace(neighbor, currentScore);
            muxScore.unlock();  
            muxPq.lock();
            openSet.insert(neighbor);
            pq.push({neighborfScore, neighbor});
            muxPq.unlock();
          } else if (currentScore >= neighborScore) {
              continue;
          }
      }
      
      muxCameFrom.lock();
      cameFrom.emplace(neighbor, current.node);
      muxCameFrom.unlock();
      
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
  graph = readGraph(x1, y1, x2, y2, inputFilename);
  printf("(%d, %d) to (%d, %d)\n", x1, y1, x2, y2);
  
  const static int MAX_THREADS = 32;

  if (numThreads > MAX_THREADS) {
    fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
    exit(1);
  }

  pthread_t workers[MAX_THREADS];
  WorkerArgs args[MAX_THREADS];

  int source = x1*graph->dim + y1;
  int target = x2*graph->dim + y2;
  printf("source: %d, target: %d\n", source, target);

  for (int i = 0; i < numThreads; i++) {
    args[i].threadId = i;
    args[i].numThreads = numThreads;
    args[i].source = source;
    args[i].target = target;
  }

  pq.push({h(source, target), source});
  openSet.insert(source);

  // gScore represents the cost of the cheapest path from start to current node
  gScore.emplace(source, 0);
  
  // initialize all other nodes to have an inf g score 
  for (int i = 0; i < graph->dim; i++) {
    for (int j = 0; j < graph->dim; j++) {
      int index = i*graph->dim + j;
      if (index != source) {
        gScore.insert({i*graph->dim + j, INT_MAX});
      }
    }
  }

  init_time += duration_cast<dsec>(Clock::now() - init_start).count();
  printf("Initialization Time: %lf.\n", init_time);

  auto compute_start = Clock::now();
  double compute_time = 0;
  
  for (int i = 1; i < numThreads; i++)
    pthread_create(&workers[i], NULL, aStar, &args[i]);

  aStar(&args[0]);

  for (int i = 1; i < numThreads; i++)
    pthread_join(workers[i], NULL);
  
  compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);

  free(graph->grid);
  writeOutput(inputFilename, path, graph);
}
