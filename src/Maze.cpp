#include <cstdint>
#include <vector>
#include <utility>
#include <string>
#include <fstream>
#include <Utilities.h>
#include <Path.h>
#include <Maze.h>

std::uint32_t Maze::getHeight() const
{
    return m_height;
}

std::uint32_t Maze::getWidth() const
{
    return m_width;
}

Maze::Maze(std::string const &fileName)
{
    m_height = 1;
    m_width = 1;
    m_horizontalWalls = {{false}};
    m_verticalWalls = {{false}};
    std::int32_t widthBMP;
    std::int32_t heightBMP;
    std::ifstream file;
    if (!Utilities::readBMP(fileName, widthBMP, heightBMP, file)) {
        return;
    }
    std::uint32_t bmpWidthBytes = (((widthBMP - 1) >> 5) + 1) << 2;    
    if ((heightBMP & 1) == 0 || (widthBMP & 1) == 0 || !file.ignore(bmpWidthBytes)) {
        return;
    }
    m_height = heightBMP >> 1;
    m_width = widthBMP >> 1;
    m_horizontalWalls.resize(m_height);
    m_verticalWalls.resize(m_height);
    for (std::uint32_t i = 0; i < m_height; i++) {
        m_horizontalWalls[i] = std::vector<bool>(m_width, false);
        m_verticalWalls[i] = std::vector<bool>(m_width, false);
    }
    char *row1 = new char[bmpWidthBytes];
    char *row2 = new char[bmpWidthBytes];
    for (std::uint32_t i = m_height; i > 0; i--) {
        if (!file.read(row2, bmpWidthBytes) || !file.read(row1, bmpWidthBytes)) {
            delete[] row1;
            delete[] row2;
            m_height = 1;
            m_width = 1;
            m_horizontalWalls = {{false}};
            m_verticalWalls = {{false}};
            return;
        }
        for (std::uint32_t j = 0; j < m_width; j++) {
            m_horizontalWalls[i - 1][j] =
                (row1[j >> 2] & (1 << (6 - ((j & 3) << 1)))) == 0 && i != 1;
            m_verticalWalls[i - 1][j] =
                (row2[j >> 2] & (1 << (7 - ((j & 3) << 1)))) == 0 && j != 0;
        }
    }
    delete[] row1;
    delete[] row2;
}

Maze::Maze(Path const &path, std::int32_t seed, std::uint16_t density)
{
    std::vector<std::vector<PathCell>> solutionCells = path.cells();
    m_height = path.getHeight();
    m_width = path.getWidth();
    m_verticalWalls.resize(m_height);
    m_horizontalWalls.resize(m_height);
    for (std::uint32_t i = 0; i < m_height; i++) {
        m_verticalWalls[i] = std::vector<bool>(m_width, false);
        m_horizontalWalls[i] = std::vector<bool>(m_width, false);
    }
    for (std::uint32_t i = 0; i < m_height; i++) {
        std::uint32_t nextSolutionCellIndex = 0;
        std::uint32_t nextSolutionCellColumn = solutionCells[i].empty() ?
            m_width : solutionCells[i][0].column;
        for (std::uint32_t j = 0; j < m_width; j++) {
            if (j == nextSolutionCellColumn) {
                m_horizontalWalls[i][j] =
                    i != 0 && !solutionCells[i][nextSolutionCellIndex].above &&
                    Utilities::randUint8(seed) < density;
                m_verticalWalls[i][j] =
                    j != 0 && !solutionCells[i][nextSolutionCellIndex].left &&
                    Utilities::randUint8(seed) < density;
                nextSolutionCellIndex++;
                nextSolutionCellColumn = nextSolutionCellIndex == solutionCells[i].size() ?
                    m_width : solutionCells[i][nextSolutionCellIndex].column;
            }
            else {
                m_horizontalWalls[i][j] = i != 0 && Utilities::randUint8(seed) < density;
                m_verticalWalls[i][j] = j != 0 && Utilities::randUint8(seed) < density;
            }
        }
    }
}

