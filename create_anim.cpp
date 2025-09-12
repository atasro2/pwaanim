#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <vector>
#include <filesystem>
#include <map>

#include "lodepng.h"
#include "pwaanim.hpp"


struct State {
    std::unordered_map<std::string, struct SheetData> sheetMap;
};


struct SheetDef {
    std::vector<struct RawPart> parts;
    std::string gfxFilename;
    bool specialAlpha;
    bool compressed;
};

#if 0
int parseSheetYaml(ryml::Tree sheetYml, struct SheetDef &out) {
    ryml::ConstNodeRef sheetcroot = sheetYml.crootref();
    out.compressed = false;
    if(sheetcroot.has_child("compressed"))
        sheetcroot["compressed"] >> out.compressed;
    if(!sheetcroot.has_child("gfx")) {
        std::cerr << "No graphics file has been specified in sheet." << std::endl;
        return 1;
    }
    sheetcroot["gfx"] >> out.gfxFilename;
    out.specialAlpha = false;
    if(sheetcroot.has_child("alphaencode")) {
        sheetcroot["alphaencode"] >> out.specialAlpha;
    }
    if(!sheetcroot.has_child("parts") || sheetcroot["parts"].num_children() == 0) {
        std::cerr << "Sheet has no parts." << std::endl;
        return 1;
    }
    int partCount = sheetcroot["parts"].num_children();
    for(int i = 0; i < partCount; i++) {
        struct RawPart part;
        if(!sheetcroot["parts"][i].has_child("x")) {
            std::cerr << "Part " << i << " has no x." << std::endl;
            return 1;
        }
        if(!sheetcroot["parts"][i].has_child("y")) {
            std::cerr << "Part " << i << " has no y." << std::endl;
            return 1;
        }
        if(!sheetcroot["parts"][i].has_child("w")) {
            std::cerr << "Part " << i << " has no w." << std::endl;
            return 1;
        }
        if(!sheetcroot["parts"][i].has_child("h")) {
            std::cerr << "Part " << i << " has no h." << std::endl;
            return 1;
        }
        sheetcroot["parts"][i]["x"] >> part.x;
        sheetcroot["parts"][i]["y"] >> part.y;
        sheetcroot["parts"][i]["w"] >> part.w;
        sheetcroot["parts"][i]["h"] >> part.h;    
        out.parts.push_back(part);
    }
    return 0;
}
#endif

