#include <cstdint>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <Path.h>
#include <Utilities.h>

std::uint64_t Path::length() const
{
    return m_directions.size();
}

std::uint32_t Path::getHeight() const
{
    return m_height;
}

std::uint32_t Path::getWidth() const
{
    return m_width;
}

Path::Path() :
    m_height{1},
    m_width{1},
    m_startRow{0},
    m_startColumn{0}
{}

Path::Path(std::uint32_t height, std::uint32_t width,
    std::uint32_t startRow, std::uint32_t startColumn,
    std::vector<Direction> const &directions) :
    m_height{height},
    m_width{width},
    m_startRow{startRow},
    m_startColumn{startColumn},
    m_directions{directions}
{}

Path::Path(std::string const &fileName) :
    m_height{1},
    m_width{1},
    m_startRow{0},
    m_startColumn{0}
{
    std::int32_t heightBMP;
    std::int32_t widthBMP;
    std::ifstream file;
    if (!Utilities::readBMP(fileName, widthBMP, heightBMP, file) ||
        (heightBMP & 1) == 0 || (widthBMP & 1) == 0)
    {
        return;
    }
    std::uint32_t bmpWidthBytes = (((widthBMP - 1) >> 5) + 1) << 2;
    if (!file.ignore(bmpWidthBytes)) {
        return;
    }
    m_height = heightBMP >> 1;
    m_width = widthBMP >> 1;
    std::vector<std::vector<bool>> horizontalConnections;
    std::vector<std::vector<bool>> verticalConnections;
    horizontalConnections.resize(m_height);
    verticalConnections.resize(m_height);
    for (std::uint32_t i = 0; i < m_height; i++) {
        horizontalConnections[i] = std::vector<bool>(m_width, false);
        verticalConnections[i] = std::vector<bool>(m_width, false);
    }
    char *row1 = new char[bmpWidthBytes];
    char *row2 = new char[bmpWidthBytes];
    for (std::uint32_t i = m_height; i > 0; i--) {
        if (!file.read(row2, bmpWidthBytes) || !file.read(row1, bmpWidthBytes)) {
            delete[] row1;
            delete[] row2;
            m_height = 1;
            m_width = 1;
            return;
        }
        for (std::uint32_t j = 0; j < m_width; j++) {
            horizontalConnections[i - 1][j] =
                i != 1 && (row1[j >> 2] & (1 << (6 - ((j & 3) << 1)))) == 0;
            verticalConnections[i - 1][j] =
                j != 0 && (row2[j >> 2] & (1 << (7 - ((j & 3) << 1)))) == 0;
        }
    }
    delete[] row1;
    delete[] row2;
    bool foundStartCell = false;
    for (std::uint32_t i = 0; i < m_height; i++) {
        for (std::uint32_t j = 0; j < m_width; j++) {
            std::uint8_t degree = 0;
            if (horizontalConnections[i][j]) {
                degree++;
            }
            if (verticalConnections[i][j]) {
                degree++;
            }
            if (i != m_height - 1 && horizontalConnections[i + 1][j]) {
                degree++;
            }
            if (j != m_width - 1 && verticalConnections[i][j + 1]) {
                degree++;
            }
            if (degree >= 3) {
                m_height = 1;
                m_width = 1;
                return;
            }
            if (degree == 1 && !foundStartCell) {
                m_startRow = i;
                m_startColumn = j;
                foundStartCell = true;
            }
        }
    }
    if (!foundStartCell) {
        m_height = 1;
        m_width = 1;
        return;
    }
    std::uint32_t i = m_startRow;
    std::uint32_t j = m_startColumn;
    while (true) {
        if (i != 0 && horizontalConnections[i][j]) {
            m_directions.push_back(Up);
            horizontalConnections[i--][j] = false;
        }
        else if (i != m_height - 1 && horizontalConnections[i + 1][j]) {
            m_directions.push_back(Down);
            horizontalConnections[++i][j] = false;
        }
        else if (j != 0 && verticalConnections[i][j]) {
            m_directions.push_back(Left);
            verticalConnections[i][j--] = false;
        }
        else if (j != m_width - 1 && verticalConnections[i][j + 1]) {
            m_directions.push_back(Right);
            verticalConnections[i][++j] = false;
        }
        else {
            return;
        }
    }
}

