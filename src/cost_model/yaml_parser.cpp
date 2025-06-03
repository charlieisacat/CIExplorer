#include "yaml_parser.h"
#include "yaml-cpp/yaml.h"
#include <regex>
#include <assert.h>
#include <iostream>


vector<string> parseFUs(string fus)
{
    regex fuPattern("\\+");

    sregex_token_iterator iter(fus.begin(), fus.end(), fuPattern, -1);
    sregex_token_iterator end;

    vector<string> tokens;
    while (iter != end)
    {
        tokens.push_back(*iter);
        iter++;
    }

    return tokens;
}

vector<int> parsePorts(const string ports_str, int offset = 0)
{
    vector<int> result;
    for (char ch : ports_str)
    {
        if (isdigit(ch))
        {
            result.push_back(ch - '0' + offset);
        }
        else
        {
            cerr << "Error: Non-digit character encountered in ports: " << ch << endl;
        }
    }
    return result;
}


void loadConfig(const string &filename, Params* params)
{
    YAML::Node config = YAML::LoadFile(filename);

    const auto &cpuNode = config["CPU"];
    params->pipe_width = cpuNode["window_size"].as<uint64_t>();
    params->robSize = cpuNode["rob_size"].as<uint64_t>();
    params->issueQSize = cpuNode["issueq_size"].as<int>();
    params->ldqSize = cpuNode["ldq_size"].as<uint64_t>();
    params->stqSize = cpuNode["stq_size"].as<int>();

    if (config["not_pipeline_fu"] && config["not_pipeline_fu"].IsSequence())
    {
        vector<string> names = config["not_pipeline_fu"].as<vector<string>>();

        for (const auto &name : names)
        {
            params->notPipelineFUs.push_back(name);
        }
    }

    // parse instructions
    for (const auto &node : config["instructions"])
    {
        InstConfig* inst = new InstConfig();

        // parse port mapping
        string portMapping = node.second["ports"].as<string>();

        regex pattern(R"((\d+)\*(\d+))"); // Pattern to match "multiplier*value"
        smatch matches;
        // a flag to indicate if ports in uops.info format
        bool flag = false;

        while (regex_search(portMapping, matches, pattern))
        {
            flag = true;
            int num = stoi(matches[1].str());
            auto ports = parsePorts(matches[2].str());
            inst->ports.push_back({num, ports});

            portMapping = matches.suffix().str();
        }

        if (!flag)
        {
            auto ports = parsePorts(node.second["ports"].as<string>());
            inst->ports.push_back({1, ports});
        }

        vector<vector<string>> tokens;

        // parse fu
        if (node.second["fu"].IsSequence())
        {
            // Iterate over the sequence and push into the vector
            for (const auto &fuStr : node.second["fu"])
            {
                auto fus = parseFUs(fuStr.as<string>());
                tokens.push_back(fus);
            }
        }
        else
        {
            vector<string> fus = parseFUs(node.second["fu"].as<string>());
            tokens.push_back(fus);
        }

        inst->opcode_num = node.second["opcode_num"].as<int>();
        inst->runtime_cycles = node.second["runtime_cycles"].as<int>();
        inst->fus = tokens;
        params->insnConfigs[inst->opcode_num] = inst;
    }

    // parse ports
    int portID = 0;
    for (const auto &port : config["ports"])
    {
        vector<string> fuTypes;
        for (const auto &fu : port.second)
        {
            fuTypes.push_back(fu.second.as<string>());
        }
        params->portConfigs[portID] = fuTypes;
        portID++;
    }
}
