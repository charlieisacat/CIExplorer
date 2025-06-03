#ifndef PA_MAPPING_H
#define PA_MAPPING_H
#include <list>
#include <map>
#include <iostream>

// um^2
// 1ns
static std::map<std::string, double> aladdin_area = {
    {"alloca", 2.779792e+02}, // adder
    {"add", 2.779792e+02}, // adder
    {"insertvalue", 2.779792e+02}, // adder
    {"br", 2.779792e+02},
    {"ret", 2.779792e+02},
    {"sub", 2.779792e+02},
    {"load", 2.779792e+02},
    {"store", 2.779792e+02},
    {"mul", 6.351338e+03}, // mul
    {"udiv", 6.351338e+03},
    {"sdiv", 6.351338e+03},
    {"urem", 6.351338e+03},
    {"srem", 6.351338e+03},
    {"shl", 2.496461e+02}, // shifter
    {"lshr", 2.496461e+02},
    {"ashr", 2.496461e+02},
    {"and", 5.036996e+01}, // bit
    {"or", 5.036996e+01},
    {"xor", 5.036996e+01},
    {"select", 5.036996e+01}, // bit
    {"icmp", 5.036996e+01},   // bit
    {"fcmp", 5.036996e+01},   // bit
    {"zext", 0},
    {"sext", 0},
    {"fptoui", 4.361094e+03}, // fp adder
    {"fptosi", 4.361094e+03},
    {"fpext", 4.361094e+03},
    {"ptrtoint", 0},
    {"inttoptr", 0},
    {"sitofp", 4.361094e+03},
    {"uitofp", 4.361094e+03},
    {"trunc", 0},
    {"fptrunc", 4.361094e+03},
    {"bitcast", 0},
    {"getelementptr", 2.779792e+02},
    {"fneg", 5.036996e+01}, // int bit
    {"fadd", 4.361094e+03}, // fp adder
    {"fsub", 4.361094e+03},
    {"fmul", 8.967113e+03}, // fp mul
    {"fdiv", 8.967113e+03},
    {"frem", 8.967113e+03}};

// pj
// static std::map<std::string, double> aladdin_energy = {
//     {"alloca", 2.104162e-01}, // adder
//     {"add", 2.104162e-01}, // adder
//     {"sub", 2.104162e-01},
//     {"mul", 1.268341e+01}, // mul
//     {"udiv", 1.268341e+01},
//     {"sdiv", 1.268341e+01},
//     {"urem", 1.268341e+01},
//     {"srem", 1.268341e+01},
//     {"shl", 4.172512e-01}, // shifter
//     {"lshr", 4.172512e-01},
//     {"ashr", 4.172512e-01},
//     {"and", 1.805590e-02}, // bit
//     {"or", 1.805590e-02},
//     {"xor", 1.805590e-02},
//     {"select", 1.805590e-02}, // bit
//     {"icmp", 1.805590e-02},   // bit
//     {"fcmp", 1.805590e-02},   // bit
//     {"zext", 0},
//     {"sext", 0},
//     {"fptoui", 1.667880e+01}, // fp adder
//     {"fptosi", 1.667880e+01},
//     {"fpext", 1.667880e+01},
//     {"ptrtoint", 0},
//     {"inttoptr", 0},
//     {"sitofp", 1.667880e+01},
//     {"uitofp", 1.667880e+01},
//     {"trunc", 0},
//     {"fptrunc", 1.667880e+01},
//     {"bitcast", 0},
//     {"getelementptr", 2.104162e-01},
//     {"fneg", 1.805590e-02}, // int bit
//     {"fadd", 1.667880e+01}, // fp adder
//     {"fsub", 1.667880e+01},
//     {"fmul", 1.177340e+01}, // fp mul
//     {"fdiv", 1.177340e+01},
//    {"frem", 1.177340e+01}};