// Let P be a path from A to B, let T ba point in P, and S(i, j) a point not contained in P.
// Direction of T is the direction T' -> T where T' is the predecessor of T in P
// (if it exists, otherwise direction of T is undefined).
// Rotation number of T is #(left turns from A to T) - #(right turns from A to T).
// Winding number of T is
// #(cells C(i + 1, j') from A to T with j' > j and with the predecessor (i, j')) -
// #(cells C(i, j') from A to T with j' > j and with the predecessor (i + 1, j')).
// Example: Path {3, 3, {Up, Left, Down}} relative to S(3, 1) has rotations
// 0, 0, 1, 2, and windings 0, 1, 1, 0.
struct PathCellParameters {
    Path::Direction direction;
    std::int64_t rotation;
    std::int64_t winding;
};

// Path cell parameters for each cell in a path, with windings
// relative to a point S(iS, jS) not contained in a path.
struct PathParameters {
    std::uint32_t height;
    std::uint32_t width;
    std::uint32_t iS;
    std::uint32_t jS;
    std::vector<std::unordered_map<std::uint32_t, PathCellParameters>> cellParameters;
    bool exists(std::uint32_t i, std::uint32_t j) const
    {
        return cellParameters[i].find(j) != cellParameters[i].end();
    }
};

// Let B be a point in a path P and A a diagonal-neighbour of B in P
// (with its position determined by booleans left and up).
// Let S be a point not contained in P. Return true if the A->B loop is either
// positive and has winding number 1, or negative and has winding number -1.
bool loopParameter(PathParameters const &path,
    std::uint32_t iB, std::uint32_t jB, bool up, bool left)
{
    std::uint32_t iA = up ? iB - 1 : iB + 1;
    std::uint32_t jA = left ? jB - 1 : jB + 1;
    std::int64_t windingInt =
        path.cellParameters[iB].at(jB).winding -
        path.cellParameters[iA].at(jA).winding;
    if (path.iS == (up ? iB : iA) && path.jS < (left ? jB : jA)) {
        windingInt += up ? 1 : -1;
    }
    std::int64_t rotationInt =
        path.cellParameters[iB].at(jB).rotation -
        path.cellParameters[iA].at(jA).rotation;
    if (rotationInt == 0) {
        Path::Direction positiveDirectionB;
        if (left && up) {
            positiveDirectionB = Path::Right;
        }
        else if (left && !up) {
            positiveDirectionB = Path::Up;
        }
        else if (!left && !up) {
            positiveDirectionB = Path::Left;
        }
        else {
            positiveDirectionB = Path::Down;
        }
        rotationInt = path.cellParameters[iB].at(jB).direction == positiveDirectionB ? 1 : -1;
    }
    return (rotationInt > 0) == (windingInt != 0);
}

