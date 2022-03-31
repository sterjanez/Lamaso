#ifndef COMMANDS_H
#define COMMANDS_H

namespace Commands {

    // Prompt for maze dimensions, seed number,
    // possible solution path file name, and file name, and create a new maze.
    void newMaze();

    // Prompt for dimesions, seed number and path parameters, and file name, and create a path.
    void newPath();

    // Prompt for maze file and solve the maze.
    void solveMaze();

    // Prompt and execute command. Return false if exit is called.
    bool commandPrompt();

}

#endif