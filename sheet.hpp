#pragma once
#include "rapidyaml.hpp"

struct RawPart {
    int x;
    int y;
    int w;
    int h;
    RawPart(ryml::ConstNodeRef node);
};

class Sheet {
private:
    bool compressed = false;
    std::string name;
    std::string gfxFilename;
    std::vector<struct RawPart> parts;
public:
    Sheet(ryml::Tree sheetYml, std::string name);

    std::vector<struct RawPart> &getParts();
    std::string getName();
    std::string getGfxFilename();
    bool isCompressed();
};