// Let P be a nontrivial path, with S any point not contained in P.
// Let B be the last point of P touching the table border, if
// such a point exists. Let T be the last point of P.
// Suppose that there exists a path from T to S which has only T
// as the common point with P. Let direction be a direction such that
// if T' is the point obtained by moving T toward direction, then
// T' is a member of the table and not an element of P.
// Return true if there exists a path from T' to S having
// no common points with P, and false otherwise.
bool pathExtension(
    PathParameters const &path,
    bool touchesBorder, std::uint32_t iB, std::uint32_t jB,
    std::uint32_t iT, std::uint32_t jT, Path::Direction direction)
{
    if (path.height == 1 || path.width == 1) {
        return true;
    }
    bool upper =
        iT != 0 && path.exists(iT - 1, jT);
    bool lower =
        iT != path.height - 1 && path.exists(iT + 1, jT);
    bool left =
        jT != 0 && path.exists(iT, jT - 1);
    bool right =
        jT != path.width - 1 && path.exists(iT, jT + 1);
    bool upperLeft =
        iT != 0 && jT != 0 && path.exists(iT - 1, jT - 1);
    bool upperRight =
        iT != 0 && jT != path.width - 1 && path.exists(iT - 1, jT + 1);
    bool lowerLeft =
        iT != path.height - 1 && jT != 0 && path.exists(iT + 1, jT - 1);
    bool lowerRight =
        iT != path.height - 1 && jT != path.width - 1 && path.exists(iT + 1, jT + 1);
    Path::Direction directionT = path.cellParameters[iT].at(jT).direction;
    if (iT == 0) {
        if (jT == 0 || jT == path.width - 1) {
            return true;
        }
        switch (directionT) {
            case Path::Left: {
                if (left || lower || !lowerLeft) {
                    return true;
                }
                bool winding = loopParameter(path, iT, jT, false, true);
                return direction == (winding ? Path::Down : Path::Left);
            }
            case Path::Right: {
                if (right || lower || !lowerRight) {
                    return true;
                }
                bool winding = loopParameter(path, iT, jT, false, false);
                return direction == (winding ? Path::Right : Path::Down);
            }
            default:
                if (!touchesBorder) {
                    return true;
                }
                std::int64_t windingInt =
                    path.cellParameters[iT].at(jT).winding -
                    path.cellParameters[iB].at(jB).winding;
                if (iB < path.iS && jB > jT) {
                    windingInt++;
                }
                return direction == (windingInt == 0 ? Path::Right : Path::Left);
        }
    }
    if (iT == path.height - 1) {
        if (jT == 0 || jT == path.width - 1) {
            return true;
        }
        switch (directionT) {
            case Path::Left: {
                if (left || upper || !upperLeft) {
                    return true;
                }
                bool winding = loopParameter(path, iT, jT, true, true);
                return direction == (winding ? Path::Left : Path::Up);
            }
            case Path::Right: {
                if (right || upper || !upperRight) {
                    return true;
                }
                bool winding = loopParameter(path, iT, jT, true, false);
                return direction == (winding ? Path::Up : Path::Right);
            }
            default:
                if (!touchesBorder) {
                    return true;
                }
                std::int64_t windingInt =
                    path.cellParameters[iT].at(jT).winding -
                    path.cellParameters[iB].at(jB).winding;
                if (iB >= path.iS && jB > jT) {
                    windingInt--;
                }
                return direction == (windingInt == 0 ? Path::Right : Path::Left);
        }
    }
    if (jT == 0) {
        switch (directionT) {
            case Path::Up: {
                if (right || upper || !upperRight) {
                    return true;
                }
                bool winding = loopParameter(path, iT, jT, true, false);
                return direction == (winding ? Path::Up : Path::Right);
            }
            case Path::Down: {
                if (right || lower || !lowerRight) {
                    return true;
                }
                bool winding = loopParameter(path, iT, jT, false, false);
                return direction == (winding ? Path::Right : Path::Down);
            }
            default:
                if (!touchesBorder) {
                    return true;
                }
                std::int64_t windingInt =
                    path.cellParameters[iT].at(jT).winding -
                    path.cellParameters[iB].at(jB).winding;
                if (iB == 0 || (iB < iT && jB == 0) ||
                    (iB < path.iS && jB == path.width - 1))
                {
                    windingInt++;
                }
                return direction == (windingInt == 0 ? Path::Up : Path::Down);
        }
    }
    if (jT == path.width - 1) {
        switch (directionT) {
            case Path::Up: {
                if (left || upper || !upperLeft) {
                    return true;
                }
                bool winding = loopParameter(path, iT, jT, true, true);
                return direction == (winding ? Path::Left : Path::Up);
            }
            case Path::Down: {
                if (left || lower || !lowerLeft) {
                    return true;
                }
                bool winding = loopParameter(path, iT, jT, false, true);
                return direction == (winding ? Path::Down : Path::Left);
            }
            default:
                if (!touchesBorder) {
                    return true;
                }
                std::int64_t windingInt =
                    path.cellParameters[iT].at(jT).winding -
                    path.cellParameters[iB].at(jB).winding;
                if (iT >= path.iS) {
                    if (iB >= path.iS && iB < iT && jB == jT) {
                        windingInt--;
                    }
                    return direction == (windingInt == 0 ? Path::Up : Path::Down);
                }
                if (iB > iT && iB < path.iS && jB == jT) {
                    windingInt++;
                }
                return direction == (windingInt == 0 ? Path::Down : Path::Up);
        }
    }
    if (upperLeft) {
        if (loopParameter(path, iT, jT, true, true)) {
            if (direction == Path::Up ||
                (direction == Path::Right && directionT == Path::Up))
            {
                return false;
            }
        }
        else if (direction == Path::Left ||
            (direction == Path::Down && directionT == Path::Left))
        {
            return false;
        }
    }
    if (lowerLeft) {
        if (loopParameter(path, iT, jT, false, true)) {
            if (direction == Path::Left ||
                (direction == Path::Up && directionT == Path::Left))
            {
                return false;
            }
        }
        else if (direction == Path::Down ||
            (direction == Path::Right && directionT == Path::Down))
        {
            return false;
        }
    }
    if (lowerRight) {
        if (loopParameter(path, iT, jT, false, false)) {
            if (direction == Path::Down ||
                (direction == Path::Left && directionT == Path::Down))
            {
                return false;
            }
        }
        else if (direction == Path::Right ||
            (direction == Path::Up && directionT == Path::Right))
        {
            return false;
        }
    }
    if (upperRight) {
        if (loopParameter(path, iT, jT, true, false)) {
            if (direction == Path::Right ||
                (direction == Path::Down && directionT == Path::Right))
            {
                return false;
            }
        }
        else if (direction == Path::Up ||
            (direction == Path::Left && directionT == Path::Up))
        {
            return false;
        }
    }
    return true;
}