class Pool {

    std::uint32_t m_size;
    std::uint32_t m_popIndex;
    std::uint32_t m_pushIndex;
    std::vector<std::uint32_t> m_values;
    std::vector<std::uint32_t> m_indices;

public:

    std::vector<std::uint32_t> degrees;

    Pool(std::uint32_t size) :
        m_size{size},
        m_popIndex{0},
        m_pushIndex{0}
    {
        m_values.resize(size);
        for (std::uint32_t i = 0; i < size; i++) {
            m_values[i] = i;
        }
        m_indices.resize(size);
        degrees.resize(size);
    }

    std::uint32_t get(std::uint32_t value) const
    {
        while (value != m_indices[value]) {
            value = m_indices[value];
        }
        return value;
    }

    void join(std::uint32_t &value1, std::uint32_t &value2)
    {
        if (degrees[value1] < degrees[value2]) {
            std::swap(value1, value2);
        }
        m_values[m_pushIndex++] = value2;
        if (m_pushIndex == m_size) {
            m_pushIndex = 0;
        }
        m_indices[value2] = value1;
        degrees[value1] += degrees[value2];
    }

    std::uint32_t pop(std::uint32_t degree)
    {
        std::uint32_t value = m_values[m_popIndex++];
        if (m_popIndex == m_size) {
            m_popIndex = 0;
        }
        m_indices[value] = value;
        degrees[value] = degree;
        return value;
    }

};

