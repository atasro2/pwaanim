#include "export.hpp"

AnimMetadata::AnimMetadata(std::string name, int index, int maxSprites) {
    this->name = name;
    this->index = index;
    this->maxSprites = maxSprites;
}

Exporter::Exporter(AnimData animData) {
    this->animData = animData;
}

void Exporter::exportAnimation() {
    // expected to be called by child function
    isMetadataReady = true;
}

void Exporter::exportMetadata() {
    if(!isMetadataReady)
        return; //TODO: raise exception
}