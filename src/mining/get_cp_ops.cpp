#include "utils.h"
#include "graph.h"
#include "PatternManager.h"
#include "../cost_model/cost_model.h"
#include "../galgo/Galgo.hpp"

#include <map>
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;

int main(int argc, char **argv)
{
    string ir_file = string(argv[1]);
    string graph_dir = string(argv[2]);
    string config_file = string(argv[3]);

    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);

    CostModel *cost_model = new CostModel(config_file);
    pm->opcode_delay = cost_model->opcode_delay;

    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    for (const auto &entry : fs::directory_iterator(graph_dir))
    {
        string graph_file = entry.path();
        if (graph_file.find(".data") == std::string::npos)
        {
            continue;
        }

        string data_file_name = split_str(graph_file, '/', -1);

        auto graphs = read_graph_data(graph_file);

        string cp_ops_file = "./cp_ops_data/" + data_file_name;
        ofstream f;
        f.open(cp_ops_file);

        string fu_ops_file = "./fu_ops_data/" + data_file_name;
        ofstream fu;
        fu.open(fu_ops_file);

        int max_size = 0;
        Graph *max_g = nullptr;
        for (auto g : graphs)
        {
            if (g->vertices.size() > max_size)
            {
                max_g = g;
                max_size = g->vertices.size();
            }
        }

        fu << "t ";
        for (auto op : max_g->vertices)
        {
            fu << pm->uid_insn_map[op]->getOpcode() << " ";
        }
        f << "\n";

        fu.close();

        for (auto g : graphs)
        {
            double max_delay = 0.;
            vector<uint64_t> cp_op_uids;

            Graph *clean_subgraph = pm->rm_graph_ops(ops, g);
            DependencyGraph dep_graph = pm->build_data_dependency_graph(clean_subgraph);

            for (auto uid : g->vertices)
            {
                vector<uint64_t> tmp_cp_op_uids;
                double delay = pm->get_cp_ops(uid, g, tmp_cp_op_uids, dep_graph);
                if (delay >= max_delay)
                {
                    max_delay = delay;
                    cp_op_uids = tmp_cp_op_uids;
                }
            }

            f << "t ";
            for (auto uid : cp_op_uids)
            {
                f << uid << " ";
                // errs() << "uid=" << uid << " opname=" << pm->uid_insn_map[uid]->getOpcodeName() << "\n";
            }
            f << "\n";
        }
        f.close();
    }

    return 0;
}