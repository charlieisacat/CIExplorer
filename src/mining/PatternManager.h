#ifndef PATTERN_MANAGER_H
#define PATTERN_MANAGER_H

#include <string>
#include <map>
#include <set>
#include <stack>
#include <vector>
#include <iostream>
#include "llvm.h"
#include "graph.h"
#include "maps.h"
#include "utils.h"

using namespace std;
using namespace llvm;

class PatternManager
{
public:
    PatternManager(string ir_file, Module &m);
    map<int, Instruction*> uid_insn_map;
    map<Instruction*, int> insn_uid_map;
    map<string, Graph*> bbname_graph_map;
    std::vector<Pattern *> patterns;
    std::vector<Graph *> graphs;
    bool force_connect = false;
    map<string, BasicBlock *> name_bb_map;

    Graph * rm_graph_ops(vector<string> ops, Graph *g);
    string rm_graph_ops_code(vector<string> ops, Graph *g);

    Graph * build_subgraph(string code, Graph *g);
    DependencyGraph build_data_dependency_graph(const vector<Instruction *> &instructions);
    Graph *sort_graph_vertices(Graph *g);


    DependencyGraph build_data_dependency_graph(Graph* graph);

    void dump_graph_data(string filename, vector<Graph *> graphs);
    void dump_graph_data(ofstream &f,  DependencyGraph &graph,  vector<Instruction *> &instructions, int gid, double score);

    void init_graphs(vector<Graph*> graphs);

    int get_proper_gid(Pattern patt);

    // cc : candidate component
    // bb : basic block
    string encode_candidate_component(Graph *cc, Graph *bb);

    bool get_convexity(string code, int pid, int gid);
    bool dfs_check_convex(Graph *graph);
    bool dfs_check_convex(set<Instruction *> insn_set, Instruction *insn);
    bool has_dependency_ending_in_graph(Instruction *inst, set<Instruction *> insn_set);

    void dump_pattern_data(vector<Graph *> graphs, int t, std::string filename, bool append = false);

    bool check_no_forbidden_insn(string code, int pid, int gid);

    void get_code_support(vector<string> &code_per_bb,
                          vector<int> &gids,
                          string code, int pid, int gid);

    map<Graph *, vector<int>> build_visit_vector(vector<Graph *> graphs);
    void compute_support(vector<string> &code_per_bb,
                         vector<int> &gids,
                         vector<Graph *> graphs,
                         vector<int> new_vertex_uids,
                         map<Graph *, vector<int>> &graph_visit_map, int gid);
    vector<int> compute_graph_similarity(
        Graph *cc,
        Graph *bb,
        vector<int> cc_vertices,
        vector<int> new_vertex_uids,
        vector<int> &vis);

    vector<int> get_io_number(string code, int pid, int gid);
    vector<int> compute_io_number(Graph *graph);

    double get_graph_cp_delay(string code, int pid, int gid);
    double get_graph_seq_delay(string code, int pid, int gid);
    double compute_graph_cp_delay(Graph* graph);
    double compute_graph_seq_delay(Graph* graph);

    bool check_ops_unkown_type(string code, int pid, int gid);

    int get_stage_delay(string code, int pid, int gid);

    double find_critical_path(DependencyGraph &graph, vector<llvm::Instruction *> &instructions);

    int check_dep(std::vector<int> vertex_uids,
                  Graph *graph,
                  int new_uid);
    bool contains(std::string s1, std::string s2);

    int compute_graph_width(Graph *graph);
    int get_graph_width(string code, int pid, int gid);
    int find_graph_width(DependencyGraph &graph, vector<llvm::Instruction *> &instructions);

    void mia(vector<Graph*> graphs, vector<string> bblist);
    int **build_conflict_graph(map<string, vector<Graph *>> bb_graphs_map);
    bool check_graph_overlap(Graph *g1, Graph *g2);

    double score(Pattern* pattern);
    double score(Graph *graph);
    vector<int> get_max_score_indep_set(vector<vector<int>> indep_sets);

    // forbidden ops
    vector<string> ops = {"input", "output", "phi",
                          "store",
                          "load",
                          "ret",
                          "br", "alloca", "call", "invoke", "landingpad"};

    void dump_dfg(Graph *g, ofstream &file);
    double get_cp_ops(uint64_t uid, Graph* g, vector<uint64_t> &cp_ops, DependencyGraph dep_graph);

    vector<int> get_cc_uids(string code, Graph* bb);

    map<int, double> opcode_delay;
    double get_hw_delay(int opcode)
    {
        assert(opcode_delay.count(opcode));
        return opcode_delay[opcode];
    }

    vector<string> find_connected_componet(Graph* g);
    int **get_adj_matrix(Graph* g);
    void dfs_conn(int v, vector<int> &visited, int **adjMat, vector<int> &subgraph);
};

void remove_illegal_patts(vector<Pattern*> &patts);
void BK(int d, int an, int sn, int nn, int **some, int **none, int **all, int **mp, vector<vector<int>> &indep_sets, int N);

#endif