// static std::map<std::string, double> aladdin_area = {
//     {"alloca", 63112.0}, // adder
//     {"add", 63112.0}, // adder
//     {"sub", 63112.0},
//     {"mul", 189336.0}, // mul
//     {"udiv", 189336.0},
//     {"sdiv", 189336.0},
//     {"urem", 189336.0},
//     {"srem", 189336.0},
//     {"shl", 63112.0}, // shifter
//     {"lshr", 63112.0},
//     {"ashr", 63112.0},
//     {"and", 63112.0}, // bit
//     {"or", 63112.0},
//     {"xor", 63112.0},
//     {"select", 63112.0}, // bit
//     {"icmp", 63112.0},   // bit
//     {"fcmp", 63112.0},   // bit
//     {"zext", 0},
//     {"sext", 0},
//     {"fptoui", 1673086.419753}, // fp adder
//     {"fptosi", 1673086.419753},
//     {"fpext", 1673086.419753},
//     {"ptrtoint", 0},
//     {"inttoptr", 0},
//     {"sitofp", 1673086.419753},
//     {"uitofp", 1673086.419753},
//     {"trunc", 0},
//     {"fptrunc", 1673086.419753},
//     {"bitcast", 0},
//     {"getelementptr", 63112.0},
//     {"fneg", 63112.0}, // int bit
//     {"fadd", 1673086.419753}, // fp adder
//     {"fsub", 1673086.419753},
//     {"fmul", 1673086.419753}, // fp mul
//     {"fdiv", 1673086.419753},
//     {"frem", 1673086.419753}};

static std::map<std::string, double> aladdin_energy = {
    {"load", 0.11813773833},
    {"store", 0.11813773833},
    {"alloca", 0.11813773833}, // adder
    {"add", 0.11813773833}, // adder
    {"insertvalue", 0.11813773833}, // adder
    {"br", 0.11813773833}, // adder
    {"ret", 0.11813773833}, // adder
    {"sub", 0.11813773833},
    {"mul", 0.23627547666}, // mul
    {"udiv", 0.23627547666},
    {"sdiv", 0.23627547666},
    {"urem", 0.23627547666},
    {"srem", 0.23627547666},
    {"shl", 0.11813773833}, // shifter
    {"lshr", 0.11813773833},
    {"ashr", 0.11813773833},
    {"and", 0.11813773833}, // bit
    {"or", 0.11813773833},
    {"xor", 0.11813773833},
    {"select", 0.11813773833}, // bit
    {"icmp", 0.11813773833},   // bit
    {"fcmp", 0.11813773833},   // bit
    {"zext", 0},
    {"sext", 0},
    {"fptoui", 0.35441321499}, // fp adder
    {"fptosi", 0.35441321499},
    {"fpext", 0.35441321499},
    {"ptrtoint", 0},
    {"inttoptr", 0},
    {"sitofp", 0.35441321499},
    {"uitofp", 0.35441321499},
    {"trunc", 0},
    {"fptrunc", 0.35441321499},
    {"bitcast", 0},
    {"getelementptr", 0.11813773833},
    {"fneg", 0.11813773833}, // int bit
    {"fadd", 0.35441321499}, // fp adder
    {"fsub", 0.35441321499},
    {"fmul", 0.35441321499}, // fp mul
    {"fdiv", 0.35441321499},
    {"frem", 0.35441321499}};  

static float getAladdinEnergyDyn(std::string opname)
{

    double ret = 0; // default value
    if (aladdin_energy.count(opname))
    {
        ret = aladdin_energy[opname];
    }
    else
        std::cout << "Missing Energy Value (" << opname << ")\n";
    return ret;
}
static int getAladdinArea(std::string opname)
{
    int ret = 0; // default value
    if (aladdin_area.count(opname))
        ret = aladdin_area[opname];
    else
        std::cout << "Missing Area Value (" << opname << ")\n";
    // std::cout << "opname = " << opname << " area = " << ret << "\n";
    return ret;
}
#endif