int compileAnimYamlIntoPixSeq(fs::path yamlfile) {
    if(!fs::directory_entry{yamlfile}.exists()) {
        std::cerr << "animation yaml file does not exist" << std::endl;
        return 1;
    }
    /*
    fs::path pixpath = yamlfile.parent_path() / yamlfile.stem().concat(".pix");
    fs::path seqpath = yamlfile.parent_path() / yamlfile.stem().concat(".seq");
    ryml::Tree ymltree = readYamlFile(yamlfile);
    // Build PIX file with metadata
    std::unordered_map<std::string, struct SheetData> sheetMap;
    int sheetCount = ymltree["sheets"].num_children();
    int currentSheetOffset = 0;
    std::vector<unsigned char> pixBytes;
    for(int sheet = 0; sheet < sheetCount; sheet++) {
        std::string sheetyml;
        ymltree["sheets"][sheet] >> sheetyml;
        fs::path sheetymlpath(sheetyml);
        ryml::Tree sheetymltree = readYamlFile(yamlfile.parent_path() / sheetymlpath);
        
        struct SheetDef sheetdef;
        if(parseSheetYaml(sheetymltree, sheetdef)) return 1;

        sheetMap[sheetymlpath.stem()].offset = currentSheetOffset;
        sheetMap[sheetymlpath.stem()].compressed = sheetdef.compressed;

        fs::path pngpath(sheetdef.gfxFilename);
        
        std::vector<unsigned char> pngbytes = readFileIntoVector(yamlfile.parent_path() / pngpath);
        lodepng::State state;
        std::vector<unsigned char> image;
        unsigned iw, ih;

        state.decoder.color_convert = true;
        state.info_raw.colortype = LCT_PALETTE;
        state.info_raw.bitdepth = 8;

        unsigned error = lodepng::decode(image, iw, ih, state, pngbytes);
        if(error) {
            std::cerr << "error " << error << ": " << lodepng_error_text(error) << std::endl;
            return 1;
        }

        if(state.info_png.color.colortype != LCT_PALETTE) {
            std::cerr << "Image " << yamlfile.parent_path() / pngpath << " is not an indexed png file." << std::endl;
            return 1;
        }
        
        // ! having color convert be false breaks some images for some reason and i wasn't gonna bother figuring out why that is
        // so we use the palette from the PNG directly instead of the info_raw palette
        
        int palCount = state.info_png.color.palettesize;
        assert((palCount % 16) == 0 && "Palette should be divisible by 16"); // TODO: this doesn't have to be the case
        palCount /= 16;
        if(sheetdef.compressed)
            palCount |= 0x80000000;
        pixBytes.push_back(palCount & 0xFF);
        pixBytes.push_back((palCount >> 8) & 0xFF);
        pixBytes.push_back((palCount >> 16) & 0xFF);
        pixBytes.push_back((palCount >> 24) & 0xFF);

        for(int i = 0; i < state.info_png.color.palettesize; i++) {
            int r = state.info_png.color.palette[i*4];
            int g = state.info_png.color.palette[i*4+1];
            int b = state.info_png.color.palette[i*4+2];
            int a = 0;
            r >>= 3;
            g >>= 3;
            b >>= 3;
            if(sheetdef.specialAlpha)
                a = state.info_png.color.palette[i*4+3] / 255;
            uint16_t color = r | (g << 5) | (b << 10) | (a << 15);
            pixBytes.push_back(color & 0xFF);
            pixBytes.push_back((color >> 8) & 0xFF);
        }

        if(sheetdef.compressed) {
            std::vector<unsigned int> parttable;
            int partCount = sheetdef.parts.size();
            parttable.resize(partCount);
            int tileStart = partCount * 4;
            std::vector<unsigned char> tileBlob;
            for(int i = 0; i < partCount; i++) {
                // Compressed tiles have a table to where the data for each part starts so their tileid is an index into the table
                sheetMap[sheetymlpath.stem()].parts.push_back((struct PartData){sheetdef.parts[i].w, sheetdef.parts[i].h, i});
                parttable[i] = tileStart;

                std::vector<unsigned char> tiles = createTilesFromImage(sheetdef.parts[i].w, sheetdef.parts[i].h, image, iw, ih, sheetdef.parts[i].x, sheetdef.parts[i].y);
                std::vector<unsigned char> compressedPart = compressRLE16BitPWAA(tiles);
                tileBlob.insert(tileBlob.end(), compressedPart.begin(), compressedPart.end()); // concat compressed part into pix
                tileStart += compressedPart.size();
            }
            if((tileBlob.size() % 4)) { // Pad to a 4 byte boundary
                tileBlob.push_back(tileBlob[0]);
                tileBlob.push_back(tileBlob[1]);
            }
            for(int table = 0; table < parttable.size(); table++) {
                pixBytes.push_back(parttable[table] & 0xFF);
                pixBytes.push_back((parttable[table] >> 8) & 0xFF);
                pixBytes.push_back((parttable[table] >> 16) & 0xFF);
                pixBytes.push_back((parttable[table] >> 24) & 0xFF);
            }
            pixBytes.insert(pixBytes.end(), tileBlob.begin(), tileBlob.end());
        } else {
            int partCount = sheetymltree["parts"].num_children();
            std::vector<unsigned char> tileBlob;
            int tileStart = 0;
            for(int i = 0; i < partCount; i++) {
                sheetMap[sheetymlpath.stem()].parts.push_back((struct PartData){sheetdef.parts[i].w, sheetdef.parts[i].h, tileStart});
                std::vector<unsigned char> partTiles = createTilesFromImage(sheetdef.parts[i].w, sheetdef.parts[i].h, image, iw, ih, sheetdef.parts[i].x, sheetdef.parts[i].y);
                tileBlob.insert(tileBlob.end(), partTiles.begin(), partTiles.end()); // concat compressed part into pix
                tileStart += (sheetdef.parts[i].w * sheetdef.parts[i].h) / 2;
            }
            pixBytes.insert(pixBytes.end(), tileBlob.begin(), tileBlob.end());
        }
    }
    */
    // We have built our sheet files and acquired the necessery metadata to build our SEQ 


    std::vector<unsigned char> seqBytes;
    // First we need frames







    //std::cout << pixpath << std::endl;
    //lodepng::save_file(pixBytes, pixpath);
    return 0;
}