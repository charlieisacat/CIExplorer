#ifndef MAPPING_H
#define MAPPING_H
#include <list>
#include <map>
#include <iostream>

// Weight Delay model taken from Region Seeker - Giorgios et al. in nS
static std::map<std::string, double> hwdelay = {
    {"br", 0.0001},
    {"add", 1.514},
    {"sub", 1.514},
    {"mul", 6.282},
    {"udiv", 74.69},
    {"sdiv", 74.69},
    {"urem", 74.69},
    {"srem", 74.69},
    {"shl", 1.89},
    {"lshr", 1.89},
    {"ashr", 1.89},
    {"and", 0.521},
    {"or", 0.521},
    {"xor", 0.521},
    {"select", 0.87},
    {"icmp", 1.26},
    {"fcmp", 12.25},
    {"zext", 0},
    {"sext", 0},
    {"fptoui", 3.693},
    {"fptosi", 6.077},
    {"fpext", 10.315},
    {"ptrtoint", 0.01},
    {"inttoptr", 0.01},
    {"sitofp", 39.785},
    {"uitofp", 39.785},
    {"trunc", 0},
    {"fptrunc", 12.895},
    {"bitcast", 0},
    {"load", 0},
    {"store", 0},
    {"getelementptr", 1.185},
    {"alloca", 1},
    {"phi", 0},
    {"call", 1},
    {"invoke", 1},
    {"switch", 0.23},
    {"ret", 1},
    {"fneg", 0.92},
    {"fadd", 53.57},  // HLS
    {"fsub", 53.57},  // HLS
    {"fmul", 44.36},  // HLS
    {"fdiv", 185},    // HLS
    {"frem", 725.76}, // HLS more than 1 cycle (35)
    {"shufflevector", 1},
    {"extractelement", 1},
    {"extractvalue", 1},
    {"insertelement", 1},
    {"addrspacecast", 1},
    {"output", 0},
    {"input", 0},
    {"insertvalue", 1}};

// Weight model taken from Region Seeker by Gioergios et al. in uM^2
static std::map<std::string, double> area = {
    {"br", 0},
    {"add", 39},    // HLS
    {"sub", 39},    // HLS
    {"mul", 1042},  // HLS
    {"udiv", 6090}, // HLS more than 1 cycle (35)
    {"sdiv", 6090}, // HLS more than 1 cvcle (35)
    {"urem", 6090}, // HLS more than 1 cycle (35)
    {"srem", 6090}, // HLS more than 1 cycle (35)
    {"shl", 85},    // HLS
    {"lshr", 85},   // HLS
    {"ashr", 85},   // HLS
    {"and", 32},    // HLS
    {"or", 32},     // HLS
    {"xor", 32},    // HLS
    {"select", 32}, // HLS
    {"icmp", 18},   // HLS
    {"fcmp", 68},   // HLS
    {"zext", 0},    // HLS
    {"sext", 1},
    {"fptoui", 558}, // HLS
    {"fptosi", 629}, // HLS
    {"fpext", 76},
    {"ptrtoint", 1},
    {"inttoptr", 1},
    {"sitofp", 629},
    {"uitofp", 558},
    {"trunc", 0}, // HLS
    {"fptrunc", 115},
    {"bitcast", 0},
    {"load", 0},
    {"store", 0},
    {"getelementptr", 15}, // HLS just vector indexing
    {"alloca", 1},
    {"phi", 1},
    {"call", 1},
    {"invoke", 1},
    {"switch", 1},
    {"ret", 1},
    {"fneg", 40},
    {"fadd", 375}, // HLS
    {"fsub", 375}, // HLS
    {"fmul", 678}, // HLS
    {"fdiv", 3608},
    {"frem", 10716}, // HLS more than 1 cycle (2)
    {"shufflevector", 1},
    {"extractelement", 1},
    {"extractvalue", 1},
    {"insertelement", 1},
    {"output", 0},
    {"input", 0},
    {"insertvalue", 1}};

