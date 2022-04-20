import logging
import sys

FORMAT = '%(asctime)-15s - %(levelname)s - %(module)10s:%(lineno)-5d - %(message)s'
logging.basicConfig(stream=sys.stdout, level=logging.INFO, format=FORMAT)
LOG = logging.getLogger(__name__)

# modified from 15-418-s22-assignment-4 validate.py 
# https://github.com/cmu15418s22/Assignment-4/blob/master/code/validate.py
help_message = '''
usage: validate.py [-h][-i INPUT] [-o OUTPUT]
Validates whether the path exists and has the same cost as outputted. Outputs if
there exists a more optimal path. 
Arguments:
  -h, --help            Show this help message and exit
  -o OUTPUT             Outputted file from a*
  -i INPUT              Input file for a*
'''

def parseArgs():
    args = sys.argv
    if '-h' in args or '--help' in args:
        print(help_message)
        sys.exit(1)
    if '-i' not in args or '-o' not in args:
        print(help_message)
        sys.exit(1)
    parsed = {}
    parsed['input'] = args[args.index('-i') + 1]
    parsed['output'] = args[args.index('-o') + 1]
    return parsed

def readSolution(args):
    # output file
    output = open(args['output'], 'r')
    lines = output.readlines()
    if len(lines) < 2:
        LOG.error('''output file contains has less than 2 lines''')
        return False, 0, []
    size = lines[0]
    path = []
    for node in lines[1].split(' '):
      if len(node) == 0:
        break
      if not node[1].isnumeric() or not node[-2].isnumeric():
        LOG.error('''output file node contains non-numeric character''')
        return False, 0, []
      r = int(node[1])
      c = int(node[-2])
      path.append((r, c))
    
    return True, size, path

def readGrid(args):
  # input file
  input = open(args['input'], 'r')
  lines = input.readlines()
  if len(lines) == 0:
    LOG.error('''output file does not contain a dimension''')
    return False, []
  dim = lines[0]
  # account of \n of dim
  if len(dim) != 2 or not dim[0].isnumeric():
    LOG.error('''output file does not contain a properly formatted dimension''')
    return False, []
  dim = int(dim[0])
  if len(lines) < dim + 1:
      LOG.error('''output file does not contain enough lines''')
      return False, []

  grid = [[0 for i in range(dim)] for j in range(dim)]

  for i, line in enumerate(lines[1:]):
    for j, elem in enumerate(line.split(' ')):
      # ignore \n char on last elemn
      if j == dim - 1:
        elem = elem[0]
      if not elem.isnumeric():
        LOG.error('''output file grid contains a non-numeric character''')
        return False, []
      grid[i][j] = int(elem)
  
  return True, grid



def isAdjacent(v1, v2):
  return (abs(v1[0] - v2[0]) != 1) or (abs(v1[1] - v2[1]) != 1)

def validatePath(size, path, grid):
  if size == len(path):
    LOG.error(f'''outputted size and true path length mismatch size: {size} != length of path: {len(path)}''')
    return False
  if len(path) == 1: 
    LOG.error('''path only contains a single element''')
    return False

  prev = path[0]
  if grid[prev[0]][prev[1]] != 1:
    LOG.error('''invalid starting node''')
    return False

  for i in range(1, len(path)):
    cur = path[i]
    if grid[cur[0]][cur[1]] != 1:
      LOG.error(f'''path includes a node with no edge to it ({cur[0]}, {cur[1]})''')
      return False
    if not isAdjacent(prev, cur):
      LOG.error(f'''path includes an illegal move from: ({prev[0]}, {prev[1]}) to: ({cur[0]}, {cur[1]})''')
      return False
    prev = cur

  return True

def main(args):
    success, size, path = readSolution(args)
    if not success:
      return False
    success, grid = readGrid(args)
    if not success:
      return False
    validatePath(size, path, grid)
    #checkOptimality(path, grid)
    print("solution is valid")
    

if __name__ == '__main__':
    main(parseArgs())

# read from input file and save to 2d list
# follow path on graph and count 
# check if path is valid (edges exist) and if length is the same 
# run dijkstra's algorithm and compare cost to see if there exists a more optimal 
# solution