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
// golden ratio used for hashing 
double A = (std::sqrt(5) - 1) / 2;

// request status variables 
MPI_Request nodeRequests[MAX_PROCS];
MPI_Request costRequests[MAX_PROCS];
MPI_Request cameFromRequests[MAX_PROCS];
MPI_Status nodeStatuses[MAX_PROCS];
MPI_Status costStatuses[MAX_PROCS];
MPI_Status cameFromStatuses[MAX_PROCS];

// heuristic function 
int h(int source, int target) {
  int sourceR = source / graph->dim;
  int sourceC = source % graph->dim;
  int targetR = target / graph->dim;
  int targetC = target % graph->dim;
  // euclidian distance
  return sqrt(std::abs(sourceR - targetR)*std::abs(sourceR - targetR) + std::abs(sourceC - targetC)*std::abs(sourceC - targetC));
  // manhatten distance 
  // return std::abs(sourceR - targetR) + std::abs(sourceC - targetC);
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
void reconstructPath(std::unordered_map<int, int> cameFrom, int current, std::vector<int> *path) {
  path->clear();
  path->emplace_back(current);
  while (cameFrom.find(current) != cameFrom.end()) {
    current = cameFrom.at(current);
    path->emplace_back(current);
  }
}

int key(int state) {
  return state;
}

int computeRecipient(int nproc, int state) {
  int k = key(state);
  // add two since first two tags are reserved
  return std::floor(nproc*(k*A - std::floor(k*A)));
}

void aStar(int source, int target, std::shared_ptr<graph_t> graph, std::vector<int> *path, int nproc, int procID) {
  std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo> pq;
  std::unordered_set<int> openSet;
  std::unordered_map<int, int> cameFrom;
  std::unordered_map<int, int> gScore;  
  std::unordered_set<int> closedSet;
  int pathCost = INT_MAX;
  int validPathExists = 0;
  
  // initialize open set for the recipient processor
  if (procID == computeRecipient(nproc, 0)) {
    // printf("proc %d initializes pq\n", procID);
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
      /*
      if (procID == 0) {
        printf("computeRecipient(%d, %d) = %d\n", nproc, i*graph->dim+j, computeRecipient(nproc, i*graph->dim+j));
      }
      */
    }
  }

  int pathCostBuf;
  int nodeRecvBuf[3];
  int nodeSendBuf[3];
  int cameFromBuf[2];
  std::vector<int> neighbors;
  int outstandingNodeRequest = 0;
  int outstandingPathRequest = 0;
  int outstandingCameFromRequest = 0;
  while (true) { 
    int nodeReady;
    if (!outstandingNodeRequest) {
      // check whether or not a node is ready to be processed
      MPI_Irecv(&nodeRecvBuf, 3, MPI_INT, MPI_ANY_SOURCE, procID, MPI_COMM_WORLD, &nodeRequests[procID]);
      outstandingNodeRequest = 1;
    }
    MPI_Test(&nodeRequests[procID], &nodeReady, &nodeStatuses[procID]);
    
    // printf("proc %d nodeReady: %d\n", procID, nodeReady);
    if (nodeReady) {
      outstandingNodeRequest = 0; 
      // buffer can be processed 
      int neighbor = nodeRecvBuf[0];
      int currentScore = nodeRecvBuf[1];
      int current = nodeRecvBuf[2]; 
      // printf("proc %d processing (%d, %d, %d)\n", procID, neighbor, currentScore, current);
      int neighborfScore = currentScore + h(neighbor, target);
      int neighborScore = gScore.at(neighbor);
      if (closedSet.find(neighbor) != closedSet.end()) {
        if (currentScore < neighborScore) {
          // printf("proc %d adding %d to pq\n", procID, neighbor);
          closedSet.erase(neighbor);
          openSet.insert(neighbor);
          pq.push({neighborfScore, neighbor});
        } else {
          continue;
        }
      } else {
        if (openSet.find(neighbor) == openSet.end()) {
          // printf("proc %d adding %d to pq\n", procID, neighbor);
          openSet.insert(neighbor);
          pq.push({neighborfScore, neighbor});
        } else if (currentScore >= neighborScore) {
          continue;
        }
      }

      // printf("proc %d neighborScore %d\n", procID, neighborScore);
      // key
      cameFromBuf[0] = neighbor;
      // value
      cameFromBuf[1] = current;
      // broadcast all dictionary updates 
      // printf("proc %d sent dict update (%d, %d)\n", procID, neighbor, current);
      for (int i = 0; i < nproc; i++) {
        MPI_Isend(&cameFromBuf, 2, MPI_INT, i, nproc, MPI_COMM_WORLD, &cameFromRequests[i]);
      }

      gScore.erase(neighbor);
      gScore.emplace(neighbor, currentScore);    
    }
    
    // dict update check here 
    int cameFromUpdate; 
    if (!outstandingCameFromRequest) {
      MPI_Irecv(&cameFromBuf, 2, MPI_INT, MPI_ANY_SOURCE, nproc, MPI_COMM_WORLD, &cameFromRequests[procID]);
      outstandingCameFromRequest = 1;
    }

    MPI_Test(&cameFromRequests[procID], &cameFromUpdate, &cameFromStatuses[procID]);

    // when is this gonna stop? there are no more messages? How do you know the path is constructed? 
    // need to process all? 
    // update cameFrom dictionary 
    if(cameFromUpdate) {
      outstandingCameFromRequest = 0;
      int neighbor = cameFromBuf[0];
      int current = cameFromBuf[1];
      // printf("proc %d received dict update (%d, %d)\n", procID, neighbor, current);
      if (cameFrom.find(current) != cameFrom.end()) {
          if (cameFrom.at(current) != neighbor) {
            cameFrom.emplace(neighbor, current);
          }
        } else {
          cameFrom.emplace(neighbor, current);
      }
    }
    
    int newPathCostExists;
    if (!outstandingPathRequest) {
      MPI_Irecv(&pathCostBuf, 1, MPI_INT, MPI_ANY_SOURCE, nproc+1, MPI_COMM_WORLD, &costRequests[procID]);
      outstandingPathRequest = 1;
    }
    // check if message with new path cost was received
    MPI_Test(&costRequests[procID], &newPathCostExists, &costStatuses[procID]);
    if (newPathCostExists) {
      outstandingPathRequest = 0;
      if (!validPathExists || pathCostBuf < pathCost) {
        validPathExists = 1;
        printf("proc %d received new cost %d\n", procID, pathCostBuf);
        pathCost = pathCostBuf;
      }
    }

    // need to prove that there is no better solution 
    // check local open list to see if there is no more work to be done 
    // also need to ensure no other messages are being sent 
    if (pq.empty() || pq.top().cost >= pathCost) {
      if (validPathExists) {
        MPI_Status doneStatus;
        int done;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &done, &doneStatus);
        // printf("proc %d message exists check\n", procID);
        //int count;
        //MPI_Get_count(&doneStatus, MPI_INT, &count);
        // printf("proc %d done %d\n", procID, done);
        if (done == 0) {
          printf("proc %d exiting\n", procID);
          break;
        }
      }
      continue;
    }
    // nodes exist to be expanded
    int current = pq.top().node;
    // printf("proc %d pq size before %d\n", procID, pq.size());
    pq.pop();
    // printf("proc %d pq size after %d\n", procID, pq.size());
    openSet.erase(current);
    closedSet.insert(current);
    // printf("proc %d closing %d \n", procID, current);

    // solution found
    if (current == target) {
      reconstructPath(cameFrom, current, path);
      printf("proc %d path ", procID);
      for (auto n = path->rbegin(); n != path->rend(); n++) {
        printf("%d, ", *n); 
      }
      printf("\n");
      pathCost = path->size();
      // while the path is invalid
      while (pathCost < 2 || path->back() != source) {
        // dict update check here 
        int cameFromUpdate; 
        if (!outstandingCameFromRequest) {
          MPI_Irecv(&cameFromBuf, 2, MPI_INT, MPI_ANY_SOURCE, nproc, MPI_COMM_WORLD, &cameFromRequests[procID]);
          outstandingCameFromRequest = 1;
        }

        MPI_Test(&cameFromRequests[procID], &cameFromUpdate, &cameFromStatuses[procID]);
        if(cameFromUpdate) {
          outstandingCameFromRequest = 0;
          int neighbor = cameFromBuf[0];
          int current = cameFromBuf[1];
          printf("proc %d received dict update (%d, %d)\n", procID, neighbor, current);
          if (cameFrom.find(current) != cameFrom.end()) {
              if (cameFrom.at(current) != neighbor) {
                cameFrom.emplace(neighbor, current);
              }
            } else {
              cameFrom.emplace(neighbor, current);
          }
        }
        reconstructPath(cameFrom, current, path);
        printf("proc %d path ", procID);
        for (auto n = path->rbegin(); n != path->rend(); n++) {
          printf("%d, ", *n); 
        }
        printf("\n");
        pathCost = path->size();
      }
      // broadcast new path cost 
      printf("proc %d broadcasting pathCost %d\n", procID, pathCost);
      for (int i = 0; i < nproc; i++) {
        MPI_Isend(&pathCost, 1, MPI_INT, i, nproc+1, MPI_COMM_WORLD, &costRequests[i]);
      }
      continue;
    }

    neighbors = getNeighbors(current, graph, neighbors);
    for (int neighbor: neighbors) {
      int currentScore = gScore.at(current) + 1;
      nodeSendBuf[0] = neighbor;
      nodeSendBuf[1] = currentScore;
      nodeSendBuf[2] = current;
      int recipient = computeRecipient(nproc, neighbor);
      // send neighbor to be processed by another processor
      // printf("proc %d sent (%d, %d, %d) to %d\n", procID, neighbor, currentScore, current, recipient);
      MPI_Isend(&nodeSendBuf, 3, MPI_INT, recipient, recipient, MPI_COMM_WORLD, &nodeRequests[recipient]);
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

  std::vector<int> *path = new std::vector<int>;

  printf("Initialization Time for proc %d: %lf.\n", procID, init_time);

  double startTime;
  double endTime;
  
  startTime = MPI_Wtime();
  aStar(source, target, graph, path, nproc, procID);
  endTime = MPI_Wtime();

  printf("Computation Time for proc %d: %f\n", procID, endTime-startTime);
  

  free(graph->grid);

  if (path->size() != 0) {
    printf("proc %d writing path of size %d\n", procID, path->size());
    writeOutput(inputFilename, *path, graph);
  }
  free(path);
  MPI_Finalize();
}