Path::Path(std::uint32_t height, std::uint32_t width, std::int32_t seed,
    std::uint32_t i1, std::uint32_t j1, std::uint32_t i2, std::uint32_t j2,
    std::uint16_t pastDecisionCount, std::uint16_t pastDecisionRelevance) :
    m_height{height},
    m_width{width},
    m_startRow{i1},
    m_startColumn{j1}
{
    std::uint32_t i = i1;
    std::uint32_t j = j1;
    if (i == i2 && j == j2) {
        return;
    }
    std::int64_t rotation = 0;
    std::int64_t winding = 0;
    Direction direction = Up;
    PathParameters pathParameters{height, width, i2, j2, {}};
    pathParameters.cellParameters.resize(height);
    pathParameters.cellParameters[i][j] = {direction, rotation, winding};
    bool touchesBorders = i == 0 || i == height - 1 || j == 0 || j == width - 1;
    std::uint32_t iB = i;
    std::uint32_t jB = j;
    std::vector<Direction> initialDirections;
    if (i != 0) {
        initialDirections.push_back(Up);
    }
    if (i != height - 1) {
        initialDirections.push_back(Down);
    }
    if (j != 0) {
        initialDirections.push_back(Left);
    }
    if (j != width - 1) {
        initialDirections.push_back(Right);
    }
    std::uint8_t randNumber = Utilities::randUint8(seed);
    direction = initialDirections[randNumber % initialDirections.size()];
    m_directions = {direction};
    if (direction == Up) {
        i--;
    }
    else if (direction == Down) {
        i++;
    }
    else if (direction == Left) {
        j--;
    }
    else {
        j++;
    }
    if (direction == Up && i + 1 == i2 && j > j2) {
        winding++;
    }
    else if (direction == Down && i == i2 && j > j2) {
        winding--;
    }
    pathParameters.cellParameters[i][j] = {direction, rotation, winding};
    std::unordered_map<Direction, std::uint32_t>
        directionCount{{Up, 0}, {Down, 0}, {Left, 0}, {Right, 0}};
    directionCount[direction]++;
    std::uint8_t legalDirectionsCount;
    std::vector<Direction> legalDirections;
    std::vector<std::uint32_t> legalCount;
    std::uint32_t legalCountTotal;
    legalDirections.resize(4);
    legalCount.resize(4);
    while (i != i2 || j != j2) {
        legalDirectionsCount = 0;
        legalCountTotal = 0;
        if (direction != Down && i != 0 && !pathParameters.exists(i - 1, j) &&
            pathExtension(pathParameters, touchesBorders, iB, jB, i, j, Up))
        {
            legalDirections[legalDirectionsCount] = Up;
            legalCountTotal += directionCount[Up];
            legalCount[legalDirectionsCount++] = legalCountTotal;
        }
        if (direction != Up && i != height - 1 && !pathParameters.exists(i + 1, j) &&
            pathExtension(pathParameters, touchesBorders, iB, jB, i, j, Down))
        {
            legalDirections[legalDirectionsCount] = Down;
            legalCountTotal += directionCount[Down];
            legalCount[legalDirectionsCount++] = legalCountTotal;
        }
        if (direction != Right && j != 0 && !pathParameters.exists(i, j - 1) &&
            pathExtension(pathParameters, touchesBorders, iB, jB, i, j, Left))
        {
            legalDirections[legalDirectionsCount] = Left;
            legalCountTotal += directionCount[Left];
            legalCount[legalDirectionsCount++] = legalCountTotal;
        }
        if (direction != Left && j != width - 1 && !pathParameters.exists(i, j + 1) &&
            pathExtension(pathParameters, touchesBorders, iB, jB, i, j, Right))
        {
            legalDirections[legalDirectionsCount] = Right;
            legalCountTotal += directionCount[Right];
            legalCount[legalDirectionsCount++] = legalCountTotal;
        }
        std::uint8_t randNumber = Utilities::randUint8(seed);
        Direction newDirection;
        if (randNumber < ((legalCountTotal * pastDecisionRelevance) >> 8)) {
            std::uint32_t randNumber2 = Utilities::randUint32(seed) % legalCountTotal;
            for (std::uint8_t k = 0; k < legalDirectionsCount; k++) {
                if (randNumber2 < legalCount[k]) {
                    newDirection = legalDirections[k];
                    break;
                }
            }
        }
        else {
            newDirection = legalDirections[randNumber % legalDirectionsCount];
        }
        if (i == 0 || i == height - 1 || j == 0 || j == width - 1) {
            touchesBorders = true;
            iB = i;
            jB = j;
        }
        if (newDirection == Up) {
            i--;
            if (direction == Right) {
                rotation++;
            }
            else if (direction == Left) {
                rotation--;
            }
            if (i + 1 == i2 && j > j2) {
                winding++;
            }
        }
        else if (newDirection == Down) {
            i++;
            if (direction == Left) {
                rotation++;
            }
            else if (direction == Right) {
                rotation--;
            }
            if (i == i2 && j > j2) {
                winding--;
            }
        }
        else if (newDirection == Left) {
            j--;
            if (direction == Up) {
                rotation++;
            }
            else if (direction == Down) {
                rotation--;
            }
        }
        else {
            j++;
            if (direction == Down) {
                rotation++;
            }
            else if (direction == Up) {
                rotation--;
            }
        }
        direction = newDirection;
        m_directions.push_back(direction);
        pathParameters.cellParameters[i][j] = {direction, rotation, winding};
        directionCount[direction]++;
        if (m_directions.size() > pastDecisionCount) {
            directionCount[m_directions[m_directions.size() - 1 - pastDecisionCount]]--;
        }
    }
}

