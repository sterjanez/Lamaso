# Lamaso
Maze generator and solver
## How to compile
1. Clone/download repository.
2. Either run build.ps1 with PowerShell (Windows environment), or build and install files with CMake.
## How to use the application
### Command 1 (New maze)
A maze is a rectangular table containing cells which may be or may not be separated by walls. A tree maze is a maze in which walls are placed such that each two cells are connected with exactly one path (the maze has no loops and no isolated/unreachable regions). Below is an example of a 10 x 15 tree maze:

![](Maze10x15.png width=310)

The following data has to be entered to construct a maze:
- *Seed:* Can be any 32-bit signed integer.
- *Path file:* Any 1-bit bmp file containing a path (such as a file produced by using Command 2). In the resulting maze, cells of the path will be connected.
- *Height, width:* If path file is provided, maze dimensions are obtained from the path file. Otherwise, maze height and width (32-bit unsigned integers) have to be entered manually.
- *Create tree maze:* Enter "y" if each two cells in the resulting maze are going to be connected by exactly one path (preferred). If no such condition is required, enter "n" (not preferred).
- *Probability set:* If tree maze is selected, the maze is constructed according to 6 probability values, which can be either defined by default (select "y" when asked if "apply default probability set") or entered manually (select "n", then enter the values, which are 8-bit unsigned integers).
- *Maze file:* Enter the output BMP file, or leave blank if no saving.
### Command 2 (New path)
A path in a rectangular m x n table is a connection (path) between any two cells (points) in the table. Cells in the table are represented by pairs (i, j), where i is the row index (may be 0 to $m - 1$) and j is the column index (0 to n - 1).
- *Height, width:* table dimensions (32-bit unsigned integers). Table entries are presented by pairs (i, j) where i is the row index ($0$ to $height - 1$), and j is the column index ($0$ to $width - 1$).
- *Start row, start column, end row, end column:* Starting and ending point of the path.
- *Seed:* Can be any 32-bit signed integer.
- *Persistency chain length:* A path is generated by a random walk algorithm. If persistency chain length (unsigned 16-bit integer) is nonzero (say, equal to k), the algorithm memorizes past $k$ decisions in the random walk and, with a certain probability, chooses the new direction with a
