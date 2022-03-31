add_library(UtilitiesLib Utilities.cpp)
target_include_directories(UtilitiesLib PUBLIC .)

add_library(PathLib "Path.cpp")
target_include_directories(PathLib PUBLIC .)
target_link_libraries(PathLib PUBLIC UtilitiesLib)

add_library(MazeLib Maze.cpp)
target_include_directories(MazeLib PUBLIC .)
target_link_libraries(MazeLib PUBLIC UtilitiesLib PathLib)

add_library(CommandsLib Commands.cpp)
target_include_directories(CommandsLib PUBLIC .)
target_link_libraries(CommandsLib PUBLIC UtilitiesLib PathLib MazeLib)

add_executable(Main Main.cpp)
target_include_directories(Main PUBLIC .)
target_link_libraries(Main PUBLIC CommandsLib)

install(TARGETS Main RUNTIME DESTINATION bin)