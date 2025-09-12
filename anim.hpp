#pragma once
#include "rapidyaml.hpp"
#include "sheet.hpp"

struct SpriteArrangement {
    int part;
    int x;
    int y;
    int palette;
    SpriteArrangement(ryml::ConstNodeRef node);
};

struct FrameArrangement {
    std::string name;
    int palMode;
    std::vector<struct SpriteArrangement> sprites;
    FrameArrangement(ryml::ConstNodeRef node);
};

struct Property {
    int id = 0;
    bool enabled = false;
    Property();
    Property(ryml::ConstNodeRef node);
};

struct Frame {
    int duration;
    bool flag10 = false;
    bool flag40 = false;
    struct Property sfx;
    struct Property action;
    std::string arrangement;
    Frame();
    Frame(ryml::ConstNodeRef node);
};

class Anim {
    std::string name;
    std::string sheet;
    std::vector<struct FrameArrangement> arrangements;
    std::vector<struct Frame> frames;
public:
    Anim(ryml::Tree animYml, std::string name);
    
    std::string getName();
    std::string getSheet();
    std::vector<struct FrameArrangement> &getArrangements();
    std::vector<struct Frame> &getFrames();
};