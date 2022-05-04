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

const MAX_THREADS = 32

type currNeighbor struct {
	current  int
	neighbor int
}

type threadInfo struct {
	graph             *goutil.Graph
	pq                goutil.PriorityQueue
	openSet           map[int]struct{}
	cameFrom          map[int]int
	gScore            map[int]int
	path              []int
	incrCount         chan bool
	decrCount         chan bool
	setCost           chan int
	checkDone         chan bool
	respondDone       chan bool
	checkCost         chan bool
	respondCost       chan int
	checkWait         chan int
	respondWait       chan *goutil.NodeInfo
	checkReconstruct  chan *goutil.NodeInfo
	respondReconstuct chan bool
	checkScore        chan currNeighbor
	respondScore      chan int
	checkCameFrom     chan currNeighbor
	respondCameFrom   chan bool
	checkPush         chan currNeighbor
	respondPush       chan bool
	killChan          chan bool
}

func h(source, target int, graph *goutil.Graph) int {
	sourceR := float64(source) / float64(graph.Dim)
	sourceC := float64(source % graph.Dim)
	targetR := float64(target) / float64(graph.Dim)
	targetC := float64(target % graph.Dim)
	// euclidian distance
	return int(math.Sqrt(math.Abs(sourceR-targetR)*math.Abs(sourceR-targetR) + math.Abs(sourceC-targetC)*math.Abs(sourceC-targetC)))
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

func getNeighbors(graph *goutil.Graph, current int) []int {
	neighbors := make([]int, 0)
	currentR := current / graph.Dim
	currentC := current % graph.Dim
	// element above
	if currentR != 0 && graph.Grid[current-graph.Dim] != 0 {
		neighbors = append(neighbors, current-graph.Dim)
	}
	// element below
	if currentR < graph.Dim-1 && graph.Grid[current+graph.Dim] != 0 {
		neighbors = append(neighbors, current+graph.Dim)
	}
	// element to the left
	if currentC != 0 && graph.Grid[current-1] != 0 {
		neighbors = append(neighbors, current-1)
	}
	// element to the right
	if currentC < graph.Dim-1 && graph.Grid[current+1] != 0 {
		neighbors = append(neighbors, current+1)
	}
	return neighbors
}

func astar(source, target int, data *threadInfo) {
	fmt.Println("launched astar")
	var neighbors []int
	waiting := false
	for {
		data.checkDone <- true
		result := <-data.respondDone
		if result {
			break
		}
		fmt.Println("not done yet")

		data.checkCost <- true
		currCost := <-data.respondCost
		fmt.Println("curr cost = ", currCost)

		data.checkWait <- currCost
		res := <-data.respondWait
		if res == nil {
			if !waiting {
				waiting = true
				data.incrCount <- true
			}
			fmt.Println("waiting")
			continue
		}
		if waiting {
			waiting = false
			data.decrCount <- true
		}

		data.checkCost <- true
		currCost = <-data.respondCost
		fmt.Println("not waiting, curr cost = ", currCost)

		if res.Node == target && res.Cost < currCost {
			data.checkReconstruct <- res
			<-data.respondReconstuct
			fmt.Println("found target")
		}

		neighbors = getNeighbors(data.graph, res.Node)
		for _, neighbor := range neighbors {
			data.checkScore <- currNeighbor{res.Node, neighbor}
			currentScore := <-data.respondScore
			if currentScore == -1 {
				continue
			}

			neighborFScore := currentScore + h(neighbor, target, data.graph)

			data.checkCameFrom <- currNeighbor{res.Node, neighbor}
			<-data.respondCameFrom

			data.checkPush <- currNeighbor{neighborFScore, neighbor}
			<-data.respondPush
		}
	}
}

func checkTermination(data *threadInfo, done chan bool, numThreads int) {
	count := 0
	for {
		select {
		case <-data.incrCount:
			count++
			if count == numThreads {
				done <- true
				return
			}
		case <-data.decrCount:
			count--
		case <-data.checkDone:
			if count == numThreads {
				data.respondDone <- true
			} else {
				data.respondDone <- false
			}
		}
	}
}

func getCost(data *threadInfo) {
	currCost := 0
	for {
		select {
		case <-data.checkCost:
			data.respondCost <- currCost
		case cost := <-data.setCost:
			currCost = cost
		case <-data.killChan:
			return
		}
	}
}

func getWaiting(data *threadInfo) {
	for {
		select {
		case currCost := <-data.checkWait:
			if data.pq.Len() == 0 || data.pq[0].Cost >= currCost {
				data.respondWait <- nil
				fmt.Println("gonna wait, nothin to process, pq len = ", data.pq.Len(), "pq cost = ", data.pq[0].Cost, "currCost = ", currCost)
			} else {
				current := heap.Pop(&data.pq).(*goutil.NodeInfo)
				delete(data.openSet, current.Node)
				data.respondWait <- current
			}
		case s := <-data.checkPush:
			if _, exists := data.openSet[s.neighbor]; !exists {
				data.openSet[s.neighbor] = struct{}{}
				node := &goutil.NodeInfo{
					Node: s.neighbor,
					Cost: s.current,
				}
				heap.Push(&data.pq, node)
			}
			data.respondPush <- true
		case <-data.killChan:
			return
		}
	}
}

func reconstruct(data *threadInfo) {
	for {
		select {
		case curr := <-data.checkReconstruct:
			current := curr.Node
			path := make([]int, 1)
			path[0] = current
			for {
				if _, exists := data.cameFrom[current]; !exists {
					break
				}
				current = data.cameFrom[current]
				path = append(path, current)
			}
			data.path = path
			fmt.Println("SET PATH of len", len(path))
			data.setCost <- curr.Cost
			data.respondReconstuct <- true
		case s := <-data.checkCameFrom:
			if elem, exists := data.cameFrom[s.current]; exists && elem != s.neighbor {
				data.cameFrom[s.neighbor] = s.current
			} else {
				data.cameFrom[s.neighbor] = s.current
			}
			data.respondCameFrom <- true
		case <-data.killChan:
			return
		}
	}
}

func score(data *threadInfo) {
	for {
		select {
		case s := <-data.checkScore:
			neighborScore := data.gScore[s.neighbor]
			currentScore := data.gScore[s.current] + 1
			if currentScore < neighborScore {
				data.gScore[s.neighbor] = currentScore
				data.respondScore <- currentScore
			} else {
				data.respondScore <- -1
			}
		case <-data.killChan:
			return
		}
	}
}

func main() {
	startTime := time.Now()

	if len(os.Args) < 6 {
		fmt.Println("Usage: go run <filename> <x1> <y1> <x2> <y2>")
		return
	}

	inputFilename := os.Args[1]
	numThreads, _ := strconv.Atoi(os.Args[2])
	x1, _ := strconv.Atoi(os.Args[3])
	y1, _ := strconv.Atoi(os.Args[4])
	x2, _ := strconv.Atoi(os.Args[5])
	y2, _ := strconv.Atoi(os.Args[6])

	if numThreads > MAX_THREADS {
		fmt.Println("Error: Max allowed threads is ", MAX_THREADS)
	}

	g := goutil.ReadGraph(x1, y1, x2, y2, inputFilename)
	if g == nil {
		fmt.Println("ReadGraph failed")
		return
	}

	source := x1*g.Dim + y1
	target := x2*g.Dim + y2
	data := &threadInfo{
		graph:             g,
		pq:                make(goutil.PriorityQueue, 0),
		openSet:           make(map[int]struct{}),
		cameFrom:          make(map[int]int),
		gScore:            make(map[int]int),
		path:              make([]int, 0),
		incrCount:         make(chan bool),
		decrCount:         make(chan bool),
		setCost:           make(chan int),
		checkDone:         make(chan bool),
		respondDone:       make(chan bool),
		checkCost:         make(chan bool),
		respondCost:       make(chan int),
		checkWait:         make(chan int),
		respondWait:       make(chan *goutil.NodeInfo),
		checkReconstruct:  make(chan *goutil.NodeInfo),
		respondReconstuct: make(chan bool),
		checkScore:        make(chan currNeighbor),
		respondScore:      make(chan int),
		checkCameFrom:     make(chan currNeighbor),
		respondCameFrom:   make(chan bool),
		checkPush:         make(chan currNeighbor),
		respondPush:       make(chan bool),
		killChan:          make(chan bool),
	}
	heap.Init(&data.pq)

	// initialize open set
	firstNode := &goutil.NodeInfo{
		Node: source,
		Cost: h(source, target, g),
	}
	fmt.Println("first node cost = ", firstNode.Cost)
	heap.Push(&data.pq, firstNode)
	data.openSet[source] = struct{}{}

	// gScore represents the cost of the cheapest path from start to current node
	data.gScore[source] = 0
	for i := range data.graph.Grid {
		if i != source {
			data.gScore[i] = math.MaxInt64
		}
	}

	initTime := time.Since(startTime)
	fmt.Println("Initialization Time: ", initTime)

	midTime := time.Now()

	done := make(chan bool)
	go checkTermination(data, done, numThreads)
	go getCost(data)
	go getWaiting(data)
	go reconstruct(data)
	go score(data)

	for i := 0; i < numThreads; i++ {
		go astar(source, target, data)
	}
	<-done
	close(data.killChan)

	computeTime := time.Since(midTime)
	fmt.Println("Computation Time: ", computeTime)

	goutil.WriteOutput(inputFilename, data.path, data.graph)
}
