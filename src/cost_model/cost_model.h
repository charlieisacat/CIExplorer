#ifndef COST_MODEL_H
#define COST_MODEL_H

#include <vector>
#include <map>

#include "../mining/utils.h"
#include "../mining/llvm.h"
#include "yaml_parser.h"

using namespace llvm;
using namespace std;

class ArchNode
{
public:
    ArchNode(int id, Instruction *insn, string name) : uid(id), insn(insn), name(name) {}
    int uid;
    string name = "";
    Instruction *insn;
    uint64_t duration = 2; // in cycle now
    uint64_t start_cycle = 0;

    // for fully pipelined fu this is 1
    // for custom fu it is the longest delay of op on critial path
    uint64_t stage_cycle = 1;

    int live_ins = 0;
    int live_outs = 1;

    vector<ArchNode *> out_edge_nodes;

    vector<string> found_ports;
    vector<string> found_fus;
    vector<int> found_port_cycles;

    // earliest time an alternative is avaliable
    uint64_t found_min_cycle = UINT_MAX;

    int get_port_cycle(string port);

    uint64_t retire_cycle = 0;
};

class ArchGraph
{
public:
    vector<ArchNode *> nodes;
    vector<Instruction *> insns;
    void add_nodes(int id, Instruction *insn);

    int pipe_width = 1;

    // resource management
    map<string, vector<string>> resource_names;
    map<string, ArchNode *> resource_use_node;
    map<string, vector<string>> port_fus;
    map<string, string> fu_port_map;

    int read_ports = 16;
    int write_ports = 16;
    int rdwt_ports = 16;

    map<string, int> port_delay_map;
    map<string, int> fu_delay_map;

    void add_fetch_dec_node(int id, Instruction *insn);
    void add_dp_node(int id, Instruction *insn);
    void add_port_node(int id, Instruction *insn);
    void add_rr_node(int id, Instruction *insn);
    void add_issue_node(int id, Instruction *insn);
    void add_cp_node(int id, Instruction *insn);
    void add_commit_node(int id, Instruction *insn);

    map<int, ArchNode *> insn_fetch_node;
    ArchNode *get_fetch_node(int uid) { return insn_fetch_node[uid]; }

    map<int, ArchNode *> insn_dp_node;
    ArchNode *get_dp_node(int uid) { return insn_dp_node[uid]; }

    map<int, ArchNode *> insn_port_node;
    ArchNode *get_port_node(int uid) { return insn_port_node[uid]; }
    
    map<int, ArchNode *> insn_rr_node;
    ArchNode *get_rr_node(int uid) { return insn_rr_node[uid]; }

    map<int, ArchNode *> insn_issue_node;
    ArchNode *get_issue_node(int uid)
    {
        if (insn_issue_node.find(uid) == insn_issue_node.end())
            return nullptr;
        return insn_issue_node[uid];
    }

    map<int, ArchNode *> insn_cp_node;
    ArchNode *get_cp_node(int uid) { return insn_cp_node[uid]; }

    map<int, ArchNode *> insn_commit_node;
    ArchNode *get_commit_node(int uid) { return insn_commit_node[uid]; }

    void visualize(ofstream &file);

    void build_data_dependency(int id, ArchNode *node);

    void read_port_contention(ArchNode *node);
    string round_robin_find_min_usage_res(string resource, set<string> ban = {});

    void write_port_contention(ArchNode *node);

    uint64_t test_issue_cycle(Instruction *insn, int id);
    int get_earliest_issue_insn(vector<Instruction *> insns, vector<int> ids);

    map<uint64_t, InstConfig *> insn_config;

    int bb_size = 0;

    // NOT USED
    void issue_port_contention(ArchNode *node);

    void find_min_port_fu(Instruction *insn,
                          vector<string> &found_ports,
                          vector<string> &found_fus,
                          uint64_t &found_min_cycle,
                          vector<int> &found_port_cycles);

    uint64_t critial_path = 0;

    // in our o3 model, nodes of different insns are creates out of order
    // so we need another function to build edges between commit nodes
    void update_commit_edges(vector<int> ids);

    map<Instruction*, int> insn_uid_map;

    map<int, vector<int>> data_deps;
    void build_data_deps(int id, Instruction *insn);

    void find_rob_entry(string &found, uint64_t &found_cycle);
    void find_issueq_entry(string &found, uint64_t &found_cycle);

    // load, store queue
    void find_ldq_entry(string &found, uint64_t &found_cycle);
    void find_stq_entry(string &found, uint64_t &found_cycle);

    int get_reg_reads_count(ArchNode *node);

    // which insn uses data of a reg
    map<int, ArchNode*> reg_node_map;

    vector<string> not_pipeline_fus;

    void compute_duration(ArchNode *node, Instruction *insn);
};

class CostModel
{
public:
    CostModel(string config_file);
    double evaluate_ino(vector<Instruction *> insns, int N);
    double evaluate_o3(vector<Instruction *> insns, int N);

    void build_rdwt_ports();
    void build_rob();
    void build_issueq();
    void build_ldstq();

    string config_file = "";
    void build_issue_ports();

    // avaliable fu count
    vector<int> fu_aval_count;
    // fu is used by which instructions (uid)
    map<int, vector<Instruction *>> fu_usage;

    // int pipe_width = 1;
    // int pipe_width = 2;
    int pipe_width = 4;

    int read_ports = 16;
    int write_ports = 16;
    int rdwt_ports = 16;
    
    int rob_size = 192;
    int issueq_size = 192;
    
    // int rob_size = 128;
    // int issueq_size = 128;
    
    // int rob_size = 64;
    // int issueq_size = 1;

    int ldq_size = 32;
    int stq_size = 32;

    // resource management
    map<string, vector<string>> resource_names;
    map<string, ArchNode *> resource_use_node;
    map<string, vector<string>> port_fus;
    map<string, string> fu_port_map;

    map<uint64_t, InstConfig *> insn_config;

    map<int, double> opcode_delay;

    vector<string> not_pipeline_fus;
};

#endif