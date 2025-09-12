#include <utility>
#include <string>
#include <filesystem>
#include "pwaanim.hpp"
#include "export.hpp"
#include "lodepng.h"
#include "gba.hpp"

void GbaExporter::buildPix() {
    int currentSheetOffset = 0;
    
    std::vector<Sheet> &sheets = animData.getSheets();
    for(unsigned int i = 0; i < sheets.size(); i++) {
        Sheet &sheet = sheets[i]; 
        //sheetMap[sheet.getName()].offset = currentSheetOffset;
        sheetMap[sheet.getName()].offset = pixBytes.size(); // this is probably safer
        sheetMap[sheet.getName()].compressed = sheet.isCompressed();

        fs::path pngpath(sheet.getGfxFilename());
        
        std::vector<unsigned char> pngbytes = readFileIntoVector(animData.ymlPath.parent_path() / pngpath);
        lodepng::State state;
        std::vector<unsigned char> image;
        unsigned iw, ih;

        state.decoder.color_convert = true;
        state.info_raw.colortype = LCT_PALETTE;
        state.info_raw.bitdepth = 8;

        unsigned error = lodepng::decode(image, iw, ih, state, pngbytes);
        if(error) {
            std::stringstream errorString;
            errorString << "image decode failed on sheet " << sheet.getName() << ": " << lodepng_error_text(error);
            std::throw_with_nested(std::runtime_error(errorString.str()));
        }

        if(state.info_png.color.colortype != LCT_PALETTE) {
            std::stringstream errorString;
            errorString << "Image " << animData.ymlPath / pngpath << " is not an indexed png file." << std::endl;
            std::throw_with_nested(std::runtime_error(errorString.str()));
        }
        
        // ! having color convert be false breaks some images for some reason and i wasn't gonna bother figuring out why that is
        // so we use the palette from the PNG directly instead of the info_raw palette
        
        int palCount = state.info_png.color.palettesize;
        assert((palCount % 16) == 0 && "Palette should be divisible by 16"); // TODO: this doesn't have to be the case
        
        
        palCount /= 16;
        if(sheet.isCompressed())
            palCount |= 0x80000000;
        pixBytes.push_back(palCount & 0xFF);
        pixBytes.push_back((palCount >> 8) & 0xFF);
        pixBytes.push_back((palCount >> 16) & 0xFF);
        pixBytes.push_back((palCount >> 24) & 0xFF);
        currentSheetOffset += 4;
        for(unsigned int i = 0; i < state.info_png.color.palettesize; i++) {
            int r = state.info_png.color.palette[i*4];
            int g = state.info_png.color.palette[i*4+1];
            int b = state.info_png.color.palette[i*4+2];
            int a = 0;
            r >>= 3;
            g >>= 3;
            b >>= 3;
            if(sheet.isSpecialGBAAlpha())
                a = !(state.info_png.color.palette[i*4+3] / 255);
            uint16_t color = r | (g << 5) | (b << 10) | (a << 15);
            pixBytes.push_back(color & 0xFF);
            pixBytes.push_back((color >> 8) & 0xFF);
            currentSheetOffset += 2;
        }
        
        std::vector<unsigned char> tileBlob;
        std::vector<struct RawPart> parts = sheet.getParts();
        int partCount = parts.size();
        
        std::vector<unsigned int> parttable;
        parttable.resize(partCount);
        int tileStart = sheet.isCompressed() ? partCount * 4 : 0;

        for(int j = 0; j < partCount; j++) {
            // Compressed tiles have a table to where the data for each part starts so their tileid is an index into the table
            int tileId = tileStart;
            int pal;
            std::vector<unsigned char> tiles = createTilesFromImage(parts[j].w, parts[j].h, image, iw, ih, parts[j].x, parts[j].y, pal);
            std::vector<unsigned char> partTiles = tiles;
            if(sheet.isCompressed()) {
                partTiles = compressRLE16BitPWAA(tiles);
                parttable[j] = tileStart;
                tileId = j;
            }
            sheetMap[sheet.getName()].parts.push_back((struct PartData){parts[j].w, parts[j].h, tileId, pal});
            tileBlob.insert(tileBlob.end(), partTiles.begin(), partTiles.end());
            tileStart += partTiles.size();
        }

        if(sheet.isCompressed()) {
            for(unsigned int table = 0; table < parttable.size(); table++) {
                pixBytes.push_back(parttable[table] & 0xFF);
                pixBytes.push_back((parttable[table] >> 8) & 0xFF);
                pixBytes.push_back((parttable[table] >> 16) & 0xFF);
                pixBytes.push_back((parttable[table] >> 24) & 0xFF);
                currentSheetOffset += 4;
            }
            if((tileBlob.size() % 4)) { // Pad to a 4 byte boundary
                tileBlob.push_back(tileBlob[0]);
                tileBlob.push_back(tileBlob[1]);
            }
        }
        pixBytes.insert(pixBytes.end(), tileBlob.begin(), tileBlob.end());
        currentSheetOffset += tileBlob.size();
    }
}

