#pragma once
#include <vector>

#include "sheet.hpp"
#include "animdata.hpp"

struct AnimMetadata { //TODO: there is more that could be added here, but for now this is all we need for headers
    std::string name;
    int maxSprites;
    int index; // on GBA this would be the animation offset;

    AnimMetadata(std::string name, int index, int maxSprites);
};

struct Metadata {
    std::vector<struct AnimMetadata> anims;
};

class Exporter {
    protected:
    AnimData animData;
    Metadata metadata;
    
    bool isMetadataReady = false;
    
    public:
    virtual void exportAnimation();
    virtual void exportMetadata();
    Exporter(AnimData animData);
};
