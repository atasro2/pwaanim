#pragma once

#define INT32_LE(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))
#define INT16_LE(a, b) (((b) << 8) | (a))

#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <vector>
#include <filesystem>

#include "rapidyaml.hpp"

namespace fs = std::filesystem;

std::vector<unsigned char> readFileIntoVector(std::string filename);
ryml::Tree readYamlFile(std::string filename);

void blitTileIntoImage(std::vector<unsigned char> &image, int iw, int ih, int partx, int party, const unsigned char * tile, int tw, int th, int pal);
std::vector<unsigned char> createTilesFromImage(int tw, int th, const std::vector<unsigned char> &image, int iw, int ih, int ix, int iy, int &pal);
std::vector<unsigned char> compressRLE16BitPWAA(std::vector<unsigned char> data);
std::vector<unsigned char> decompressRLE16BitPWAA(unsigned char * data, unsigned int size, unsigned int &compressedSize);


int compileAnimYamlIntoPixSeq(fs::path yamlfile);