std::vector<std::vector<PathCell>> Path::cells() const
{
    std::vector<std::vector<PathCell>> result;
    result.resize(m_height);
    std::uint32_t i = m_startRow;
    std::uint32_t j = m_startColumn;
    result[i].push_back({j, false, false});
    for (Direction direction : m_directions) {
        switch(direction) {
            case Up:
                result[i--].back().above = true;
                result[i].push_back({j, false, false});
                break;
            case Down:
                result[++i].push_back({j, true, false});
                break;
            case Left:
                result[i].back().left = true;
                result[i].push_back({--j, false, false});
                break;
            case Right:
                result[i].push_back({++j, false, true});
        }
    }
    for (auto &rowCells : result) {
        std::sort(rowCells.begin(), rowCells.end(),
            [](PathCell const &cell1, PathCell const &cell2)
            { return cell1.column < cell2.column; });
    }
    return result;
}

std::string Path::toString() const
{
    std::vector<std::vector<PathCell>> pathCells = cells();
    std::string result;
    for (std::vector<PathCell> &rowCells : pathCells) {
        std::string rowString1((m_width << 1) + 1, ' ');
        std::string rowString2 = rowString1;
        for (PathCell &cell : rowCells) {
            if (cell.above) {
                rowString1[(cell.column << 1) + 1] = char(219);
            }
            if (cell.left) {
                rowString2[cell.column << 1] = char(219);
            }
            rowString2[(cell.column << 1) + 1] = char(219);
        }
        result += rowString1 + '\n';
        result += rowString2 + '\n';
    }
    result += std::string((m_width << 1) - 1, ' ') + char(219) + ' ';
    result[1] = char(219);
    return result;
}

