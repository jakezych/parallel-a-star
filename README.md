# Parallelization of Bi-Directional A* Traversal #

## By: William Foy (wfoy) and Jacob Zych (jzych) ##
 
### Summary: ###

We will implement parallel versions of bi-directional A* search using two different variants of the algorithm and compare it to the sequential C++ implementation. Each variant of the algorithm will be parallelized using both openMP and openMPI. We will generate performance metrics and graphs to determine which framework is best optimized for each variant. 

### Background: ###

A* search is a graph traversal algorithm used to find the shortest path (using some heuristic function) between a source and target vertex. A* minimizes the path funtion `f(n)=g(n)+h(n)` where `g(n)` is the cost from the start node to node `n` and `h(n)` is the heuristic function estimating the cost from node `n` to the target node. This algorithm is typically implemented sequentially using priority queues and an algorithm similar to BFS. 

![Alt Text](https://upload.wikimedia.org/wikipedia/commons/9/98/AstarExampleEn.gif)

We seek to implement the bi-directional version in parallel which starts two searches at once: one from the source to the target and one from the target to the source. There are multiple variants of this bi-directional approach to A*, namely the front-to-front and the front-to-back variants. 

The front-to-back variant, the simpler of the two, runs each search simultaneously calculating the path cost the same way that the original A* algorithm does. Each search continues until an intersecting node is found at which the algorithm terminates with the best path it found.


The front-to-front variant combines the two searches together when considering the path cost for any node `n`. The path cost function to minimize is now written as `f(n) = g(start,x) + h(x,y) + g(y,goal)`. Essentially while searching from both the target and source vertex, one considers the path cost from the current node of the search to all of the nodes in the frontier set (or nodes to be chosen from on the next iteration) of the opposing search. This approach is very computationally intensive because the heuristic function must be computed once per node in the opposing search's frontier for each node the search visits. Thus, it is important that a heuristic function is chosen that can be parallelized effectively. 


Ideally, the heuristic function chosen is fast and scalable. To make use of less precise heuristics, our implementation of A* will most likely be designed to work on grid based graphs. We will start by using Manhatten distance as the heuristic which computes the absolute distance between two nodes on the x and y axis and sums them. Since there is a tradeoff between speed and accuracy, we will test other heuristics to find the one that yields the best performance with the smallest impact on the optimality of the solution generated. 

Below is some high level pseudocode of the A* search algorithm:
```
def search(G, source, target):
    # initialize data structures 
    while frontier is not empty: 
        # get the current node 
        if current node equals the target:
            return the best path 
        for each neighbor of the current node:
            # calculate the path cost f(n) between the current node and the neighbor 
        # add the node yielding the best path cost to the frontier 
```

Implementing this algorithm bidirectionally requires starting two simultaneous searches at the same time and modifying how the searches themselves are run depending on what type of variant is being used. For the front-to-back variant this search can be further optimized by stopping when both searches are considering the same node. The front-to-front variant requires modifying the path cost calculation to compare to all of the neighbors being considered in the other search. 

Our final report will consist of performance graphs for each of the variants and parallel frameworks, also considering how the type of graph (grid-based with no obstacles, grid-based with obstacles) impacts the accuracy and perfomance of the implementation. We will analyze which frameworks and variants are best depending on the type of application and why this might be the case. 

If time permits, we will also attempt to implement A* using the domain-specific language GraphLab and compare its performance to our other implementations. 


### Challenges: ###

* Finding dimensions of the algorithms to parallelize that work with openMP and openMPI
    * Parallelizing the heuristic function
    * Parallelizing the path cost calculations for all nodes in the frontier
    * Parallelizing the search to simultaneously run in two directions
* Find the best way to communicate between processors efficiently 
    * The front-to-front variant requires considering the frontier set of the other search
* Load balancing for scale
    * The simplest approach may be to assign 1 processors to each search, but this fails to scale with increasing amounts of processing power
    * A possibly faster, but more complex, approach could be to parallelize across nodes or edges 
* Optimizing the heuristic function to be both efficient and scalable
  * The front-to-front variant is very computationally expensive and the frontier set can grow exponentially in some cases 

### Resources: ### 
We plan on referencing some existing C++ implementations of the A* algorithms to help us familiarize ourselves with the sequential implementation. 

We will use both OpenMP and OpenMPI to parallelize the algorithm. We may also use GraphLab at the end as a stretch goal if time allows.

Our hardware for testing will consist of:
* GHC Machines (Intel(R) Core(TM) i7-9700 CPU @ 3.00GHz, 8 cores)
* PSC Machines - Regular Memory (2 AMD EPYC 7742 CPUs @ 2.25-3.40 GHz, 64 cores)

### Goals and Deliverables: ###
* *75%*
    * Working parallel implementation of A* using one of the algorithms
    * Comparison of performance using different parallel frameworks (openMP and openMPI) and different hardware
* *100%* 
    * Working parallel implementation of A* using both front-to-front and front-to-back algorithms in both OpenMP and OpenMPI 
    * Running the implementations on GHC and PSC machines
    * Graphs and Metrics comparing the implementations, parallel framework, fine tuning, graph size, and hardware 
* *125%*
    * Working parallel implementation of A* using both front-to-front and front-to-back algorithms in both OpenMP and OpenMPI 
    * Running the implementations on GHC and PSC machines
    * Graphs and Metrics comparing the implementations, parallel framework, fine tuning, graph size, and hardware 
    * Working GraphLab implementation
    * Animation showing graph algorithms in action

### Schedule: ### 
 
| Week                 | Goal    | 
| ---------            | --------| 
| Week 3 *(4/10-4/17)* | Write parallel implementations for front-to-front and front-to-back using OpenMP |
| Week 4 *(4/17-4/24)* | Write parallel implementations for front-to-front and front-to-back using OpenMPI|
| Week 5 *(4/24-4/29)* | Gather metrics, make graphs, attempt GraphLab or animation|


### References:  ###
 * http://theory.stanford.edu/~amitp/GameProgramming/Variations.html
 * https://www.geeksforgeeks.org/bidirectional-search/
 * https://en.wikipedia.org/wiki/A*_search_algorithm

### MILESTONE:  ###
So far we have attempted to get a sequential version working of our original proposal with delaunay triangulation but with no success. The algorithm is too complex and we haven't had time to understand it and write our own implementation, and we also struggled to come up with tangible ways to parallelize the reference code we found. Because of these struggles, we chose to come up with a new idea that seemed more feasible.
Our new project focus is on parallelizing different implementations of the A* graph traversal algorithm using different frameworks and hardware and comparing the results. We think that this is an interesting problem space which can be completed in the remaining time left. We can also learn to use GraphLab in the later weeks if time allows. So far we have worked on formulating this new idea and getting sequential implementations up and running. We are still in the process of setting our build environment up and writing the sequential implementations for front-to-front and front-to-back.
The deliverables and new schedule seem much more doable than those in our previous proposal. We underestimated how difficult the sequential Delaumnay triangulation would be, but A* seems easier and the focus can be on parallelization.
At the poster session we plan to show graphs comparing the speedups for different configurations (implementation, framework, hardware). We might also have an animation showing how the graph traversal works if possible. We don't have any preliminary results at this time.
Currently the biggest concern is sitting down and doing the work. Our plan seems solid and the plan seems very feasible and interesting. So far both of us have not been able to dedicate the time we intended to this project due to other classes and commitments, but going forward we will each have more time to focus on this project.