Maze::Maze(Path const &path, std::int32_t seed,
    std::vector<std::uint8_t> const &probabilitySet)
{
    m_height = path.getHeight();
    m_width = path.getWidth();
    m_verticalWalls.resize(m_height);
    m_horizontalWalls.resize(m_height);
    for (std::uint32_t i = 0; i < m_height; i++) {
        m_verticalWalls[i] = std::vector<bool>(m_width, false);
        m_horizontalWalls[i] = std::vector<bool>(m_width, false);
    }
    std::vector<std::vector<PathCell>> solutionCells = path.cells();
    std::uint32_t poolSize = m_width + 2;
    Pool pool(poolSize);
    std::vector<std::uint32_t> cellIndices;
    cellIndices.resize(m_width);

    std::uint8_t probability1 = probabilitySet[0];
    std::uint8_t probability2 = probabilitySet[1];
    std::uint8_t probability3 = probabilitySet[2];
    std::uint8_t probability4 = probabilitySet[3];
    std::uint8_t probability5 = probabilitySet[4];
    std::uint8_t probability6 = probabilitySet[5];
    std::uint8_t probability12 =
        static_cast<std::uint16_t>(probability1) * static_cast<std::uint16_t>(probability2) / 256;
    std::uint8_t probability34 = probability3 -
        static_cast<std::uint16_t>(probability3) * static_cast<std::uint16_t>(probability4) / 256 +
        probability4;
    std::uint8_t probability35 =
        static_cast<std::uint16_t>(probability3) * static_cast<std::uint16_t>(probability5) / 256;

    std::uint32_t solutionIndex = pool.pop(0);
    std::uint32_t borderIndex = pool.pop(0);
    for (std::uint32_t j = 0; j < m_width; j++) {
        cellIndices[j] = pool.pop(1);
    }

    for (std::uint32_t i = 0; i < m_height; i++) {
        std::uint32_t nextSolutionCellIndex = 0;
        std::uint32_t nextSolutionCellColumn = solutionCells[i].empty() ?
            m_width : solutionCells[i][0].column;
        borderIndex = pool.get(borderIndex);
        if (pool.degrees[borderIndex] != 0) {
            borderIndex = pool.pop(0);
        }
        std::uint32_t index = borderIndex;
        for (std::uint32_t j = 0; j < m_width; j++) {
            std::uint32_t aboveIndex = pool.get(cellIndices[j]);
            std::uint8_t randValue = Utilities::randUint8(seed);
            if (j == nextSolutionCellColumn) {
                solutionIndex = pool.get(solutionIndex);
                if (index == solutionIndex) {
                    if (!solutionCells[i][nextSolutionCellIndex].left) {
                        m_verticalWalls[i][j] = true;
                    }
                    if (aboveIndex == solutionIndex) {
                        if (!solutionCells[i][nextSolutionCellIndex].above) {
                            m_horizontalWalls[i][j] = true;
                        }
                    }
                    else if (pool.degrees[aboveIndex] == 1 ||
                        randValue < probability35 || randValue >= probability34)
                    {
                        pool.join(index, aboveIndex);
                    }
                    else {
                        m_horizontalWalls[i][j] = true;
                        pool.degrees[aboveIndex]--;
                        pool.degrees[index]++;
                    }
                }
                else if (aboveIndex == solutionIndex) {
                    if (!solutionCells[i][nextSolutionCellIndex].above) {
                        m_horizontalWalls[i][j] = true;
                    }
                    if ((randValue >= probability35 && randValue < probability3) ||
                        randValue >= probability34)
                    {
                        pool.join(index, solutionIndex);
                    }
                    else {
                        m_verticalWalls[i][j] = true;
                        index = solutionIndex;
                    }
                }
                else if (aboveIndex == index) {
                    if (randValue < probability12) {
                        m_horizontalWalls[i][j] = true;
                        m_verticalWalls[i][j] = true;
                        pool.degrees[index]--;
                        pool.degrees[solutionIndex]++;
                        index = solutionIndex;
                    }
                    else {
                        if (randValue < probability1) {
                            m_horizontalWalls[i][j] = true;
                        }
                        else {
                            m_verticalWalls[i][j] = true;
                        }
                        pool.join(index,solutionIndex);
                    }
                }
                else if (pool.degrees[aboveIndex] == 1) {
                    pool.join(solutionIndex,aboveIndex);
                    if (randValue < probability6) {
                        m_verticalWalls[i][j] = true;
                        index = solutionIndex;
                    }
                    else {
                        pool.join(index, solutionIndex);
                    }
                }
                else if (randValue < probability35) {
                    m_verticalWalls[i][j] = true;
                    pool.join(solutionIndex, aboveIndex);
                    index = solutionIndex;
                }
                else if (randValue < probability3) {
                    m_horizontalWalls[i][j] = true;
                    pool.degrees[aboveIndex]--;
                    pool.degrees[index]++;
                    pool.join(index, solutionIndex);
                }
                else if (randValue < probability34) {
                    m_verticalWalls[i][j] = true;
                    m_horizontalWalls[i][j] = true;
                    pool.degrees[aboveIndex]--;
                    pool.degrees[solutionIndex]++;
                    index = solutionIndex;
                }
                else {
                    pool.join(index, solutionIndex);
                    pool.join(index, aboveIndex);
                }
                nextSolutionCellIndex++;
                nextSolutionCellColumn =
                    nextSolutionCellIndex == solutionCells[i].size() ?
                    m_width : solutionCells[i][nextSolutionCellIndex].column;
            }
            else if (index == aboveIndex) {
                if (randValue < probability12) {
                    m_horizontalWalls[i][j] = true;
                    m_verticalWalls[i][j] = true;
                    pool.degrees[aboveIndex]--;
                    index = pool.pop(1);
                }
                else if (randValue < probability1) {
                    m_horizontalWalls[i][j] = true;
                }
                else {
                    m_verticalWalls[i][j] = true;
                }
            }
            else if (pool.degrees[aboveIndex] == 1) {
                if (randValue < probability6) {
                    m_verticalWalls[i][j] = true;
                    index = aboveIndex;
                }
                else {
                    pool.join(index, aboveIndex);
                }
            }
            else if (randValue < probability35) {
                m_verticalWalls[i][j] = true;
                index = aboveIndex;
            }
            else if (randValue < probability3) {
                m_horizontalWalls[i][j] = true;
                pool.degrees[aboveIndex]--;
                pool.degrees[index]++;
            }
            else if (randValue < probability34) {
                m_verticalWalls[i][j] = true;
                m_horizontalWalls[i][j] = true;
                pool.degrees[aboveIndex]--;
                index = pool.pop(1);
            }
            else {
                pool.join(index, aboveIndex);
            }
            cellIndices[j] = index;
        }
    }
    std::uint32_t index = pool.get(borderIndex);
    for (std::uint32_t j = 0; j < m_width; j++) {
        std::uint32_t aboveIndex = pool.get(cellIndices[j]);
        if (index != aboveIndex) {
            std::uint8_t randValue = Utilities::randUint8(seed);
            if ((pool.degrees[index] + 1) * randValue < 256) {
                m_verticalWalls[m_height - 1][j] = false;
                pool.join(index, aboveIndex);
            }
            else {
                index = aboveIndex;
            }
        }
        pool.degrees[index]--;
    }
    for (std::uint32_t i = 0; i < m_height; i++) {
        m_verticalWalls[i][0] = false;
    }
}

