#include "utils.h"
#include "graph.h"
#include "PatternManager.h"
#include "../cost_model/cost_model.h"

#include <map>
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;

void set_levels(PatternManager *pm, Graph *graph, DependencyGraph dep_graph, map<int, int> &uid_lvl_map, int uid, int level)
{
    if (level > uid_lvl_map[uid])
    {
        uid_lvl_map[uid] = level;
    }

    Instruction *insn = pm->uid_insn_map[uid];

    for (auto *dependent : dep_graph[insn])
    {
        int dep_uid = pm->insn_uid_map[dependent];
        set_levels(pm, graph, dep_graph, uid_lvl_map, dep_uid, level + 1);
    }
}

void group(PatternManager *pm, Graph *graph, map<int, int> &uid_lvl_map)
{
    std::vector<Instruction *> instructions;
    map<llvm::Instruction *, int> in_degree;

    for (int j = 0; j < graph->vertices.size(); j++)
    {
        int uid = graph->vertices[j];
        Instruction *insn = pm->uid_insn_map[uid];

        instructions.push_back(insn);
    }
    DependencyGraph dep_graph = pm->build_data_dependency_graph(instructions);

    for (auto insn : instructions)
    {
        in_degree[insn] = 0;
    }

    for (auto insn : instructions)
    {
        auto dependents = dep_graph[insn];
        for (auto *dependent : dependents)
        {
            in_degree[dependent]++;
        }
    }

    for (auto insn : instructions)
    {
        int degree = in_degree[insn];
        if (degree == 0)
        {
            int uid = pm->insn_uid_map[insn];
            uid_lvl_map[uid] = 0;
            set_levels(pm, graph, dep_graph, uid_lvl_map, uid, 0);
        }
    }
}

vector<Graph *> find_parallel_insns_pair(PatternManager *pm, Graph *g, int threshold, map<int, int> uid_lvl_map)
{
    set<int> visited;
    set<int> visited_lvl;

    vector<Graph *> ret;

    if (threshold == 0)
    {
        // max number
        threshold = g->vertices.size() / 2;
    }

    int n = 0;
    for (auto uid : g->vertices)
    {
        if (visited.count(uid))
            continue;

        int lvl = uid_lvl_map[uid];
        if (visited_lvl.count(uid))
            continue;

        Instruction *insn = pm->uid_insn_map[uid];
        double delay = pm->get_hw_delay(insn->getOpcode());

        double max_pair_delay = 0.;
        int max_delay_pair_uid = -1;
        for (auto pair_uid : g->vertices)
        {
            if (lvl != uid_lvl_map[pair_uid])
                continue;

            if (pair_uid == uid)
                continue;
            if (visited.count(pair_uid))
                continue;

            Instruction *pair_insn = pm->uid_insn_map[pair_uid];
            double pair_delay = pm->get_hw_delay(pair_insn->getOpcode());
            if (pair_delay > max_pair_delay)
            {
                max_pair_delay = pair_delay;
                max_delay_pair_uid = pair_uid;
            }
        }

        if (max_delay_pair_uid != -1)
        {
            visited.insert(uid);
            visited.insert(max_delay_pair_uid);

            // one pair in each lvl
            visited_lvl.insert(lvl);

            Graph *temp_g = new Graph();
            temp_g->add_vertex(uid);
            temp_g->add_vertex(max_delay_pair_uid);
            ret.push_back(temp_g);
            temp_g->bbname = g->bbname;

            n++;
        }

        if (n >= threshold)
            break;
    }

    return ret;
}

