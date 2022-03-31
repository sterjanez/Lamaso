#ifndef PATH_H
#define PATH_H

#include <cstdint>
#include <vector>
#include <string>

// Path cell in a maze.
struct PathCell {
    std::uint32_t column; // column index of the cell
    bool above; // true if the cell is connected to the cell above it
    bool left; // true if the cell is connected to the cell left to it
};

// Path in a maze.
class Path {

public:

    enum Direction {Up, Down, Left, Right};

private:

    // Maze height.
    std::uint32_t m_height;

    // Maze width.
    std::uint32_t m_width;

    // Row index of start cell.
    std::uint32_t m_startRow;

    // Column index of start cell.
    std::uint32_t m_startColumn;

    // Path directions.
    std::vector<Direction> m_directions;

public:

    std::uint32_t getHeight() const;

    std::uint32_t getWidth() const;

    // Path length.
    std::uint64_t length() const;

    // Empty path.
    Path();

    // Path with given start cell position and directions.
    Path(std::uint32_t height, std::uint32_t width,
        std::uint32_t startRow, std::uint32_t startColumn,
        std::vector<Direction> const &directions);

    // Read path from a BMP file. Return empty path if failed.
    Path(std::string const &fileName);

    // Random path in a table of size height x width from
    // (i1, j1) to (i2, j2), with given random seed,
    // where pastDecisionCount is the number of past directions affecting
    // the choice of the new direction at each step
    // and pastDecisionRelevance is the relevance of each of the past choices.
    Path(std::uint32_t height, std::uint32_t width, std::int32_t seed,
        std::uint32_t i1, std::uint32_t j1, std::uint32_t i2, std::uint32_t j2,
        std::uint16_t pastDecisionCount, std::uint16_t pastDecisionRelevance);

    // Cells contained in the path, where i-th element of the return vector
    // contains all cells in i-th row, sorted from left to right.
    std::vector<std::vector<PathCell>> cells() const;

    // View path as a string.
    std::string toString() const;

    // Save as BMP file. Return false if failed.
    bool toBMP(std::string const &fileName) const;

    // The integral of a path is the sum of i-components of point in the path
    // having predecessor on the left minus the sum of i-components of points
    // having predecessor on the right. Geometrically, it represents the
    // signed area above the path curve.
    std::int64_t integral() const;

};

#endif