int getGbaSpriteShapeSize(int width, int height) {
    const std::map<std::pair<int, int>, int> spriteSizeMap = {
        {{8,  8},  0x0},
        {{16, 16}, 0x4},
        {{32, 32}, 0x8},
        {{64, 64}, 0xC},
        {{16, 8},  0x1},
        {{32, 8},  0x5},
        {{32, 16}, 0x9},
        {{64, 32}, 0xD},
        {{8, 16},  0x2},
        {{8, 32},  0x6},
        {{16, 32}, 0xA},
        {{32, 64}, 0xE}
    };

    auto it = spriteSizeMap.find({width, height});
    if (it != spriteSizeMap.end()) {
        return it->second;
    }
    return -1;
}

int GbaExporter::serializeArrangement(FrameArrangement arrangement, std::string sheet, std::vector<unsigned char>&out) {
    int size = 0;
    int spriteCount = arrangement.sprites.size();
    if(spriteCount == 0) // TODO: this is very dumb
        return size;
    out.push_back(spriteCount & 0xFF);
    out.push_back((spriteCount >> 8) & 0xFF);
    out.push_back((spriteCount >> 16) & 0xFF);
    out.push_back((spriteCount >> 24) & 0xFF);
    size += 4;
    struct SheetData sheetData = sheetMap[sheet];
    for(int i = 0; i < spriteCount; i++) {
        struct PartData partData = sheetData.parts[arrangement.sprites[i].part]; 
        out.push_back(arrangement.sprites[i].x & 0xFF);
        out.push_back(arrangement.sprites[i].y & 0xFF);
        
        int palette;
        int tileId;
        int tileMask = 0x1FF;
        int maxPalCnt = 1;
        switch(arrangement.palMode) {
            case 1:
            tileMask = 0x7FF;
            break;
            case 2:
            tileMask = 0x3FF;
            break;
            case 3:
            tileMask = 0x1FF;
            break;
        }
        maxPalCnt <<= arrangement.palMode;
        if(arrangement.sprites[i].palette < 0) { // Palette has no override in animations and we get it from the sheet
            palette = partData.palette;
        } else {
            palette = arrangement.sprites[i].palette;
        }
        if(maxPalCnt <= palette) {
            std::stringstream errorString;
            errorString << "sprite # " << i << " has an impossible palette for the current pallete mode";
            std::throw_with_nested(std::runtime_error(errorString.str()));
        }

        int sizeShape = getGbaSpriteShapeSize(partData.w, partData.h);
        if(sizeShape < 0) {
            std::stringstream errorString;
            errorString << "sprite # " << i << " has an impossible size for the GBA";
            std::throw_with_nested(std::runtime_error(errorString.str()));
        }

        if(sheetData.compressed) {
            tileId = arrangement.sprites[i].part;
        } else {
            tileId = partData.offset / 0x20; // byte offset -> tile offset
        }

        if(tileId > tileMask) {
            std::stringstream errorString;
            errorString << "tileId " << tileId << " of part " << arrangement.sprites[i].part << " is too big for tileMask " << tileMask;
            std::throw_with_nested(std::runtime_error(errorString.str()));   
        }

        int data = 0;
        data |= sizeShape << 12;
        data |= palette << (12-arrangement.palMode);
        data |= tileId & tileMask; // technically this mask is not needed

        out.push_back(data & 0xFF);
        out.push_back((data >> 8) & 0xFF);
        size += 4;
    }
    return size;
}