// Energy model In nJ callibrated with Power8/9 measurements
static std::map<std::string, double> energy = {
    {"br", 0.5},
    {"add", 40.6},
    {"sub", 40.6},
    {"mul", 49.2},
    {"udiv", 482.3},
    {"sdiv", 489.3},
    {"urem", 601.9},
    {"srem", 601.9},
    {"shl", 42.5},
    {"lshr", 46.9},
    {"ashr", 61.5},
    {"and", 38.7},
    {"or", 39.1},
    {"xor", 39.9},
    {"select", 48.5},
    {"icmp", 58.8},
    {"fcmp", 58.4},
    {"zext", 20},
    {"sext", 20},
    {"fptoui", 44.0},
    {"fptosi", 44.0},
    {"fpext", 44.0},
    {"ptrtoint", 0.5},
    {"inttoptr", 0.5},
    {"sitofp", 44.0},
    {"uitofp", 44.0},
    {"trunc", 20},
    {"fptrunc", 20},
    {"bitcast", 0.5},
    {"load", 75.1},
    {"store", 93.1},
    {"getelementptr", 40.6},
    {"alloca", 40.6},
    {"phi", 0.5},
    {"call", 0.5},
    {"invoke", 1},
    {"switch", 0.5},
    {"ret", 0.5},
    {"fneg", 25},
    {"fadd", 50.4},
    {"fsub", 50.4},
    {"fmul", 50.4},
    {"fdiv", 968.4},
    {"frem", 1069.2},
    {"shufflevector", 968.4},
    {"extractelement", 1069.2},
    {"extractvalue", 1069.2},
    {"insertelement", 50},
    {"output", 0},
    {"input", 0},
    {"insertvalue", 40.6}};

// Static power in nwatts not callibrated
static std::map<std::string, double> powersta = {
    {"br", 0},
    {"add", 0.5},
    {"sub", 0.5},
    {"mul", 0.15},
    {"udiv", 0.3},
    {"sdiv", 0.3},
    {"urem", 0.35},
    {"srem", 0.35},
    {"shl", 0.1},
    {"lshr", 0.1},
    {"ashr", 0.1},
    {"and", 0.2},
    {"or", 0.2},
    {"xor", 0.2},
    {"select", 0.1},
    {"icmp", 0.5},
    {"fcmp", 0.5},
    {"zext", 0.1},
    {"sext", 0.1},
    {"fptoui", 0.1},
    {"fptosi", 0.1},
    {"fpext", 0.1},
    {"ptrtoint", 0.1},
    {"inttoptr", 0.1},
    {"sitofp", 0.1},
    {"uitofp", 0.1},
    {"trunc", 0.1},
    {"fptrunc", 0.1},
    {"bitcast", 0.1},
    {"load", 0.5},
    {"store", 0.2},
    {"getelementptr", 0.2},
    {"alloca", 0},
    {"phi", 0.3},
    {"call", 0},
    {"invoke", 0},
    {"switch", 0.2},
    {"ret", 0},
    {"fneg", 0.07},
    {"fadd", 0.15},
    {"fsub", 0.15},
    {"fmul", 0.4},
    {"fdiv", 0.7},
    {"frem", 0.8},
    {"shufflevector", 0.7},
    {"extractelement", 0.8},
    {"extractvalue", 0.8},
    {"insertelement", 0.7},
    {"output", 0},
    {"input", 0},
    {"insertvalue", 0.2}};

static double getHwDelay(std::string opname)
{
    double ret = 0; // default value
    if (hwdelay.count(opname))
    {
        // ret = hwdelay[I->getOpcode()] / 1000000000; // from ns to seconds
        ret = hwdelay[opname]; // ensure it is integer
    }
    else
        std::cout << "Missing Delay Value (" << opname << ")\n";
    return ret;
}

static float getEnergyDyn(std::string opname)
{

    double ret = 0; // default value
    if (energy.count(opname))
    {
        ret = energy[opname] - 28;
        if (ret < 0)
            ret = 0.5;
    }
    else
        std::cout << "Missing Energy Value (" << opname << ")\n";
    return ret;
}

static float getPowerSta(std::string opname)
{
    float ret = 0; // default value
    if (powersta.count(opname))
        ret = powersta[opname];
    else
        std::cout << "Missing Energy Value (" << opname << ")\n";
    return ret;
}

static int getArea(std::string opname)
{
    int ret = 0; // default value
    if (area.count(opname))
        ret = area[opname];
    else
        std::cout << "Missing Area Value (" << opname << ")\n";
    return ret;
}

static int getPower(std::string opname)
{
    int ret = getPowerSta(opname) + getEnergyDyn(opname);
    return ret;
}

#endif