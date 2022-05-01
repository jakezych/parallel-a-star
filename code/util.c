#include "graph.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>

/* 
  Reads the graph from the input file with the inputted name. The first line of
  the file represents the dimenensions of the grid (i.e. 5 => 5x5).
*/
std::shared_ptr<graph_t> readGraph(int x1, int y1, int x2, int y2, char *inputFilename) {
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
  int lineSize = 2*dim+1;
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
        grid[dim*lineCount + nodeCount] = (int)(line[i] - '0');
        nodeCount++;
      }
    }
    lineCount++;
  }
  fclose(input);
  free(line);
  return std::shared_ptr<graph_t>(new graph_t(dim, grid));
}

/* 
 * writes the output of the A* algorithm to a file in the following format 
 * rs cs rt ct              where s is the source node and t is the target node 
 * n                        where n is the length of the path 
 * (r1, c1) (r2, c2) ... (rn, cn) the path outputted by the algorithm
 */
void writeOutput(char *inputFilename, std::vector<int> ret, std::shared_ptr<graph_t> graph) {
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

  // output the length of the most optimal path (edge weights assumed to be 1)
  outputFile << ret.size() << std::endl;

  // output the path
  for (auto n = ret.rbegin(); n != ret.rend(); n++) {
    outputFile << "(" <<  *n / graph->dim << "," << *n % graph->dim << ") "; 
  }
  outputFile.close();
}