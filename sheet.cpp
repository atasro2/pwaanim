#include "sheet.hpp"

RawPart::RawPart(ryml::ConstNodeRef node) { // parse a Part from yaml node
    if(!node.has_child("x")) {
        std::throw_with_nested(std::runtime_error("part has no x position."));
    }
    if(!node.has_child("y")) {
        std::throw_with_nested(std::runtime_error("part has no y position."));
    }
    if(!node.has_child("w")) {
        std::throw_with_nested(std::runtime_error("part has no width."));
    }
    if(!node.has_child("h")) {
        std::throw_with_nested(std::runtime_error("part has no height."));
    }
    node["x"] >> x;
    node["y"] >> y;
    node["w"] >> w;
    node["h"] >> h;
}

Sheet::Sheet(ryml::Tree animYml, std::string name) { // Sheet from yaml
    this->name = name;
    ryml::ConstNodeRef sheetcroot = animYml.crootref();
    if(sheetcroot.has_child("compressed"))
        sheetcroot["compressed"] >> compressed;
    if(!sheetcroot.has_child("gfx"))
        std::throw_with_nested(std::runtime_error("No graphics file has been specified in sheet."));
    sheetcroot["gfx"] >> gfxFilename;
    if(sheetcroot.has_child("alphaencode"))
        sheetcroot["alphaencode"] >> specialGBAAlpha;
    if(!sheetcroot.has_child("parts") || sheetcroot["parts"].num_children() == 0) {
        std::throw_with_nested(std::runtime_error("Sheet has no parts."));
    }
    int partCount = sheetcroot["parts"].num_children();
    for(int i = 0; i < partCount; i++) {
        try
        {
            parts.push_back(RawPart(sheetcroot["parts"][i]));
        }
        catch (...)
        {   
            std::throw_with_nested(std::runtime_error("failed to parse part " + std::to_string(i)));
        }
    }
}
std::vector<struct RawPart> &Sheet::getParts() {
    return parts;
}
std::string Sheet::getName() {
    return name;
}
std::string Sheet::getGfxFilename() {
    return gfxFilename;
}
bool Sheet::isCompressed() {
    return compressed;
}
bool Sheet::isSpecialGBAAlpha() {
    return specialGBAAlpha;
}