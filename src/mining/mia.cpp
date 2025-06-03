#include "PatternManager.h"
#include "graph.h"
#include "utils.h"

#include <memory>
#include <filesystem>
#include <unordered_map>
namespace fs = std::filesystem;

using namespace std;

int main(int argc, char **argv)
{
    string ir_file = string(argv[1]);
    string graph_path = string(argv[2]);
    string patt_dir = string(argv[3]);
    string dyn_info_file = string(argv[4]);
    string bblist_path = string(argv[5]);

    vector<Graph *> graphs = read_graph_data(graph_path);

    vector<Pattern *> patterns;

    for (const auto &entry : fs::directory_iterator(patt_dir))
    {
        string patt_path = entry.path();
        std::cout<<"patt_path="<<patt_path<<"\n";
        Pattern pattern = read_graph_data(patt_path);
        patterns.push_back(new Pattern(pattern.begin(), pattern.end()));
    }

    map<string, long> iter_map;
    read_dyn_info(dyn_info_file, &iter_map);
    bind_dyn_info(&iter_map, graphs);
    
    vector<string> bblist;
    read_bblist(bblist_path, bblist);

    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);

    pm->init_graphs(graphs);
    pm->patterns = patterns;

    pm->mia(graphs, bblist);

    return 0;
}