bool Path::toBMP(std::string const &fileName) const
{
    std::int32_t bmpHeight = (m_height << 1) + 1;
    std::int32_t bmpWidth = (m_width << 1) + 1;
    std::uint32_t bmpWidthBytes = (((bmpWidth - 1) >> 5) + 1) << 2;
    std::ofstream file;
    if (!Utilities::writeBMP(fileName, bmpWidth, bmpHeight, file)) {
        return false;
    }
    char *row1 = new char[bmpWidthBytes];
    char *row2 = new char[bmpWidthBytes];
    std::vector<std::vector<PathCell>> pathCells = cells();
    std::vector<char> whiteLine;
    whiteLine.resize(bmpWidthBytes);
    for (char &ch : whiteLine) {
        ch = 0;
    }
    for (std::uint32_t j = 0; j < bmpWidth; j++) {
        whiteLine[j >> 3] |= 1 << (7 - (j & 7));
    }
    for (std::uint32_t j = 0; j < bmpWidthBytes; j++) {
        row1[j] = whiteLine[j];
    }
    if (!file.write(row1, bmpWidthBytes)) {
        file.close();
        delete[] row1;
        delete[] row2;
        return false;
    }
    for (std::uint32_t i = m_height; i > 0; i--) {
        for (std::uint32_t j = 0; j < bmpWidthBytes; j++) {
            row1[j] = whiteLine[j];
            row2[j] = whiteLine[j];
        }
        for (PathCell &cell : pathCells[i - 1]) {
            if (cell.above) {
                row1[cell.column >> 2] ^= 1 << (6 - ((cell.column & 3) << 1));
            }
            if (cell.left) {
                row2[cell.column >> 2] ^= 1 << (7 - ((cell.column & 3) << 1));
            }
            row2[cell.column >> 2] ^= 1 << (6 - ((cell.column & 3) << 1));
        }
        if (!file.write(row2, bmpWidthBytes) || !file.write(row1, bmpWidthBytes)) {
            file.close();
            delete[] row1;
            delete[] row2;
            return false;
        }
    }
    file.close();
    delete[] row1;
    delete[] row2;
    return true;
}

std::int64_t Path::integral() const
{
    std::int64_t result = 0;
    std::uint32_t i = m_startRow;
    for (Direction direction : m_directions) {
        switch (direction) {
            case Up:
                i--;
                break;
            case Down:
                i++;
                break;
            case Left:
                result -= i;
                break;
            default:
                result += i;
        }
    }
    return result;
}