std::string Maze::toString() const
{
    std::string mazeString;
    for (std::uint32_t i = 0; i < m_height; i++) {
        if (i == 0) {
            mazeString += char(219);
            mazeString += ' ';
            mazeString += std::string((m_width << 1) - 1, char(219));
        }
        else {
            for (std::uint32_t j = 0; j < m_width; j++) {
                mazeString += char(219);
                mazeString += m_horizontalWalls[i][j] ? char(219) : ' ';
            }
            mazeString += char(219);
        }
        mazeString += '\n';
        for (std::uint32_t j = 0; j < m_width; j++) {
            if (j == 0) {
                mazeString += char(219);
            }
            else {
                mazeString += m_verticalWalls[i][j] ? char(219) : ' ';
            }
            mazeString += ' ';
        }
        mazeString += char(219);
        mazeString += '\n';
    }
    mazeString += std::string((m_width << 1) - 1, char(219));
    mazeString += ' ';
    mazeString += char(219);
    return mazeString;
}

bool Maze::toBMP(std::string const &fileName) const
{
    std::int32_t bmpHeight = (m_height << 1) + 1;
    std::int32_t bmpWidth = (m_width << 1) + 1;
    std::ofstream file;
    if (!Utilities::writeBMP(fileName, bmpWidth, bmpHeight, file)) {
        return false;
    }
    std::uint32_t bmpWidthBytes = (((bmpWidth - 1) >> 5) + 1) << 2;
    char *row1 = new char[bmpWidthBytes];
    char *row2 = new char[bmpWidthBytes];
    for (std::uint32_t j = 0; j < bmpWidthBytes; j++) {
        row1[j] = 0;
    }
    row1[(bmpWidth - 2) >> 3] |= 1 << (7 - ((bmpWidth - 2) & 7));
    if (!file.write(row1, bmpWidthBytes)) {
        delete[] row1;
        delete[] row2;
        return false;
    }
    for (std::uint32_t i = m_height - 1; i > 0; i--) {
        for (std::uint32_t j = 0; j < bmpWidthBytes; j++) {
            row1[j] = 0;
            row2[j] = 0;
        }
        for (std::uint32_t j = 0; j < m_width; j++) {
            if (!m_horizontalWalls[i][j]) {
                row1[j >> 2] |= 1 << (6 - ((j & 3) << 1));
            }
            if (j != 0 && !m_verticalWalls[i][j]) {
                row2[j >> 2] |= 1 << (7 - ((j & 3) << 1));
            }
            row2[j >> 2] |= 1 << (6 - ((j & 3) << 1));
        }
        if (!file.write(row2, bmpWidthBytes) || !file.write(row1, bmpWidthBytes)) {
            delete[] row1;
            delete[] row2;
            return false;
        }
    }
    for (std::uint32_t j = 0; j < bmpWidthBytes; j++) {
        row1[j] = 0;
        row2[j] = 0;
    }
    row1[0] = 64;
    for (std::uint32_t j = 0; j < m_width; j++) {
        if (j != 0 && !m_verticalWalls[0][j]) {
            row2[j >> 2] |= 1 << (7 - ((j & 3) << 1));
        }
        row2[j >> 2] |= 1 << (6 - ((j & 3) << 1));
    }
    if (!file.write(row2, bmpWidthBytes) || !file.write(row1, bmpWidthBytes)) {
        delete[] row1;
        delete[] row2;
        return false;
    }
    delete[] row1;
    delete[] row2;
    return true;
}

