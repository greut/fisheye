# fisheye

Parallelization of a Fish-Eye filter program.

## History

The order the stuff were built, and what has been added over time.

 * **Serial** are simple no parallelization programs;
 * **[OpenMP](http://openmp.org/)** are parallelized programs using shared memory;
 * **[DPS](http://dps.epfl.ch/)** are parallelized programs using message passing.

### Serial 0

Naive version.

### Serial 1

Built from Serial 0.

Precalculate the mask for the whole image.

### Serial 2

Built from Serial 1.

Precalculate the mask for the fisheye zone only

### Serial 3

Built from Serial 2.

Works in the source memory area and removes the need for having a destination
image.

### Serial 4

Built from Serial 3.

Stores only a mask that is a quarter of the full lense area.

### Serial 5

Built from Serial 4.

It doesn't work in-place anymore and the file is read using `mmap` which cuts
off the loading time of the bitmap entirely.

### Serial 6

Built from Serial 5.

Copy the input file to the output file destination using `sendfile` and then
works in place using `mmap`. No save operations are required.

### OpenMP 0

Built from Serial 4.

Reading the file and building the mask are done in parallel.

**Problem:** the memory bandwidth limitation makes the reading way longer than
in serial mode.

### OpenMP 1

Built from Serial 5.

Using the parallel work for creating the mask and doing the computation. `schedule`
is set to `dynamic`.

### MPI 0

Built from Serial 4.

Parallelize the mask computation and the file reading. Only the root reads the
file and the mask computation is done by the others, each one computes one
chunk of it (nothing fancy here).

### MPI 1

Built from Serial 5.

Parallelize the image computation. The mask is calculated independently by each
node as the operation is relatively cheap compared to any requied communication.
The image is slit in 4 parts (8 being slightly harder).

### MPI 2

From scratch (Serial 0).

Cut everything into horizontal chunks, each node computes one and the node 0
combines them all.

### MPI 3

Built from MPI 2.

Uses the fact that the picture is lens if symmetrical to have 2 chunks per
node. 0 will compute the top and bottom ones.
