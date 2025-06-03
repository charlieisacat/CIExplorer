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
        score_map[orig_bb] = cost_model->evaluate_o3(orig_insns, dup);
    }
    score = score_map[orig_bb];

    vector<Instruction *> new_insns;
    for (Instruction &insn : *new_bb->bb)
    {
        new_insns.push_back(&insn);
    }
    double new_score = cost_model->evaluate_o3(new_insns, dup);
    double speedup = score / new_score;
    return speedup;
}
#endif

Graph *combine_subgraph_helper(Graph *g1, Graph *g2, PatternManager *pm, Module &m,
                               LLVMContext &context, CostModel *cost_model, int dup, int nin, int nout, int pack_n)
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

    merged_graph->score = score(pm, merged_graph, m, context, cost_model, dup, pack_n);

#if 0
    // 8 4 is better?
    vector<int> ion = pm->compute_io_number(merged_graph);
    bool io_flag = (ion[0] <= nin) && (ion[1] <= nout);

    if (!io_flag)
        return nullptr;
#endif

    bool io_flag = true;

    if (io_flag && pm->dfs_check_convex(merged_graph) && (merged_graph->score > g1->score && merged_graph->score > g2->score) && merged_graph->score > 1.1)
    // if (pm->dfs_check_convex(merged_graph) && io_flag)
    {
        // cout << "merge = " << merged_graph->score << " g1=" << g1->score << " g2=" << g2->score << "\n";
        return merged_graph;
    }

    return nullptr;
}

vector<Graph *> combine_subgraphs(vector<Graph *> subgraphs, PatternManager *pm, Module &m,
                                  LLVMContext &context, CostModel *cost_model, int dup, int nin, int nout, int pack_n)
{
    std::sort(subgraphs.begin(), subgraphs.end(),
              [](Graph *a, Graph *b)
              { return a->vertices.size() > b->vertices.size(); });

    bool terminate = false;
    int N = subgraphs.size();
    vector<bool> visisted(N, 0);
    vector<Graph *> ret;

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
                Graph *temp = combine_subgraph_helper(merged_graph, subgraphs[j], pm, m, context, cost_model, dup, nin, nout, pack_n);
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
        // merged_graph->score = score(pm, merged_graph, m, context, cost_model, 4);
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
    std::string config_file = std::string(argv[5]);
    int nin = stoi(argv[6]);
    int nout = stoi(argv[7]);
    int pack_n = stoi(argv[8]);

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

    std::cout << "config_file=" << config_file << "\n";
    CostModel *cost_model = new CostModel(config_file);
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
            // cout << "name=" << g->bbname << "\n";
            continue;
        }

        Graph *clean_subgraph = pm->rm_graph_ops(ops, g);
        auto components = pm->find_connected_componet(clean_subgraph);

        vector<Graph *> subgraphs;

        for (auto code : components)
        {
            Graph *subgraph = pm->build_subgraph(code, clean_subgraph);
            // subgraph->score = 1.;

            if (!pm->dfs_check_convex(subgraph))
                continue;

#if 0
            vector<int> ion = pm->compute_io_number(subgraph);
            // bool io_flag = (ion[0] <= 8) && (ion[1] <= 4);
            bool io_flag = true;

            if (!io_flag)
                continue;
#endif

            subgraph->score = 0.1; // ooo4

            subgraph->score = score(pm, subgraph, *m, context, cost_model, 4, pack_n);
            // if (subgraph->vertices.size() > 1 && subgraph->score > 0.2)
            if (subgraph->vertices.size() > 1)
            {
                // subgraph->score = 1.;
                // pm->dump_graph_data(outdir + "/" + to_string(pid) + "_splitbb.patt", {subgraph});
                // pid++;
                subgraphs.push_back(subgraph);
            }
            // if (subgraph->vertices.size() == 1)
            // {
            // subgraphs.push_back(subgraph);
            // }
        }

        // combination can be slow, and just use split results
        if (subgraphs.size() >= 24)
        {
            for (auto subgraph : subgraphs)
            {
#if 0
                vector<int> ion = pm->compute_io_number(subgraph);
                bool io_flag = (ion[0] <= nin) && (ion[1] <= nout);
                if (io_flag && subgraph->vertices.size() > 1 && subgraph->score > 1.1)
#endif
                if (subgraph->vertices.size() > 1 && subgraph->score > 1.1)
                {
                    pm->dump_graph_data(outdir + "/" + to_string(pid) + "_splitbb.patt", {subgraph});
                    pid++;
                }
            }

            continue;
        }

        auto merged_subgraphs = combine_subgraphs(subgraphs, pm, *m, context, cost_model, 4, nin, nout, pack_n);
        for (auto subgraph : merged_subgraphs)
        {
#if 0
            vector<int> ion = pm->compute_io_number(subgraph);
            bool io_flag = (ion[0] <= nin) && (ion[1] <= nout);
            if (subgraph->vertices.size() > 1 && io_flag)
#endif
            if (subgraph->vertices.size() > 1)
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