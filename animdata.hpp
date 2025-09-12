#pragma once
#include "pwaanim.hpp"
#include "sheet.hpp"
#include "anim.hpp"

class AnimData {
    private:
    std::vector<Sheet> sheets;
    std::vector<Anim> anims;
    public:
    fs::path ymlPath;
    
    AnimData() = default;
    AnimData(fs::path ymlPath);

    std::vector<Sheet> &getSheets();
    std::vector<Anim> &getAnims();
};