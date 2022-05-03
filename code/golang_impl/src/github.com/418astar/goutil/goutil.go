package goutil

import (
	"bufio"
	"container/heap"
	"fmt"
	"os"
	"strconv"
	"strings"
)

// A NodeInfo is something we manage in a priority queue.
type NodeInfo struct {
	Node    int // The value of the item
	Cost int    // The priority of the item in the queue.
	// The index is needed by update and is maintained by the heap.Interface methods.
	index int // The index of the item in the heap.
}

// A PriorityQueue implements heap.Interface and holds Items.
type PriorityQueue []*NodeInfo

func (pq PriorityQueue) Len() int { return len(pq) }

func (pq PriorityQueue) Less(i, j int) bool {
	// We want Pop to give us the highest, not lowest, priority so we use greater than here.
	return pq[i].Cost > pq[j].Cost
}

func (pq PriorityQueue) Swap(i, j int) {
	pq[i], pq[j] = pq[j], pq[i]
	pq[i].index = i
	pq[j].index = j
}

func (pq *PriorityQueue) Push(x interface{}) {
	n := len(*pq)
	item := x.(*NodeInfo)
	item.index = n
	*pq = append(*pq, item)
}

func (pq *PriorityQueue) Pop() interface{} {
	old := *pq
	n := len(old)
	item := old[n-1]
	old[n-1] = nil  // avoid memory leak
	item.index = -1 // for safety
	*pq = old[0 : n-1]
	return item
}

// update modifies the priority and value of an Item in the queue.
func (pq *PriorityQueue) update(item *NodeInfo, value int, priority int) {
	item.Node = value
	item.Cost = priority
	heap.Fix(pq, item.index)
}


type Graph struct {
	Dim  int
	Grid []int
} 

func ReadGraph(x1, y1, x2, y2 int, inputFilename string) *Graph {
	file, err := os.Open(inputFilename)
	if err != nil {
		fmt.Println("Filename ", inputFilename, " is invalid");
		return nil;
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	if !scanner.Scan() {
		fmt.Println("File must have dimension on first line")
		return nil
	}
	
	dim, _ := strconv.Atoi(scanner.Text())
	if x1 < 0 || x1 >= dim || y1 < 0 || y1 >= dim {
		fmt.Println("First path point dimension error: ", x1, ", ", y1, " out of bounds for graph of dim ", dim)
    return nil;
  }
	if x2 < 0 || x2 >= dim || y2 < 0 || y2 >= dim {
		fmt.Println("Second path point dimension error: ", x2, ", ", y2, " out of bounds for graph of dim ", dim)
    return nil;
  }
	
	g := new(Graph)
	g.Dim = dim
	g.Grid = make([]int, dim*dim)

	i := 0
	for scanner.Scan() {
		line := scanner.Text()
		nodes := strings.Split(line, " ")
		for j, node := range nodes {
			g.Grid[i*dim + j], _ = strconv.Atoi(node)
		}
		i++
	}

	return g
}