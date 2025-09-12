#pragma once
#include <vector>

#include "sheet.hpp"
#include "animdata.hpp"

class Exporter {
    protected:
    AnimData animData;
    public:
    virtual void exportAnimation() = 0;
    Exporter(AnimData animData);
};
