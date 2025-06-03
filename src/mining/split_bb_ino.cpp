#include "utils.h"
#include "graph.h"
#include "PatternManager.h"
#include "../cost_model/cost_model.h"
#include "../cost_model/packing.h"

#include <map>
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;

map<BasicBlock *, double> score_map;

#if 0
double score(PatternManager *pm, Graph *g, Module &m,
             LLVMContext &context, CostModel *cost_model, int dup)
{
    int cp_delay = int(std::floor(pm->compute_graph_cp_delay(g)));
    int seq_delay = int(std::floor(pm->compute_graph_seq_delay(g)));

    if(cp_delay == 0)
        return 0.;

    return 1.0*seq_delay / (1.0 * cp_delay);
}
#endif

#if 1

double score(PatternManager *pm, Graph *g, Module &m,
             LLVMContext &context, CostModel *cost_model, int dup, int pack_n)
{
    if (g->vertices.size() <= 1)
        return 0.;

    BasicBlock *orig_bb = pm->name_bb_map[g->bbname];
    TempBasicBlock *new_bb = copy_bb(orig_bb);

    vector<Instruction *> component_insns;
    for (int uid : g->vertices)
    {
        Instruction *insn = pm->uid_insn_map[uid];
        component_insns.push_back(new_bb->orig_to_copy[insn]);
    }

    if (component_insns.size() < 2)
        return 0.;

    int cp_delay = int(std::floor(pm->compute_graph_cp_delay(g)));
    int stage_cycles = cp_delay;

    customize({component_insns}, m, context, pack_n, {1.0 * cp_delay}, {stage_cycles});

    vector<Instruction *> orig_insns;
    for (Instruction &insn : *orig_bb)
    {
        orig_insns.push_back(&insn);
    }

    double score = 0.;
    if (!score_map.count(orig_bb))
    {
        // score_map[orig_bb] = cost_model->evaluate_o3(orig_insns, dup);
        score_map[orig_bb] = cost_model->evaluate_ino(orig_insns, dup);
    }
    score = score_map[orig_bb];

    vector<Instruction *> new_insns;
    for (Instruction &insn : *new_bb->bb)
    {
        new_insns.push_back(&insn);
    }
    // double new_score = cost_model->evaluate_o3(new_insns, dup);
    double new_score = cost_model->evaluate_ino(new_insns, dup);
    double speedup = score / new_score;
    // double speedup = g->vertices.size() * score / new_score;

    // cout << "g->itervalue=" << g->itervalue << " bbname=" << g->bbname << "\n";
    // cout << "score = " << score << " new_score=" << new_score << "\n";

    // if (speedup > 0.4)
    //     return speedup * g->itervalue / dup * g->vertices.size();

    // return 0.;

    return speedup;
}
#endif

Graph *combine_subgraph_helper(Graph *g1, Graph *g2, PatternManager *pm, Module &m,
                               LLVMContext &context, CostModel *cost_model, int dup, bool use_score, int pack_n)
{
    assert(g1->bbname == g2->bbname);

    Graph *merged_graph = new Graph();

    for (auto v : g1->vertices)
    {
        merged_graph->add_vertex(v);
    }

    for (auto v : g2->vertices)
    {
        merged_graph->add_vertex(v);
    }

    merged_graph->bbname = g1->bbname;
    merged_graph->itervalue = g1->itervalue;
    // merged_graph->score = 1.;

    if (use_score)
    {
        merged_graph->score = score(pm, merged_graph, m, context, cost_model, dup, pack_n);
    }

    // 8 4 is better?
    // vector<int> ion = pm->compute_io_number(merged_graph);
    // bool io_flag = (ion[0] <= 4) && (ion[1] <= 2);
    bool io_flag = true;

    // if (pm->dfs_check_convex(merged_graph) && merged_graph->score > 1. && (merged_graph->score > g1->score + g2->score))
    // if (pm->dfs_check_convex(merged_graph) && io_flag && merged_graph->score > (g1->score + g2->score) / 2)
    if (pm->dfs_check_convex(merged_graph) && io_flag)
    {
        if (use_score && merged_graph->score <= max(g1->score, g2->score))
        {
            return nullptr;
        }
        // cout << "merge = " << merged_graph->score << " g1=" << g1->score << " g2=" << g2->score << "\n";
        return merged_graph;
    }

    return nullptr;
}

