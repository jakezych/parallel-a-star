## Parallel Voronoi Diagram Terrain Generation ##
**By: William Foy (wfoy) and Jacob Zych (jzych)**
 
### Summary: ###


We are going to parallelize the sweep hull variant of the Delaunay triangulation algorithm. Using this triangulation algorithm, we plan to generate a Voronoi diagram that will be colored using Perlin noise to create a 2D terrain map. By parallelizing the Delaunay triangulation algorithm using both OpenMPI and OpenMP, we will analyze which parallel framework results in the best speedup.


### Background: ###

Given a set of `P` points, the Delaunay triangulation algorithm is used to generate a triangulation of those points such that no point in the triangulation is within any of the circumcircles of any of the triangles. There are several different algorithms for implementing this with various efficiences. The most efficient algorithms include the recursive divide-and-conquer approach and the iterative sweephull algorithm which both run in `O(nlog(n))` time. Given a Delaunay triangulation of a set of points, the Voronoi diagram can be drawn by connecting the circumcenters of the triangles adjacent to each other. 


![](https://i.imgur.com/2Eefk2U.png)

*https://en.wikipedia.org/wiki/Delaunay_triangulation*

Voronoi diagrams are frequently used as the basis for terrain generation in video games and landscape visualization. By using Perlin noise to color the cells of the Voronoi diagram, one can generate a semi-realistic looking 2D terrain with features like biomes, islands, and elevation faster than current sequential implementations. 

We suspect that the generation of the Voronoi diagram using Delaunay triangulation is the most computationally intensive part of 2D terrain generation, so our focus will be on parallelizing this aspect. We will likely also parallelize the coloring of the terrain but this is a computationally easier and less interesting problem to parallelize, so it will be a smaller focus of the project. 

The sweephull variant of the Delaunay triangulation algorithm will be our algorithm of choice for parallelization. It is one of the fastest known iterative implementations, which is much easier to parallelize than the recursive divide-and-conquer implementation. The sweephull variant can be broken down into two major parts: the initial triangulation of the points and the flipping of adjacent triangles. The initial triangulation requires radially sorting the points as well as iteratively adding points to a convex hull. Some implementations use a linked list or stack to support this which comes with its own set of challenges in parallelizing it. Moreover, the flipping algorithm requires modifying the points of all pairs of adjacent triangles in memory which can also benefit from parallelization. 

Below is some high level pseudocode: 

```
def delaunayTriangulation(points):
    triangles = initialTriangulation(points)
    flipAdjacentTriangles(triangles)
    return triangles 

def intialTriangulation(points):
    radialSort(points)
    triangles = initializeHull(points);
    for point in points:
        addPointToConvexHull(triangles, point)
    return triangles 
```

We plan on parallelizing a sequential implementation using both OpenMP and openMPI and comparing the performance of each to the sequential implementations run-time to determine which framework is better suited for this algorithm. 


### Challenges: ###

* Adding points to the convex hull 
    * Typical implementations use data structures like linked lists or stacks. We need to consider how we can design a solution that correctly generates the triangulation and avoids data races but still optimizes for performance. 
* Flipping Triangles 
    * Adjacent pairs of triangles are selected and flipped using the flipping algorithm. We need to find the most optimal way to assign and schedule work since there are multiple axes of parallelism like the triangles or points themselves.
    * The algorithm that flips two triangles performs several read/write memory accesses to swap the points of the triangles. We need to design our solution to optimize for cache locality to reduce cache misses.
* Rendering of polygonal terrain
    * The rendering of the polygonal terrain after performing the Delaunay triangulation presents challenges with how to best represent an irregular region, like a polygon, in memory and how to assign work in a such a way that reduces communication overhead.


### Resources: ### 
We plan on referencing some existing C++ implementations of the Delaunay triangulation to help us familiarize ourselves with the sequential implemenation. The variant of the algorithm we plan on implementing does not have many resources detailing the algorithm at a pseudocode level, so we may also pull certain aspects of the sequential code we find online to create our initial sequential implementation. This will allow us to focus more on parallelization in this project. 

We will use both OpenMP and OpenMPI to parallelize the algorithm. We may also use a C++ graphics rendering or Perlin noise package to assist us in rendering the terrain. 

Our hardware for testing will consist of:
* GHC Machines (Intel(R) Core(TM) i7-9700 CPU @ 3.00GHz, 8 cores)
* PSC Machines - Regular Memory (2 AMD EPYC 7742 CPUs @ 2.25-3.40 GHz, 64 cores)

### Goals and Deliverables: ###
* *75%*
    * Working parallel implementation of Delaunay Triangulation in one of the two frameworks we plan on using
    * Rendering of the Voronoi diagram 
* *100%* 
    * Working parallel implementation of Delaunay Triangulation in both OpenMP and OpenMPI 
    * Rendering of the Voronoi diagram 
    * Perlin noise used to generate a 2D terrain map with biomes, islands, and elevation
* *125%*
    * Working parallel implementation of Delaunay Triangulation in both openMP and openMPI 
    * Rendering of the Voronoi diagram 
    * Perlin noise used to generate a 2D terrain map with biomes, islands, and elevation
    * More complex terrain generation features like trees, rivers and lakes 
    * parallel 3D rendering of the terrain to better show elevation

### Schedule: ### 
 
| Week                 | Goal     | 
| --------             | -------- | 
| Week 1 *(3/27-4/3)*  | Research and write sequential implementation of Delaunay Triangulation                    |
| Week 2 *(3/27-4/3)*  | Write Voronoi diagram generation and parallelize Delaunay Triangulation using OpenMP      |
| Week 3 *(4/10-4/17)* | Create checkpoint report and parallelize Delaunay Triangulation using OpenMPI             |
| Week 4 *(4/17-4/24)* | Write terrain generation and measure final performance                                    |
| Week 5 *(4/24-4/29)* | Summarize results in a final report and presentation                                      |


### References:  ###
* https://en.wikipedia.org/wiki/Delaunay_triangulation
* http://www.s-hull.org/paper/s_hull.pdf
* https://github.com/delfrrr/delaunator-cpp
* https://github.com/tlvb/shull
* https://www.redblobgames.com/x/2022-voronoi-maps-tutorial/
