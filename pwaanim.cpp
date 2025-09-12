#define RYML_SINGLE_HDR_DEFINE_NOW
#include "rapidyaml.hpp"

#include "lodepng.h"

#include "argparse.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <vector>
#include <filesystem>

#include "pwaanim.hpp"
#include "animdata.hpp"
#include "export.hpp"
#include "converter/gba.hpp"

namespace fs = std::filesystem;

int dumpPNGFromPixAndYml(fs::path ymlpath, fs::path pixpath) {
    if(!fs::directory_entry{ymlpath}.exists()) {
        std::cerr << "animation yaml file does not exist" << std::endl;
        return 1;
    }
    if(!fs::directory_entry{pixpath}.exists()) {
        std::cerr << "animation pix file does not exist" << std::endl;
        return 1;
    }
    ryml::Tree ymltree = readYamlFile(ymlpath);
    std::string sheetyml;

    unsigned int pixOffset = 0;
    lodepng::State state;
    std::vector<unsigned char> pixFile = readFileIntoVector(pixpath);
    int sheetCount = ymltree["sheets"].num_children();
    for(int sheet = 0; sheet < sheetCount; sheet++) {
        ymltree["sheets"][sheet] >> sheetyml;
        ryml::Tree sheetymltree = readYamlFile(ymlpath.parent_path() / sheetyml);
        ryml::ConstNodeRef root = sheetymltree.crootref();
        unsigned int palCount = INT32_LE(pixFile[pixOffset+0], pixFile[pixOffset+1], pixFile[pixOffset+2], pixFile[pixOffset+3]);
        bool compressed = !!(palCount & 0x80000000);
        palCount &= ~0x80000000;
        pixOffset += 4;

        lodepng_palette_clear(&state.info_png.color);
        lodepng_palette_clear(&state.info_raw);

        for(int i = 0; i < 16 * palCount; i++) {
            uint16_t color = INT16_LE(pixFile[pixOffset+0], pixFile[pixOffset+1]);
            int r = color & 0x1F;
            int g = (color >> 5) & 0x1F;
            int b = (color >> 10) & 0x1F;
            int a = !((color >> 15) & 1);
            r = (r * 255) / 31;
            g = (g * 255) / 31;
            b = (b * 255) / 31;
            
            // Not a great idea when your first color doesn't have the alpha bit set 
            // and then you have a bunch of other colors with it set
            //if(i % 16 == 0) a = 0; // Force transparency on color 0

            //palette must be added both to input and output color mode, because in this
            //sample both the raw image and the expected PNG image use that palette.
            lodepng_palette_add(&state.info_png.color, r, g, b, a*255);
            lodepng_palette_add(&state.info_raw, r, g, b, a*255);
            pixOffset += 2;
        }
        state.info_png.color.colortype = LCT_PALETTE; //if you comment this line, and create the above palette in info_raw instead, then you get the same image in a RGBA PNG.
        state.info_png.color.bitdepth = 8;
        state.info_raw.colortype = LCT_PALETTE;
        state.info_raw.bitdepth = 8;
        state.encoder.auto_convert = 0; //we specify ourselves exactly what output PNG color mode we want
        
        unsigned w;
        unsigned h;

        if(!root.has_child("width") || !root.has_child("height")) {
            #ifdef VERBOSE
            std::cout << "No proper width and height provided for sheet " << sheetyml << ". assuming 256x256." << std::endl;
            #endif
            w = h = 256;
        } else {
            sheetymltree["width"] >> w;
            sheetymltree["height"] >> h;    
        }
        
        std::vector<unsigned char> image;
        image.resize((w * h), 0);

        int partCount = sheetymltree["parts"].num_children();
        for(int i = 0; i < partCount; i++) {
            int partx;
            int party;
            int partw;
            int parth; 
            int partp;
            sheetymltree["parts"][i]["w"] >> partw;
            sheetymltree["parts"][i]["h"] >> parth;
            sheetymltree["parts"][i]["x"] >> partx;
            sheetymltree["parts"][i]["y"] >> party;
            partp = 0;
            if(root["parts"][i].has_child("p")) sheetymltree["parts"][i]["p"] >> partp;
            #ifdef VERBOSE
            else std::cout << "No palette provided. assuming 0" << std::endl;
            #endif
            blitTileIntoImage(image, w, h, partx, party, pixFile.data()+pixOffset, partw, parth, partp);
            pixOffset += (partw*parth) / 2;
        }
        std::vector<unsigned char> buffer;
        lodepng::encode(buffer, image, w, h, state);
        std::string outfile;
        sheetymltree["gfx"] >> outfile;
        lodepng::save_file(buffer, ymlpath.parent_path() / outfile);
    }
    return 0;
}

void print_exception(const std::exception& e, int level =  0)
{
    std::cerr << std::string(level, ' ') << "exception: " << e.what() << '\n';
    try
    {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& nestedException)
    {
        print_exception(nestedException, level + 1);
    }
    catch (...) {}
}

int main(int argc, char ** argv)
{
    // this is completely overkill and a waste of space and time
    argparse::ArgumentParser parser("pwaanim");
    auto &flagGroup = parser.add_mutually_exclusive_group(true);
    flagGroup.add_argument("-x", "--dump").help("Dump Sheet PNGs from a .pix file and the corrosponding animation yaml").flag();
    flagGroup.add_argument("-c", "--create").help("Create the .seq, .pix, and header files from the animation yaml").flag();
    parser.add_argument("-pix").help(".pix file used for dumping PNGs");
    parser.add_argument("ymlpath").help("Path to animation yaml file");
    //parser.add_argument("-o", "--output").help("name of compiled animation files");
    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << std::endl;
        std::cerr << parser;
        return 1;
    }
    if(parser["-x"] == true) {// if dumping
        if(!parser.present("-pix")) {
            std::cerr << "Please provide a .pix file for dumping" << std::endl;
            return 1;
        }
        return dumpPNGFromPixAndYml(parser.get<std::string>("ymlpath"), parser.get<std::string>("-pix"));
    }
    if(parser["-c"] == true) {// if creating
        fs::path yamlfile = parser.get<std::string>("ymlpath");
        fs::path pixpath = yamlfile.parent_path() / yamlfile.stem().concat(".pix");
        fs::path seqpath = yamlfile.parent_path() / yamlfile.stem().concat(".seq");
        try {
            if(!fs::exists(yamlfile)) {
                std::throw_with_nested(std::runtime_error("source yaml file does not exist."));
            }
            AnimData animData = AnimData(yamlfile);
            Exporter *exporter = new GbaExporter(animData, seqpath, pixpath);
            exporter->exportAnimation();
            delete exporter;
        } catch (const std::exception& e) {
            std::cerr << "Couldn't convert animation " << yamlfile << std::endl;
            print_exception(e);
        }
        return 0;
        //return compileAnimYamlIntoPixSeq(parser.get<std::string>("ymlpath"));
    }
    return 1;
}