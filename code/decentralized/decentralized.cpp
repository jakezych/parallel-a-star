#include "../graph.h"
#include "../util.c"
#include "mpi.h"
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

const static int MAX_PROCS = 64;

// request status variables 
MPI_Request nodeRequests[MAX_PROCS];
MPI_Request costRequests[MAX_PROCS];
MPI_Status nodeStatuses[MAX_PROCS];
MPI_Status costStatuses[MAX_PROCS];

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

int computeRecipient(int nprocs) {
  return std::rand() % nprocs;
}

void aStar(int source, int target, std::shared_ptr<graph_t> graph, std::vector<int> path, int nproc, int procID) {
  std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo> pq;
  std::unordered_set<int> openSet;
  std::unordered_map<int, int> cameFrom;
  std::unordered_map<int, int> gScore;  
  int pathCost = INT_MAX;
  int pathCostBuf;
  printf("procID in a*:%d\n", procID);
  printf("nproc in a*:%d\n", nproc);
  
  // initialize open set for the recipient processor
  if (procID == computeRecipient(nproc)) {
    printf("proc %d initializes pq\n", procID);
    pq.push({h(source, target), source});
    openSet.insert(source);

    // gScore represents the cost of the cheapest path from start to current node
  }
  gScore.insert({source, 0});

  // initialize all other nodes to have an inf g score 
  for (int i = 0; i < graph->dim; i++) {
    for (int j = 0; j < graph->dim; j++) {
      if (i != source / graph->dim || j != source % graph->dim) {
        gScore.insert({i*graph->dim + j, INT_MAX});
      }
    }
  }
  int nodeRecvBuf[3];
  int nodeSendBuf[3];
  std::vector<int> neighbors;
  while (true) { 
    int nodeReady;
    // check whether or not a node is ready to be processed
    for (int i = 0; i < nproc; i++) {
      MPI_Irecv(&nodeRecvBuf, 3, MPI_INT, i, 0, MPI_COMM_WORLD, &nodeRequests[i]);
      MPI_Test(&nodeRequests[i], &nodeReady, &nodeStatuses[i]);
      
      printf("proc %d nodeReady: %d check for proc %d\n", procID, nodeReady, i);
      if (nodeReady) {
        // buffer can be processed 
        int neighbor = nodeRecvBuf[0];
        int currentScore = nodeRecvBuf[1];
        int current = nodeRecvBuf[2]; 
        printf("proc %d processing (%d, %d, %d)\n", procID, neighbor, currentScore, current);
        int neighborScore = gScore.at(neighbor);
        if (currentScore < neighborScore) {
          printf("proc %d currentScore %d neighborScore %d\n", procID, currentScore, neighborScore);
          if (cameFrom.find(current) != cameFrom.end()) {
            if (cameFrom.at(current) != neighbor) {
              cameFrom.emplace(neighbor, current);
            }
          } else {
            cameFrom.emplace(neighbor, current);
          }
          // check if this is working
          gScore.erase(neighbor);
          gScore.emplace(neighbor, currentScore);
          printf("proc %d neighbor: %d gscore: %d\n", procID, neighbor, gScore.at(neighbor));
          int neighborfScore = currentScore + h(neighbor, target);
          if (openSet.find(neighbor) == openSet.end()) {
            openSet.emplace(neighbor);
            pq.push({neighborfScore, neighbor});
          }
        }
      }
    }
    int newPathCostExists;
    // printf("cost source: %d\n", costStatuses[procID].MPI_SOURCE);
    MPI_Irecv(&pathCostBuf, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &costRequests[procID]);
    // check if message with new path cost was received
    MPI_Test(&costRequests[procID], &newPathCostExists, &costStatuses[procID]);
    if (newPathCostExists) {
      printf("proc %d received new cost %d\n", procID, pathCostBuf);
      pathCost = pathCostBuf;
      // need to prove that there is no better solution 
      // check local open list to see if there is no more work to be done 
      // also need to ensure no other messages are being sent 
      if (pq.empty()) {
        MPI_Status doneStatus;
        int messageExists; 
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &messageExists, &doneStatus);
        printf("proc %d message exists check %d\n", procID, messageExists);
        if (messageExists == 0) {
          printf("proc %d exiting\n", procID);
          break;
        }
      }
    }

    // if no work in local queue and no messages loop
    if (pq.empty() || pq.top().cost >= pathCost) {
      continue;
    }

    // nodes exist to be expanded
    int current = pq.top().node;
    pq.pop();
    openSet.erase(current);
    printf("proc %d current: %d\n", procID, current);
    // solution found
    if (current == target) {
      path = reconstructPath(cameFrom, current);
      pathCost = path.size();
      // broadcast new path cost 
      printf("proc %d broadcasting pathCost %d\n", procID, pathCost);
      for (int i = 0; i < nproc; i++) {
        MPI_Isend(&pathCost, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &costRequests[i]);
      }
      // TODO: make sure message also being sent to self 
      continue;
    }

    neighbors = getNeighbors(current, graph, neighbors);
    for (int neighbor: neighbors) {
      // this overflows sometimes (do we need to broadcast gScore updates?? seems like that would defeat the purpose)
      int currentScore = gScore.at(current) + 1;
      nodeSendBuf[0] = neighbor;
      nodeSendBuf[1] = currentScore;
      nodeSendBuf[2] = current;
      int recipient = computeRecipient(nproc);
      // send neighbor to be processed by another processor
      printf("proc %d sent (%d, %d, %d) to %d\n", procID, neighbor, currentScore, current, recipient);
      MPI_Isend(&nodeSendBuf, 3, MPI_INT, recipient, 0, MPI_COMM_WORLD, &nodeRequests[recipient]);
    }
    neighbors.clear();
  }
}


int main(int argc, char *argv[]) {
  using namespace std::chrono;
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::duration<double> dsec;

  auto init_start = Clock::now();
  double init_time = 0;

  int procID;
  int nproc;
  int opt = 0;
  char *inputFilename = NULL;
  int x1 = -1;
  int y1 = -1;
  int x2 = -1;
  int y2 = -1;

  MPI_Init(&argc, &argv);

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
      MPI_Finalize();
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

  // Get process rank
  MPI_Comm_rank(MPI_COMM_WORLD, &procID);
  // Get total number of processes specificed at start of run
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  std::vector<int> path(0, 0);

  printf("Initialization Time for proc %d: %lf.\n", procID, init_time);

  double startTime;
  double endTime;
  
  startTime = MPI_Wtime();
  aStar(source, target, graph, path, nproc, procID);
  endTime = MPI_Wtime();

  printf("Computation Time for proc %d: %f", procID, endTime-startTime);
  

  free(graph->grid);
  // this is causes issues if you don't have a path 
  int pathCost = path.size();
  int *bestCost = (int *)malloc(sizeof(int));
  printf("proc %d called path.size()\n", procID);
  // CHANGED THIS TO MPI_MAX 
  // REWRITE
  MPI_Reduce(&pathCost, bestCost, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
  // if processor has lowest cost, write the path to file 
  // TODO: might be race conditions here 
  if (path.size() == *bestCost) {
    writeOutput(inputFilename, path, graph);
  }

  MPI_Finalize();
}