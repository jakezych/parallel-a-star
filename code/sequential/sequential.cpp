#include "../graph.h"
#include <unistd.h>
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>
#include <unordered_map>
#include <climits>
#include <unordered_set>
#include <fstream>

/* DEBUG */
void printGraph(graph_t graph) {
  for (int i = 0; i < graph.dim; i++) {
    for (int j = 0; j < graph.dim; j++) {
      printf("%d ", graph.grid[i*graph.dim + j]);
    }
    printf("\n");
  }
}

/* reads the graph from the file and validates the source and target nodes */
graph_t readGraph(int x1, int y1, int x2, int y2, char *inputFilename) {
  int dim; 

	FILE *input = fopen(inputFilename, "r");
	if (!input) {
		printf("Unable to open file: %s.\n", inputFilename);
		return {};
	}

  // Read the dimension of the grid
  fscanf(input, "%d\n", &dim);

  if (x1 < 0 || x1 >= dim || y1 < 0 || y1 >= dim) {
    printf("first path point dimension error: %d, %d out of bounds for graph of dim %d\n", x1, y1, dim);
    return {};
  }
  if (x2 < 0 || x2 >= dim || y2 < 0 || y2 >= dim) {
    printf("second path point dimension error: %d, %d out of bounds for graph of dim %d\n", x2, y2, dim);
    return {};
  }

  int *grid = (int *)malloc(dim*dim*sizeof(int));

  // account for spaces by multiplying by 2
  int lineSize = 2*dim + 1;
  char* line = (char *)malloc(lineSize);
  int lineCount = 0;
  int nodeCount = 0;

  // read row by row
  while (fgets(line, lineSize, input))  {
    nodeCount = 0;
    // ignore last two characters
    for (int i = 0; i < lineSize - 2; i++) {
      if (!isspace(line[i])) {
        // convert char to int
        grid[dim*lineCount + nodeCount] = line[i] - '0';
        nodeCount++;
      }
    }
    lineCount++;
  }
  fclose(input);
  return {dim, grid};
}

/* 
 * writes the output of the A* algorithm to a file in the following format 
 * rs cs rt ct              where s is the source node and t is the target node 
 * n                        where n is the length of the path 
 * (r1, c1) (r2, c2) ... (rn, cn) the path outputted by the algorithm
 */
void writeOutput(char *inputFilename, graph_t graph, std::vector<node_t> ret, node_t source, node_t target) {
  std::string full_filename = std::string(inputFilename);
  std::string base_filename = full_filename.substr(full_filename.find_last_of("/\\") + 1);
  std::string::size_type const p(base_filename.find_last_of('.'));
  std::string file_without_extension = base_filename.substr(0, p);

  std::stringstream outputStream;
  outputStream << "output_" << file_without_extension << ".txt";

  std::string outputName = outputStream.str();
  std::ofstream outputFile;
  outputFile.open(outputName);
  if (!outputFile) {
    std::cout << "Error opening file " << outputName << std::endl;
    return;
  }

  // print source and target here 
  outputFile << source.row << " " << source.col << " ";
  outputFile << target.row << " " << target.col << std::endl;

  // output the length of the most optimal path (edge weights assumed to be 1)
  outputFile << ret.size() << std::endl;

  // output the path
  for (auto n = ret.rbegin(); n != ret.rend(); n++) {
    outputFile << "(" <<  n->row << ", " << n->col << ") "; 
  }
  outputFile << std::endl;
  outputFile.close();
}

// heuristic function 
int h(node_t source, node_t target) {
  // manhatten distance
  return std::abs(source.row - target.row) + std::abs(source.col - target.col);
}

/* 
 * returns the neighbors of the current node as a vector. Checks whether or
 * not they exist (if they are within the grid and equal to 1)
 */
std::vector<node_t> getNeighbors (node_t current, graph_t graph) {
  std::vector<node_t> result;

  int i = current.row*graph.dim + current.col;
  // element above
  if (current.row != 0 && graph.grid[i - graph.dim]) {
    result.push_back({current.row - 1, current.col});
  }
  // element below
  if (current.row < graph.dim - 1 && graph.grid[i + graph.dim]) {
    result.push_back({current.row + 1, current.col});
  }
  // element to the left 
  if (current.col != 0 && graph.grid[i - 1]) {
    result.push_back({current.row, current.col - 1});
  }
  // element to the right
  if (current.col < graph.dim - 1 && graph.grid[i + 1]) {
    result.push_back({current.row, current.col + 1});
  }
  return result;
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

std::vector<node_t> aStar(node_t source, node_t target, graph_t graph) {
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

  while (!pq.empty()) {
    node_t current = pq.top().node;
    // find solution
    if (current == target) {
      path = reconstructPath(cameFrom, current);
      break;
    }

    pq.pop();
    openSet.erase(current);
    std::vector<node_t> neighbors = getNeighbors(current, graph);
    for (node_t neighbor: neighbors) { 
      int neighborScore = gScore.find(neighbor) != gScore.end() ? gScore.at(neighbor) : INT_MAX;
      // every edge has weight 1
      int currentScore = gScore.at(current) + 1;
      if (currentScore < neighborScore) {
        cameFrom.insert({neighbor, current});
        gScore.insert({neighbor, currentScore});
        int neighborfScore = currentScore + h(neighbor, target);
        fScore.insert({neighbor, neighborfScore});
        if (openSet.find(neighbor) == openSet.end()) {
          openSet.insert(neighbor);
          pq.push({neighborfScore, neighbor});
        }
      }
    }
  assert(pq.size() == openSet.size());
  }

  return path;
}

// TODO: move file i/o to utility file
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

  graph_t graph = readGraph(x1, y1, x2, y2, inputFilename);
  
  node_t source = {x1, y1};
  node_t target = {x2, y2};

  std::vector<node_t> ret = aStar(source, target, graph);

  writeOutput(inputFilename, graph, ret, source, target);
}

// TODO: testing infrastructure, larger and more varied outputs 