Path Maze::solve(std::uint32_t i1, std::uint32_t j1, std::uint32_t i2, std::uint32_t j2) const
{
    if (i1 == i2 && j1 == j2) {
        return {m_height, m_width, i1, j1, std::vector<Path::Direction>{}};
    }
    std::vector<Path::Direction> initialDirections;
    if (i1 != 0 && !m_horizontalWalls[i1][j1]) {
        initialDirections.push_back(Path::Up);
    }
    if (i1 != m_height - 1 && !m_horizontalWalls[i1 + 1][j1]) {
        initialDirections.push_back(Path::Down);
    }
    if (j1 != 0 && !m_verticalWalls[i1][j1]) {
        initialDirections.push_back(Path::Left);
    }
    if (j1 != m_width - 1 && !m_verticalWalls[i1][j1 + 1]) {
        initialDirections.push_back(Path::Right);
    }
    for (Path::Direction initialDirection : initialDirections) {
        std::uint32_t i = i1;
        std::uint32_t j = j1;
        std::vector<Path::Direction> directions{initialDirection};
        Path::Direction direction;
        switch (initialDirection) {
            case Path::Up:
                i--;
                direction = Path::Left;
                break;
            case Path::Down:
                i++;
                direction = Path::Right;
                break;
            case Path::Left:
                j--;
                direction = Path::Down;
                break;
            case Path::Right:
                j++;
                direction = Path::Up;
        }
        while ((i != i1 || j != j1) && (i != i2 || j != j2)) {
            if (direction == Path::Up) {
                if (i != 0 && !m_horizontalWalls[i][j]) {
                    if (directions.back() == Path::Down) {
                        directions.pop_back();
                    }
                    else {
                        directions.push_back(Path::Up);
                    }
                    i--;
                    direction = Path::Left;
                }
                else {
                    direction = Path::Right;
                }
            }
            else if (direction == Path::Down) {
                if (i != m_height - 1 && !m_horizontalWalls[i + 1][j]) {
                    if (directions.back() == Path::Up) {
                        directions.pop_back();
                    }
                    else {
                        directions.push_back(Path::Down);
                    }
                    i++;
                    direction = Path::Right;
                }
                else {
                    direction = Path::Left;
                }
            }
            else if (direction == Path::Left) {
                if (j != 0 && !m_verticalWalls[i][j]) {
                    if (directions.back() == Path::Right) {
                        directions.pop_back();
                    }
                    else {
                        directions.push_back(Path::Left);
                    }
                    j--;
                    direction = Path::Down;
                }
                else {
                    direction = Path::Up;
                }
            }
            else {
                if (j != m_width - 1 && !m_verticalWalls[i][j + 1]) {
                    if (directions.back() == Path::Left) {
                        directions.pop_back();
                    }
                    else {
                        directions.push_back(Path::Right);
                    }
                    j++;
                    direction = Path::Up;
                }
                else {
                    direction = Path::Down;
                }
            }
        }
        if (i == i2 && j == j2) {
            return {m_height, m_width, i1, j1, directions};
        }
    }
    return {m_height, m_width, i1, j1, std::vector<Path::Direction>{}};
}