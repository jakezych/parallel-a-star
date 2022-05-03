package main

import (
	"container/heap"
	"fmt"
	"math"
	"os"
	"strconv"
	"time"
	"github.com/418astar/goutil"
)

var graph *goutil.Graph

func h(source, target int) int {
	sourceR := float64(source) / float64(graph.Dim);
  sourceC := float64(source % graph.Dim);
	targetR := float64(target) / float64(graph.Dim);
  targetC := float64(target % graph.Dim);
  // euclidian distance
  return int(math.Sqrt(math.Abs(sourceR - targetR)*math.Abs(sourceR - targetR) + math.Abs(sourceC - targetC)*math.Abs(sourceC - targetC)))
}

func reconstructPath(cameFrom map[int]int, current int) []int {
	path := make([]int, 1)
	path[0] = current
	for {
		if _, exists := cameFrom[current]; !exists {
			break
		}
		current = cameFrom[current]
		// fmt.Println("current =", current)
		path = append(path, current)
	}
	return path
}

func getNeighbors(current int) []int {
	neighbors := make([]int, 0)
	currentR := current / graph.Dim;
  currentC := current % graph.Dim;
  // element above
  if (currentR != 0 && graph.Grid[current - graph.Dim] != 0) {
    neighbors = append(neighbors, current - graph.Dim)
  }
  // element below
  if (currentR < graph.Dim - 1 && graph.Grid[current + graph.Dim] != 0) {
    neighbors = append(neighbors, current + graph.Dim)
  }
  // element to the left 
  if (currentC != 0 && graph.Grid[current - 1] != 0) {
    neighbors = append(neighbors, current - 1)
  }
  // element to the right
  if (currentC < graph.Dim - 1 && graph.Grid[current + 1] != 0) {
    neighbors = append(neighbors, current + 1)
  }
  return neighbors;
}

func astar(source, target int) []int {
	//init priority queue
	pq := make(goutil.PriorityQueue, 0)
	heap.Init(&pq)

	openSet := make(map[int]struct{})
	cameFrom := make(map[int]int)
	gScore := make(map[int]int)
	var path []int

	// initialize open set 
	firstNode := &goutil.NodeInfo{
		Node: source,
		Cost: h(source, target),
	}
	heap.Push(&pq, firstNode)
	openSet[source] = struct{}{}

	// gScore represents the cost of the cheapest path from start to current node
  gScore[source] = 0
	for i := range graph.Grid {
		if i != source {
			gScore[i] = math.MaxInt64
		}
	}

	var neighbors []int
	for pq.Len() > 0 {
		// fmt.Println("beg loop, pq size =", pq.Len())
		current := heap.Pop(&pq).(*goutil.NodeInfo).Node
		if current == target {
			path = reconstructPath(cameFrom, current)
			// fmt.Println("path found, len = ", len(path))
			break
		}

		delete(openSet, current)
		neighbors = getNeighbors(current)
		for _, neighbor := range neighbors {
			// fmt.Println("current =", current, "neighbor =", neighbor)
			neighborScore := gScore[neighbor]
			currentScore := gScore[current] + 1

			// fmt.Println("curr score", currentScore, ", neighbor score", neighborScore)
			if currentScore < neighborScore {
				if elem, exists := cameFrom[current]; exists && elem != neighbor {
					cameFrom[neighbor] = current
				} else {
					cameFrom[neighbor] = current
				}
				gScore[neighbor] = currentScore
				neighborFScore := currentScore + h(neighbor, target)
				if _, exists := openSet[neighbor]; !exists {
					openSet[neighbor] = struct{}{}
					node := &goutil.NodeInfo{
						Node: neighbor,
						Cost: neighborFScore,
					}
					// fmt.Println("pushing node onto pq:", neighbor)
					heap.Push(&pq, node)
				}
			}
		}
	}
	return path
}

func main() {
	startTime := time.Now()

	if len(os.Args) < 6 {
		fmt.Println("Usage: go run <filename> <x1> <y1> <x2> <y2>")
		return
	}

	inputFilename := os.Args[1]
	x1, _ := strconv.Atoi(os.Args[2])
	y1, _ := strconv.Atoi(os.Args[3])
	x2, _ := strconv.Atoi(os.Args[4])
	y2, _ := strconv.Atoi(os.Args[5])

	graph = goutil.ReadGraph(x1, y1, x2, y2, inputFilename)
	if graph == nil {
		fmt.Println("ReadGraph failed")
		return
	}

	initTime := time.Since(startTime)
	fmt.Println("Initialization Time: ", initTime)

	midTime := time.Now()

	source := x1*graph.Dim + y1
	target := x2*graph.Dim + y2
	path := astar(source, target)

	computeTime := time.Since(midTime)
	fmt.Println("Computation Time: ", computeTime)

	goutil.WriteOutput(inputFilename, path, graph)
}