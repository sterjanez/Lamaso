#include <cstdint>
#include <string>
#include <fstream>
#include <Utilities.h>

std::uint8_t Utilities::randUint8(std::int32_t &seed)
{
    return ((seed = seed * 214013 + 2531011) >> 24) & 0xff;
}

std::uint32_t Utilities::randUint32(std::int32_t &seed)
{
    return (randUint8(seed) << 24) + (randUint8(seed) << 16) +
        (randUint8(seed) << 8) + randUint8(seed);
}

bool Utilities::writeBMP(std::string const &fileName,
    std::int32_t width, std::int32_t height, std::ofstream &file)
{
    file.open(fileName, std::ios::binary | std::ios::out);
    if (!file) {
        return false;
    }
    std::uint32_t widthBytes = (((width - 1) >> 5) + 1) << 2;
    std::uint32_t imageSize = static_cast<std::uint32_t>(height) * widthBytes;
    std::uint32_t fileSize = imageSize + 14 + 40 + 8;
    char const fileHeader[14] = {
        'B', 'M',
        char(fileSize), char(fileSize >> 8), char(fileSize >> 16), char(fileSize >> 24), // file size
        0, 0, // not used
        0, 0, // not used
        14 + 40 + 8, 0, 0, 0}; // image data offset
    char const infoHeader[40] = {
        40, 0, 0, 0, //infoHeader size
        char(width), char(width >> 8), char(width >> 16), char(width >> 24), // width
        char(height), char(height >> 8), char(height >> 16), char(height >> 24), // height
        1, 0, // number of color planes
        1, 0, // bits per pixel
        0, 0, 0, 0, // no compression
        char(imageSize), char(imageSize >> 8),
        char(imageSize >> 16), char(imageSize >> 24), // image data size
        0, 0, 0, 0, // horizontal resolution, optional
        0, 0, 0, 0, // vertical resolution, optional
        0, 0, 0, 0, // number of colors in pallete
        0, 0, 0, 0}; // number of important colors
    char const pallete[8] = {
        0, 0, 0, 0, // black
        char(0xff), char(0xff), char(0xff), 0}; // white
    return file.write(fileHeader, 14) && file.write(infoHeader, 40) && file.write(pallete, 8);
}

bool Utilities::readBMP(std::string const &fileName,
    std::int32_t &width, std::int32_t &height, std::ifstream &file)
{
    file.open(fileName, std::ios::binary | std::ios::in);
    if (!file) {
        return false;
    }
    char fileHeader[14];
    char infoHeader[40];
    char palette[8];
    if (!file.read(fileHeader, 14) || !file.read(infoHeader, 40) || !file.read(palette, 8)) {
        return false;
    }
    height = 0;
    width = 0;
    for (std::uint8_t i = 0; i < 4; i++) {
        width <<= 8;
        width |= static_cast<std::uint8_t>(infoHeader[7 - i]);
        height <<= 8;
        height |= static_cast<std::uint8_t>(infoHeader[11 - i]);
    }
    if (width < 0 || height < 0) {
        return false;
    }
    std::uint32_t widthBytes = (((width - 1) >> 5) + 1) << 2;
    std::uint32_t imageSize = static_cast<std::uint32_t>(height) * widthBytes;
    std::uint32_t fileSize = imageSize + 14 + 40 + 8;
    return fileHeader[0] == 'B' && fileHeader[1] == 'M' &&
        fileHeader[2] == char(fileSize) &&
        fileHeader[3] == char(fileSize >> 8) &&
        fileHeader[4] == char(fileSize >> 16) &&
        fileHeader[5] == char(fileSize >> 24) &&
        fileHeader[10] == 14 + 40 + 8 &&
        fileHeader[11] == 0 &&
        fileHeader[12] == 0 &&
        fileHeader[13] == 0 &&
        infoHeader[0] == 40 &&
        infoHeader[1] == 0 &&
        infoHeader[2] == 0 &&
        infoHeader[3] == 0 &&
        infoHeader[12] == 1 &&
        infoHeader[13] == 0 &&
        infoHeader[14] == 1 &&
        infoHeader[15] == 0 &&
        infoHeader[16] == 0 &&
        infoHeader[17] == 0 &&
        infoHeader[18] == 0 &&
        infoHeader[19] == 0 &&
        infoHeader[20] == char(imageSize) &&
        infoHeader[21] == char(imageSize >> 8) &&
        infoHeader[22] == char(imageSize >> 16) &&
        infoHeader[23] == char(imageSize >> 24) &&
        infoHeader[32] == 0 &&
        infoHeader[33] == 0 &&
        infoHeader[34] == 0 &&
        infoHeader[35] == 0 &&
        infoHeader[36] == 0 &&
        infoHeader[37] == 0 &&
        infoHeader[38] == 0 &&
        infoHeader[39] == 0 &&
        palette[0] == 0 &&
        palette[1] == 0 &&
        palette[2] == 0 &&
        palette[3] == 0 &&
        palette[4] == char(0xff) &&
        palette[5] == char(0xff) &&
        palette[6] == char(0xff) &&
        palette[7] == 0;
}