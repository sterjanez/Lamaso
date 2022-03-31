#include <iostream>
#include <string>
#include <Commands.h>

int main()
{
    std::cout << "Lamaso 8.0\n\nJanez Ster\n\n";
    while (Commands::commandPrompt()) {}
    return 0;
}