int GbaExporter::serializeFrame(Frame frame, std::vector<unsigned char>&out) {
    int palMode = arrangementData[frame.arrangement].palMode;
    int spriteDataOff = arrangementData[frame.arrangement].reloffset;

    out.push_back(spriteDataOff & 0xFF);
    out.push_back((spriteDataOff >> 8) & 0xFF);

    out.push_back(frame.duration & 0xFF);

    int palBit = 0; // 1 bit palette is default  
    switch(palMode) {
        case 2:
            palBit = 0x8;
            break;
        case 3:
            palBit = 0x1;
            break;
    }
    int flags = 0;
    flags |= palBit;
    flags |= frame.sfx.enabled ? 0x2 : 0;
    flags |= frame.action.enabled ? 0x4 : 0;
    flags |= frame.flag10 ? 0x10 : 0;
    flags |= frame.flag40 ? 0x40 : 0;
    out.push_back(flags & 0xFF);
    out.push_back(frame.sfx.id);
    out.push_back(frame.action.id);
    out.push_back(0);
    out.push_back(0);
    return 8;
}

void GbaExporter::buildSeq() {
    std::vector<Anim> &anims = animData.getAnims();
    for(unsigned int i = 0; i < anims.size(); i++) {
        std::vector<Frame> &frames = anims[i].getFrames();
        std::vector<FrameArrangement> &arrangements = anims[i].getArrangements();
        int frameCount = frames.size();
        // Frame Header
        seqBytes.push_back(0);
        seqBytes.push_back(0);
        seqBytes.push_back(frameCount & 0xFF);
        seqBytes.push_back((frameCount >> 8) & 0xFF);

        // Sheet offset
        int sheetOff = sheetMap[anims[i].getSheet()].offset;
        seqBytes.push_back(sheetOff & 0xFF);
        seqBytes.push_back((sheetOff >> 8) & 0xFF);
        seqBytes.push_back((sheetOff >> 16) & 0xFF);
        seqBytes.push_back((sheetOff >> 24) & 0xFF);

        int arrOffset = frameCount * 8 + 8;
        
        std::vector<unsigned char> arrBlob;
        std::vector<unsigned char> frameBlob;

        for(unsigned int j = 0; j < arrangements.size(); j++) {
            arrangementData[arrangements[j].name].palMode = arrangements[j].palMode;  
            arrangementData[arrangements[j].name].reloffset = arrOffset;
            try {
                arrOffset += serializeArrangement(arrangements[j], anims[i].getSheet(), arrBlob);
            } catch(...) {
                std::throw_with_nested(std::runtime_error("Error while serializing arrangement " + arrangements[j].name + " of anim " + anims[i].getName()));
            }
        }
        for(unsigned int j = 0; j < frames.size(); j++) {
            serializeFrame(frames[j], frameBlob);
        }
        // First we have frameData
        seqBytes.insert(seqBytes.end(), frameBlob.begin(), frameBlob.end());
        // Then we have sprites
        seqBytes.insert(seqBytes.end(), arrBlob.begin(), arrBlob.end());
    }
}

GbaExporter::GbaExporter(AnimData &anim, fs::path seq, fs::path pix) : Exporter(anim) {
    pixpath = pix;
    seqpath = seq;
}

void GbaExporter::exportAnimation() {
    buildPix();
    buildSeq();
    lodepng::save_file(pixBytes, pixpath);
    lodepng::save_file(seqBytes, seqpath);
}