vector<Graph *> find_max_delay_insns_pair(PatternManager *pm, Graph *g, int threshold = 1)
{
    DependencyGraph dep_graph = pm->build_data_dependency_graph(g);
    set<int> visited;

    vector<Graph *> ret;

    if (threshold == 0)
    {
        // max number
        threshold = g->vertices.size() / 2;
    }

    int n = 0;
    for (auto uid : g->vertices)
    {
        if (visited.count(uid))
            continue;
        Instruction *insn = pm->uid_insn_map[uid];
        double delay = pm->get_hw_delay(insn->getOpcode());

        double max_dep_delay = 0.;
        int max_delay_dep_uid = -1;
        for (auto *dependent : dep_graph[insn])
        {
            uint64_t dep_uid = pm->insn_uid_map[dependent];
            if (visited.count(dep_uid))
                continue;
            Instruction *dep_insn = pm->uid_insn_map[uid];
            double dep_delay = pm->get_hw_delay(dep_insn->getOpcode());
            if (dep_delay > max_dep_delay)
            {
                max_dep_delay = dep_delay;
                max_delay_dep_uid = dep_uid;
            }
        }

        if (max_delay_dep_uid != -1)
        {
            visited.insert(uid);
            visited.insert(max_delay_dep_uid);

            Graph *temp_g = new Graph();
            temp_g->add_vertex(uid);
            temp_g->add_vertex(max_delay_dep_uid);
            ret.push_back(temp_g);
            temp_g->bbname = g->bbname;

            n++;
        }

        if (n >= threshold)
            break;
    }

    return ret;
}

int main(int argc, char **argv)
{
    string ir_file = string(argv[1]);
    string bblist_path = string(argv[2]);
    string graph_path = string(argv[3]);
    int threshold = stoi(argv[4]);
    string config_file = string(argv[5]);

    vector<Graph *> graphs = read_graph_data(graph_path);

    vector<string> bblist;
    read_bblist(bblist_path, bblist);

    set<string> allow_bbs(bblist.begin(), bblist.end());

    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);
    pm->init_graphs(graphs);

    CostModel *cost_model = new CostModel(config_file);
    pm->opcode_delay = cost_model->opcode_delay;

    vector<Graph *> seeds;
    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    map<int, int> uid_lvl_map;

    for (auto graph : graphs)
    {
        group(pm, graph, uid_lvl_map);
    }

    for (Graph *g : graphs)
    {
        if (!allow_bbs.count(g->bbname))
        {
            // cout << "name=" << g->bbname << "\n";
            continue;
        }

        Graph *clean_subgraph = pm->rm_graph_ops(ops, g);
        auto components = pm->find_connected_componet(clean_subgraph);

        for (auto code : components)
        {
            Graph *c = pm->build_subgraph(code, clean_subgraph);
            Pattern ret = find_max_delay_insns_pair(pm, c, threshold);
            seeds.insert(seeds.end(), ret.begin(), ret.end());

            if (ret.size() == 0)
            {
                ret = find_parallel_insns_pair(pm, c, threshold, uid_lvl_map);
                // cout << "ret.size=" << ret.size() << "\n";
                seeds.insert(seeds.end(), ret.begin(), ret.end());
            }
        }
    }

    ofstream f;
    f.open("seeds.data");

    int t = 0;
    for (auto seed : seeds)
    {
        int id0 = seed->vertices[0];
        int id1 = seed->vertices[1];
        // cout << "size=" << seed->vertices.size() << "\n";
        // cout << "t # " << t << "\n";
        // cout << "p # 0\n";
        f << "t # " << t << "\n";
        f << "p # 0\n";

        auto insn0 = pm->uid_insn_map[id0];
        auto insn1 = pm->uid_insn_map[id1];

        assert(pm->uid_insn_map.count(id1));

        // cout << "id0=" << id0 << " " << insn_to_string(insn0) << "\n";
        // cout << "id1=" << id1 << " " << insn_to_string(insn1)<<"\n";

        // cout << "a 0 1 module func " << seed->bbname << " " << insn0->getOpcodeName() << " " << insn1->getOpcodeName() << " " << id0 << " " << id1 << "\n";
        f << "a 0 1 module func " << seed->bbname << " " << insn0->getOpcodeName() << " " << insn1->getOpcodeName() << " " << id0 << " " << id1 << "\n";

        t++;
    }
    f.close();

    return 0;
}