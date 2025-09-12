#pragma once

#include "pwaanim.hpp"
#include "export.hpp"

struct PartData {
    int w;
    int h;
    int offset;
    int palette;
};

struct SheetData {
    std::vector<struct PartData> parts;
    int offset;
    bool compressed;
};

struct ArrangementData {
    //std::vector<struct PartData> parts;
    int reloffset; // offset relative to start of blob
    int palMode;
};

class GbaExporter : public Exporter {
    private:
    fs::path pixpath;
    fs::path seqpath;
    std::unordered_map<std::string, struct SheetData> sheetMap;
    std::unordered_map<std::string, struct ArrangementData> arrangementData;
    
    std::vector<unsigned char> pixBytes;
    std::vector<unsigned char> seqBytes;

    void buildPix();
    void buildSeq();
    int serializeArrangement(FrameArrangement arrangement, std::string sheet, std::vector<unsigned char>&out);
    int serializeFrame(Frame frame, std::vector<unsigned char>&out);
    public:
    GbaExporter(AnimData &anim, fs::path seq, fs::path pix);
    void exportAnimation();
};