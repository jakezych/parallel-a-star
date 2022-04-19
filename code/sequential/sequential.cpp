#include "../graph.h"
#include <unistd.h>
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>
#include <unordered_map>
#include <climits>

void printGraph(graph_t graph) {
  for (int i = 0; i < graph.dim; i++) {
    for (int j = 0; j < graph.dim; j++) {
      printf("%d ", graph.grid[i*graph.dim + j]);
    }
    printf("\n");
  }
}
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

// heuristic function 
int manhattenDistance(node_t source, node_t target) {
  return std::abs(source.row - target.row) + std::abs(source.col - target.col);
}


int *aStar(node_t source, node_t target, graph_t graph) {
  std::priority_queue<node_info_t, std::vector<node_info_t>, CompareNodeInfo>pq;
  std::unordered_map<node_t, node_t> cameFrom;
  std::unordered_map<node_t, int> gScore;
  std::unordered_map<node_t, int> fScore;

  pq.push({manhattenDistance(source, target), source});
  gScore.insert({source, INT_MAX});

  return NULL;
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

  graph_t graph = readGraph(x1, y1, x2, y2, inputFilename);
  printGraph(graph);

  int *ret = aStar({x1, y1}, {x2, y2}, graph);
}

// how are we taking input to main? 
 // read from and write output (path / cost) to file? 
 // graph input file format: grid 
  // can read directly into our adjacency matrix
// how are we defining graphs? 
  // adjacency matrix (might be easier to work with, space inefficient for sparse graph)
  // adjacency list (harder to work with, need to encode edge weights)
// how are we taking input to a* 
  // pass start node, target node, and graph


