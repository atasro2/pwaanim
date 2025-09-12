#include <vector>
#include <string>
#include "pwaanim.hpp"
#include "animdata.hpp"
#include "sheet.hpp"

AnimData::AnimData(fs::path ymlPath) {
    this->ymlPath = ymlPath;
    ryml::Tree ymltree = readYamlFile(ymlPath);
    int sheetCount = ymltree["sheets"].num_children();
    for(int sheet = 0; sheet < sheetCount; sheet++) {
        std::string sheetyml;
        ymltree["sheets"][sheet] >> sheetyml;
        try {
            fs::path sheetymlpath(sheetyml);
            if(!fs::exists(ymlPath.parent_path() / sheetymlpath)) {
                std::throw_with_nested(std::runtime_error("sheet " + sheetyml + " does not exist."));
            }
            ryml::Tree sheetymltree = readYamlFile(ymlPath.parent_path() / sheetymlpath);
                sheets.push_back(Sheet(sheetymltree, sheetymlpath.stem()));
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed while parsing sheet " + sheetyml));
        }
    }
    int animCount = ymltree["anims"].num_children();
    for(int anim = 0; anim < animCount; anim++) {
        std::string animyml;
        ymltree["anims"][anim] >> animyml;
        try {
            fs::path animymlpath(animyml);
            if(!fs::exists(ymlPath.parent_path() / animymlpath)) {
                std::throw_with_nested(std::runtime_error("Anim " + animyml + " does not exist."));
            }
            ryml::Tree animymltree = readYamlFile(ymlPath.parent_path() / animymlpath);
            anims.push_back(Anim(animymltree, animymlpath.stem()));
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed while parsing anim " + animyml));
        }
    }
}

std::vector<Sheet> &AnimData::getSheets() {
    return sheets;
}

std::vector<Anim> &AnimData::getAnims() {
    return anims;
}
