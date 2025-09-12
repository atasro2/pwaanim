#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <vector>
#include <filesystem>
#include <stdint.h>

#include "rapidyaml.hpp"

#include "pwaanim.hpp"

std::vector<unsigned char> readFileIntoVector(std::string filename) {
    std::ifstream fs(filename, std::ios::binary);
    std::vector<unsigned char> file((std::istreambuf_iterator<char>(fs)),
                 std::istreambuf_iterator<char>());
    return file;
} 

ryml::Tree readYamlFile(std::string filename) {
    std::ifstream ymlfs(filename);
    std::string ymlstr((std::istreambuf_iterator<char>(ymlfs)),
                 std::istreambuf_iterator<char>());
    return ryml::parse_in_arena(ymlstr.c_str());
} 

void blitTileIntoImage(std::vector<unsigned char> &image, int iw, int ih, int partx, int party, const unsigned char * tile, int tw, int th, int pal) {
    assert((tw % 8) == 0 && (th % 8) == 0 && "Tiles should be divisible by 8");
    for(int y = party; y < th+party; y++) {
        for(int x = partx; x < tw+partx; x+=2) {
            size_t byte_index = (y * iw + x);
            int tx = (x - partx) / 8;
            int ty = (y - party) / 8;
            image[byte_index] = tile[(tx+ty*tw/8)*32 + (y % 8) * 4 + (x/2) % 4] & 0xF;
            image[byte_index+1] = tile[(tx+ty*tw/8)*32 + (y % 8) * 4 + (x/2) % 4] & 0xF0;
            image[byte_index+1] >>= 4;
            image[byte_index] += pal * 16;
            image[byte_index+1] += pal * 16;
        }   
    }
}

std::vector<unsigned char> createTilesFromImage(int tw, int th, const std::vector<unsigned char> &image, int iw, int ih, int ix, int iy, int &pal) {
    assert((tw % 8) == 0 && (th % 8) == 0 && "Tiles should be divisible by 8");
    std::vector<unsigned char> tiles;
    tiles.resize((tw*th)/2);
    for(int y = 0; y < th; y++) {
        for(int x = 0; x < tw/2; x++) {
            int pixelL = image[(iy+y)*iw+ix+x*2] % 16;
            int pixelR = image[(iy+y)*iw+ix+x*2+1] % 16;
            int xx = x / 4;
            int yy = y / 8;
            int tileOffset = (xx+yy*(tw/8))*32;
            tiles[tileOffset + (y % 8) * 4 + x % 4] = pixelL | (pixelR << 4);
        }
    }
    pal = image[(iy)*iw+ix] / 16;
    return tiles;
}

std::vector<unsigned char> compressRLE16BitPWAA(std::vector<unsigned char> data) {
    std::vector<unsigned char> compressedData;
    int currentIndex = 0;
    int originalSize = data.size();
    assert((data.size() % 2) == 0);
    data.push_back(0); // ! we append a 0 to satisfy our simulated buggy code later on
    data.push_back(0); 
    while(true) {
        bool compress = false;
        int currentUncompIdx = currentIndex;
        int uncompressedSize = 0;
        while(currentIndex < originalSize && uncompressedSize <= 0x7FFF) { //! this check has a bug where it checks beyond the actual size of the data being passed in causing some erronius repeat blocks to generate
            uint16_t cur = INT16_LE(data[currentIndex], data[currentIndex+1]);
            uint16_t next = INT16_LE(data[currentIndex+2], data[currentIndex+3]);
            compress = cur == next;
            if(compress)
                break;
            currentIndex += 2;
            uncompressedSize++;
        }
        if(uncompressedSize > 0) {
            compressedData.push_back(uncompressedSize & 0xFF);
            compressedData.push_back((uncompressedSize >> 8) & 0xFF);
        }
        while(uncompressedSize--) {
            compressedData.push_back(data[currentUncompIdx++]);
            compressedData.push_back(data[currentUncompIdx++]);
        }
        if(compress) {
            int repeat = INT16_LE(data[currentUncompIdx], data[currentUncompIdx+1]);
            int repeatedBytes = 0;
            while(currentIndex+repeatedBytes*2 < originalSize && repeatedBytes <= 0x7FFF && repeat == INT16_LE(data[currentIndex+repeatedBytes*2], data[currentIndex+repeatedBytes*2+1])) {
                repeatedBytes++;
            }
            int repeatblock = 0x8000 | repeatedBytes;
            compressedData.push_back(repeatblock & 0xFF);
            compressedData.push_back((repeatblock >> 8) & 0xFF);
            compressedData.push_back(repeat & 0xFF);
            compressedData.push_back((repeat >> 8) & 0xFF);
            currentIndex += repeatedBytes*2;
        }
        if(currentIndex == originalSize)
            break;
    }
    return compressedData;
}