#ifndef UTILITIES_H
#define UTILITIES_H

#include <cstdint>
#include <string>
#include <fstream>

namespace Utilities {

    // Update seed and return a random 8-bit unsigned integer.
    std::uint8_t randUint8(std::int32_t &seed);

    // Update seed and return a random 32-bit unsigned integer.
    std::uint32_t randUint32(std::int32_t &seed);

    // Create (or overwrite) a binary file and write BMP file header,
    // Windows NT bitmap info header and color palette for a monochrome BMP file.
    // Return file stream class of an open file. Return false if failed.
    bool writeBMP(std::string const &fileName,
        std::int32_t width, std::int32_t height, std::ofstream &file);

    // Open and read a monochrome Windows BMP file's file header, info header and palette.
    // Return image data. Return false if illegal file format or failed reading.
    bool readBMP(std::string const &fileName,
        std::int32_t &width, std::int32_t &height, std::ifstream &file);

}

#endif