vector<Graph *> combine_subgraphs(vector<Graph *> subgraphs, PatternManager *pm, Module &m,
                                  LLVMContext &context, CostModel *cost_model, int dup, int pack_n)
{
    std::sort(subgraphs.begin(), subgraphs.end(),
              [](Graph *a, Graph *b)
              { return a->vertices[0] < b->vertices[0]; });

    bool terminate = false;
    int N = subgraphs.size();
    vector<bool> visisted(N, 0);
    vector<Graph *> ret;

    // avoid cost of too much time
    // bool use_score = subgraphs.size() <= 24;
    bool use_score = false;

    for (int i = 0; i < N; i++)
    {
        if (visisted[i])
            continue;

        Graph *merged_graph = subgraphs[i];

        bool merge = true;
        while (merge)
        {
            merge = false;

            for (int j = i + 1; j < N; j++)
            {
                if (visisted[j])
                    continue;
                Graph *temp = combine_subgraph_helper(merged_graph, subgraphs[j], pm, m, context, cost_model, dup, use_score, pack_n);
                if (temp)
                {
                    // cout << "merge i=" << i << " j=" << j << "\n";
                    visisted[j] = 1;
                    merged_graph = temp;
                    merge = true;
                }
            }
        }

        visisted[i] = 1;
        merged_graph->score = score(pm, merged_graph, m, context, cost_model, 4, pack_n);
        ret.push_back(merged_graph);
    }

    return ret;
}

int main(int argc, char **argv)
{
    string ir_file = string(argv[1]);
    string bblist_path = string(argv[2]);
    string graph_path = string(argv[3]);
    std::string dynamic_info_file = std::string(argv[4]);
    int pack_n = stoi(argv[5]);
    string config_path = argv[6];

    vector<Graph *> graphs = read_graph_data(graph_path);

    vector<string> bblist;
    read_bblist(bblist_path, bblist);

    set<string> allow_bbs(bblist.begin(), bblist.end());

    map<string, long> iter_map;
    read_dyn_info(dynamic_info_file, &iter_map);
    bind_dyn_info(&iter_map, graphs);

    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);
    pm->init_graphs(graphs);

    CostModel *cost_model = new CostModel(config_path);
    pm->opcode_delay = cost_model->opcode_delay;

    vector<Graph *> seeds;
    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    string outdir = "split_bb_output";
    int pid = 0;
    for (Graph *g : graphs)
    {
        if (!allow_bbs.count(g->bbname))
        {
            continue;
        }

        Graph *clean_subgraph = pm->rm_graph_ops(ops, g);
        auto components = pm->find_connected_componet(clean_subgraph);
        cout << "size = " << components.size() << "\n";

        vector<Graph *> subgraphs;

        for (auto code : components)
        {
            Graph *subgraph = pm->build_subgraph(code, clean_subgraph);
            // subgraph->score = 1.;

            // if (!pm->dfs_check_convex(subgraph))
            //     continue;

            // vector<int> ion = pm->compute_io_number(subgraph);
            // bool io_flag = (ion[0] <= 4) && (ion[1] <= 2);
            bool io_flag = true;

            if (!io_flag)
                continue;

            // subgraph->score = 0.1; // ooo4

            subgraph->score = score(pm, subgraph, *m, context, cost_model, 4, pack_n);
            // if (subgraph->vertices.size() > 1 && subgraph->score > 0.2)
            if (subgraph->vertices.size() > 1)
            {
                // subgraph->score = 1.;
                // pm->dump_graph_data(outdir + "/" + to_string(pid) + "_splitbb.patt", {subgraph});
                // pid++;
                subgraphs.push_back(subgraph);
            }
            if (subgraph->vertices.size() == 1)
            {
                subgraphs.push_back(subgraph);
            }
        }

        auto merged_subgraphs = combine_subgraphs(subgraphs, pm, *m, context, cost_model, 4, pack_n);
        for (auto subgraph : merged_subgraphs)
        {
            if (subgraph->vertices.size() > 1 && subgraph->score > 0.9)
            // if (subgraph->vertices.size() > 1)
            {
                // not involved in mining
                // subgraph->score = 1.;
                pm->dump_graph_data(outdir + "/" + to_string(pid) + "_splitbb.patt", {subgraph});
                pid++;
            }
        }
    }

    return 0;
}