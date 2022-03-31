#ifndef MAZE_H
#define MAZE_H

#include <cstdint>
#include <vector>
#include <string>
#include <Path.h>

// 2D maze of size m x n.
class Maze {

    // Height.
    std::uint32_t m_height;

    // Width.
    std::uint32_t m_width;

    // m_verticalWalls[i][j] = true if the (i, j)-th cell has wall left to it.
    // Cells in column j = 0 have no walls on the left.
    std::vector<std::vector<bool>> m_verticalWalls;

    // m_horizontalWalls[i][j] = true if the (i, j)-th cell has wall above it.
    // Cells in row i = 0 have no walls above it.
    std::vector<std::vector<bool>> m_horizontalWalls;

public:

    std::uint32_t getHeight() const;

    std::uint32_t getWidth() const;

    // Create maze from a BMP file. Return 1 x 1 maze if could not read file.
    Maze(std::string const &fileName);

    // Create a random maze with given predefined path inside, random seed number
    // and wall density (density = 0 is no walls, and density > 255 is all walls).
    Maze(Path const &path, std::int32_t seed, std::uint16_t density);

    // Create a random maze such that each two cells are connected by exactly one path,
    // with given random seed number, predefined path inside the maze,
    // and probability set (6 values).
    // Maximum maze width is UINT32_MAX - 2.
    // Possible values for probability set: {163, 118, 123, 123, 94, 103}
    Maze(Path const &path, std::int32_t seed,
        std::vector<std::uint8_t> const &probabilitySet = {163, 118, 123, 123, 94, 103});

    // View maze as a multi-line string.
    std::string toString() const;

    // Save as BMP file. Return false if failed.
    bool toBMP(std::string const &fileName) const;

    // Find path between given cells using "always turn left" algoritm.
    // Return empty path with initial point (i1, j1)
    // if the algorithm finds a loop before finding a solution.
    Path solve(std::uint32_t i1, std::uint32_t j1, std::uint32_t i2, std::uint32_t j2) const;

};

#endif