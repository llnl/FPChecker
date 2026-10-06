# miniAMR - Adaptive Mesh Refinement Mini-App

miniAMR applies a stencil calculation on a unit cube computational domain, which is divided into blocks. The blocks all have the same number of cells in each direction and communicate ghost values with neighboring blocks. With adaptive mesh refinement, the blocks can represent different levels of refinement in the larger mesh. Neighboring blocks can be at the same level or one level different, which means that the length of cells in neighboring blocks can differ by only a factor of two in each direction. The calculations on the variables in each cell is an averaging of the values in the chosen stencil. The refinement and coarsening of the blocks is driven by objects that are pushed through the mesh. If a block intersects with the surface or the volume of an object, then that block can be refined. There is also an option to uniformly refine the mesh. Each cell contains a number of variables, each of which is evaluated indepently.

Questions? Contact Courtenay Vaughan (ctvaugh@sandia.gov)

