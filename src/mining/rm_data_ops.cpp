#include "PatternManager.h"
#include "graph.h"
#include "utils.h"

#include <memory>
#include <filesystem>
#include <unordered_map>
#include <iostream>
namespace fs = std::filesystem;

using namespace std;

int main(int argc, char **argv)
{
    string ir_file = string(argv[1]);
    string graph_path = string(argv[2]);
    string out_dir = string(argv[3]);
    string bblist_path = string(argv[4]);

    vector<Graph*> graphs;

    graphs = read_graph_data(graph_path);

    vector<string> bblist;
    read_bblist(bblist_path, bblist);

    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);

    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    vector<Graph*> subgraphs;
    for (auto g : graphs)
    {
        if (count(bblist.begin(), bblist.end(), g->bbname) == 0)
        {
            continue;
        }
        auto subgraph = pm->rm_graph_ops(ops, g);
        if (subgraph->vertices.size() > 1)
            subgraphs.push_back(subgraph);
    }

    pm->dump_graph_data(out_dir+"/clean.data", subgraphs);

    return 0;
}