#include "PatternManager.h"
#include "graph.h"
#include "utils.h"
#include "../cost_model/cost_model.h"

#include <map>
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;

int main(int argc, char **argv)
{
    string ir_file = string(argv[1]);
    string patt_dir = string(argv[2]);
    string graph_path = string(argv[3]);
    string config_file = string(argv[4]);

    vector<Graph *> graphs = read_graph_data(graph_path);

    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);
    pm->init_graphs(graphs);

    CostModel *cost_model = new CostModel(config_file);
    pm->opcode_delay = cost_model->opcode_delay;

    int file_id = 0;
    Pattern clean_patt;

    for (const auto &entry : fs::directory_iterator(patt_dir))
    {
        string patt_path = entry.path();
        if (patt_path.find(".data") == std::string::npos)
        {
            continue;
        }
        std::cout << "patt_path=" << patt_path << "\n";
        Pattern pattern = read_graph_data(patt_path);

        for (auto g : pattern)
        {
            Graph *clean_subgraph = pm->rm_graph_ops(pm->ops, g);
            if (clean_subgraph->vertices.size() > 1)
            {
                clean_patt.push_back(clean_subgraph);
            }
        }
    }

    for (auto g : clean_patt)
    {

        string data_file_name = to_string(file_id) + ".data";

        pm->dump_graph_data("temp_output/" + data_file_name, {g});

        string cp_ops_file = "./cp_ops_data/" + data_file_name;
        ofstream f;
        f.open(cp_ops_file);
        
        string fu_ops_file = "./fu_ops_data/" + data_file_name;
        ofstream fu;
        fu.open(fu_ops_file);

        double max_delay = 0.;
        vector<uint64_t> cp_op_ids;

        DependencyGraph dep_graph = pm->build_data_dependency_graph(g);

        for (auto uid : g->vertices)
        {
            vector<uint64_t> tmp_cp_op_ids;
            double delay = pm->get_cp_ops(uid, g, tmp_cp_op_ids, dep_graph);
            if (delay >= max_delay)
            {
                max_delay = delay;
                cp_op_ids = tmp_cp_op_ids;
            }
        }

        f << "t ";
        for (auto uid : cp_op_ids)
        {
            f << uid << " ";
            // errs() << "uid=" << uid << " opname=" << pm->uid_insn_map[uid]->getOpcodeName() << "\n";
        }
        f << "\n";
        
        fu << "t ";
        for (auto uid : g->vertices)
        {
            fu << pm->uid_insn_map[uid]->getOpcode() << " ";
            // errs() << "uid=" << uid << " opname=" << pm->uid_insn_map[uid]->getOpcodeName() << "\n";
        }
        fu << "\n";

        f.close();
        fu.close();

        file_id++;
    }
    return 0;
}