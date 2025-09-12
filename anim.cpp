#include "anim.hpp"

// --- SpriteArrangement

SpriteArrangement::SpriteArrangement(ryml::ConstNodeRef node) {
    if(!node.has_child("x")) {
        std::throw_with_nested(std::runtime_error("Arrangement sprite has no x position."));
    }
    if(!node.has_child("y")) {
        std::throw_with_nested(std::runtime_error("Arrangement sprite has no y position."));
    }
    if(!node.has_child("part")) {
        std::throw_with_nested(std::runtime_error("Arrangement sprite has no part."));
    }
    if(node.has_child("palette")) {
        node["palette"] >> palette;
        //std::throw_with_nested(std::runtime_error("Arrangement sprite has no palette."));
        //TODO: should warn?
    } else {
        palette = -1; // This signals to get the palette from the partData
    }
    node["part"] >> part;
    node["x"] >> x;
    node["y"] >> y;
}

// --- FrameArrangement

FrameArrangement::FrameArrangement(ryml::ConstNodeRef node) {
    ryml::ConstNodeRef spritesNode = node.find_child("sprites");
    ryml::csubstr key = node.key();
    std::string frameName(key.data(), key.size());
    if(!node.has_child("sprites")) {
        std::throw_with_nested(std::runtime_error("Arrangement '" + frameName + "' has no sprites node."));
    }

    this->name = frameName;
    if(!node.has_child("pal_mode")) {
        std::throw_with_nested(std::runtime_error("Arrangement should define the resolution of palette used for its sprites."));
    }
    node["pal_mode"] >> palMode;
    
    if(!spritesNode.has_children()) {
        // TODO We ignore empty frames for now but it should warn/error
        return;
        //std::throw_with_nested(std::runtime_error("Arrangement " + frameName + " shouldn't be empty"));
    }

    //if(node.num_children() > 1)
    //    std::throw_with_nested(std::runtime_error("Arrangement " + frameName + " has more than 1 child"));

    if(!spritesNode.is_seq())
        std::throw_with_nested(std::runtime_error("Expected sequence for Arrangement " + frameName));

    int partCount = spritesNode.num_children();
    for(int i = 0; i < partCount; i++) {
        try {
            this->sprites.push_back(SpriteArrangement(spritesNode[i]));
        } catch(...) {
            std::throw_with_nested(std::runtime_error("Error while parsing part " + std::to_string(i)));
        }
    
    }
}

// --- Property

Property::Property() {
    enabled = false;
    id = 0;
}

Property::Property(ryml::ConstNodeRef node)  {
    if(!node.has_child("id")) {
        std::throw_with_nested(std::runtime_error("Proprty doesn't have an assosciated id"));
    }
    node["id"] >> id;
    if(!node.has_child("enabled")) {
        enabled = true;
    } else {
        node["enabled"] >> enabled;
    }
}


// --- Frame

Frame::Frame(ryml::ConstNodeRef node) {
    if(!node.has_child("duration")) {
        std::throw_with_nested(std::runtime_error("Frame has no duration."));
    }
    if(!node.has_child("arrangement")) {
        std::throw_with_nested(std::runtime_error("Frame has no arrangement."));
    }
    if(node.has_child("flag10")) {
        node["flag10"] >> flag10;
    }
    if(node.has_child("flag40")) {
        node["flag40"] >> flag40;
    }
    if(node.has_child("sfx") && node["sfx"].has_children()) {
        sfx = Property(node["sfx"]);
    }
    if(node.has_child("action") && node["action"].has_children()) {
        action = Property(node["action"]);
    }
    node["duration"] >> duration;
    node["arrangement"] >> arrangement;
}


Anim::Anim(ryml::Tree animYml, std::string name) { // Anim from yaml
    this->name = name;
    ryml::ConstNodeRef animcroot = animYml.crootref();
    if(!animcroot.has_child("sheet"))
        std::throw_with_nested(std::runtime_error("No sheet has been specified in animation."));
    animcroot["sheet"] >> sheet;
    if(!animcroot.has_child("arrangements") || animcroot["arrangements"].num_children() == 0) {
        std::throw_with_nested(std::runtime_error("Anim has no arrangements."));
    }
    int arrangementCount = animcroot["arrangements"].num_children();
    for(int i = 0; i < arrangementCount; i++) {
        try
        {
            arrangements.push_back(FrameArrangement(animcroot["arrangements"][i]));
        }
        catch (...)
        {   
            std::throw_with_nested(std::runtime_error("failed to parse arrangement " + std::to_string(i)));
        }
    }
    
    if(!animcroot.has_child("frames") || animcroot["frames"].num_children() == 0) {
        std::throw_with_nested(std::runtime_error("Anim has no frames."));
    }
    int frameCount = animcroot["frames"].num_children();
    for(int i = 0; i < frameCount; i++) {
        try
        {
            frames.push_back(Frame(animcroot["frames"][i]));
        }
        catch (...)
        {   
            std::throw_with_nested(std::runtime_error("failed to parse frame " + std::to_string(i)));
        }
    }
    
}

std::string Anim::getName() {
    return name;
}

std::string Anim::getSheet() {
    return sheet;
}

std::vector<struct FrameArrangement> &Anim::getArrangements() {
    return arrangements;
}

std::vector<struct Frame> &Anim::getFrames() {
    return frames;
}

/*
    if(!node.has_children())
        std::throw_with_nested(std::runtime_error("Animation contains no frames"));
    if(!node.is_seq())
        std::throw_with_nested(std::runtime_error("Expected sequence for frames node"));

    this->name = frameName;
    int partCount = childNode.num_children();
    for(int i = 0; i < partCount; i++) {
        // TODO handle exception
        this->sprites.push_back(SpriteArrangement(childNode[i]));
    }
}
*/