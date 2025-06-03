#ifndef INCLUDE_YAML_PARSER_H
#define INCLUDE_YAML_PARSER_H
#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace std;

struct InstConfig
{
    int opcode_num;
    int runtime_cycles;
    vector<pair<int, vector<int>>> ports;
    vector<vector<string>> fus;
};

struct Params
{
public:
    uint64_t robSize = 192;
    uint64_t issueQSize = 64;
    uint64_t ldqSize = 32;
    uint64_t stqSize = 32;
    int pipe_width = 4;

    map<uint64_t, InstConfig*> insnConfigs;
    map<int, vector<string>> portConfigs;
    vector<string> notPipelineFUs;
};

void loadConfig(const string &filename, Params* params);
#endif
