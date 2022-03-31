#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <Commands.h>
#include <Path.h>
#include <Maze.h>
#include <Utilities.h>

std::int64_t integerInput()
{
    std::string valueString;
    std::getline(std::cin, valueString);
    return std::stoll(valueString);
}

std::string stringInput()
{
    std::string inputStr;
    std::getline(std::cin, inputStr);
    return inputStr;
}

void Commands::newMaze()
{
    std::cout << "\nCreate new maze\n\n";
    std::cout << "Seed number (32-bit signed integer): ";
    std::int32_t seed = integerInput();
    std::cout << "Path file (empty if none): ";
    std::string pathFileName = stringInput();
    Path path;
    if (pathFileName.empty()) {
        std::cout << "Height: ";
        std::uint32_t height = integerInput();
        std::cout << "Width: ";
        std::uint32_t width = integerInput();
        path = Path(height, width, 0, 0, {});
    }
    else {
        std::cout << "Loading path ...";
        path = Path(pathFileName);
        std::cout << "\n";
    }
    std::cout << "Create tree maze? (y = Yes, n = No) ";
    bool tree = stringInput() == "y";
    std::uint16_t density;
    std::vector<std::uint8_t> probabilitySet = {163, 118, 123, 123, 94, 103};
    if (tree) {
        std::cout <<
            "Apply default probability set {163, 118, 123, 123, 94, 103}? (y = Yes, n = No) ";
        if (stringInput() != "y") {
            for (std::uint8_t i = 0; i < 6; i++) {
                std::cout << "Probability value " << i + 1 << " (0 - 255): ";
                probabilitySet[i] = integerInput();
            }
        }
    }
    else {
        std::cout << "Wall density (0 - 65535): ";
        density = integerInput();
    }
    std::cout << "Creating maze ...";
    auto t1 = std::chrono::high_resolution_clock::now();
    Maze maze = tree ? Maze(path, seed, probabilitySet) : Maze(path, seed, density);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << "\nFinished in " << duration.count() << " milliseconds.\n";
    std::cout << "Maze file (empty if no saving): ";
    std::string mazeFileName = stringInput();
    if (!mazeFileName.empty()) {
        std::cout << "Saving ...";
        std::cout << (maze.toBMP(mazeFileName) ? " Finished." : " Failed!") << "\n";
    }
}

void Commands::newPath()
{
    std::cout << "\nCreate new path\n";
    std::cout << "Height: ";
    std::uint32_t height = integerInput();
    std::cout << "Width: ";
    std::uint32_t width = integerInput();
    std::cout << "Start row: ";
    std::uint32_t i1 = integerInput();
    std::cout << "Start column: ";
    std::uint32_t j1 = integerInput();
    std::cout << "End row: ";
    std::uint32_t i2 = integerInput();
    std::cout << "End column: ";
    std::uint32_t j2 = integerInput();
    std::cout << "Seed number (unsigned 32-bit integer): ";
    std::int32_t seed = integerInput();
    std::cout << "Persistency chain length (0 to 65535): ";
    std::uint16_t persistencyCount = integerInput();
    std::cout << "Persistency strength (0 to 65535): ";
    std::uint16_t persistencyStrength = integerInput();
    std::cout << "Creating path ...";
    auto t1 = std::chrono::high_resolution_clock::now();
    Path path(height, width, seed, i1, j1, i2, j2,
        persistencyCount, persistencyStrength);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << "\nFinished in " << duration.count() << " milliseconds.\n";
    std::cout << "Path length: " << path.length() << "\n";
    std::cout << "Save as (empty if no saving): ";
    std::string pathFileName = stringInput();
    if (!pathFileName.empty()) {
        std::cout << "Saving ...";
        std::cout << (path.toBMP(pathFileName) ? " Finished." : " Failed!") << "\n";
    }
}

void Commands::solveMaze()
{
    std::cout << "\nSolve maze\n";
    std::cout << "Maze file name: ";
    std::string mazeFileName = stringInput();
    std::cout << "Start row: ";
    std::uint32_t i1 = integerInput();
    std::cout << "Start column: ";
    std::uint32_t j1 = integerInput();
    std::cout << "End row: ";
    std::uint32_t i2 = integerInput();
    std::cout << "End column: ";
    std::uint32_t j2 = integerInput();
    Maze maze(mazeFileName);
    if (maze.getHeight() == 1 && maze.getWidth() == 1) {
        std::cout << "Empty maze. Possible failure when reading file.\n\n";
        return;
    }
    std::cout << "Solving ...";
    auto t1 = std::chrono::high_resolution_clock::now();
    Path path = maze.solve(i1, j1, i2, j2);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << "\nFinished in " << duration.count() << " milliseconds.\n";
    std::cout << "Path length: " << path.length() << "\n";
    float avgHeight = (j1 == j2) ? 0 : path.integral() /
        (static_cast<float>(j2) - static_cast<float>(j1));
    std::cout << "Average path i-component: " << avgHeight << "\n";
    std::cout << "Save as (empty if no saving): ";
    std::string pathFileName = stringInput();
    if (!pathFileName.empty()) {
        std::cout << "Saving ...";
        std::cout << (path.toBMP(pathFileName) ? " Finished." : " Failed!") << "\n";
    }
}

bool Commands::commandPrompt()
{
    std::cout << "Commands:\n";
    std::cout << "1 New maze\n";
    std::cout << "2 New path\n";
    std::cout << "3 Solve maze\n";
    std::cout << "4 Exit\n";
    std::cout << "Command: ";
    std::uint8_t command = integerInput();
    if (command == 1) {
        newMaze();
    }
    else if (command == 2) {
        newPath();
    }
    else if (command == 3) {
        solveMaze();
    }
    else if (command == 4) {
        return false;
    }
    else {
        std::cout << "Unknown command.\n";
    }
    std::cout << "\n";
    return true;
}