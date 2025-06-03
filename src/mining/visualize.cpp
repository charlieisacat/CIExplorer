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
    string patt_dir = string(argv[2]);
    string dyn_info_file = string(argv[3]);
    
    map<string, long> iter_map;
    read_dyn_info(dyn_info_file, &iter_map);

    vector<Pattern *> patterns;
    for (const auto &entry : fs::directory_iterator(patt_dir))
    {
        string patt_path = entry.path();
        Pattern pattern = read_graph_data(patt_path);
        patterns.push_back(new Pattern(pattern.begin(), pattern.end()));

        bind_dyn_info(&iter_map, *(patterns.back()));
    }
    
    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);

    pm->patterns = patterns;

    int new_insn_id = 0;
    for (Pattern *pattern : patterns)
    {
        int cc_id = 0;
        for (Graph *g : *pattern)
        {
            string dfg_file = "./mining_output/inst" + to_string(new_insn_id) + "_" + to_string(cc_id) + "_" + to_string(g->itervalue) + ".dot";
            ofstream dfg_output;
            dfg_output.open(dfg_file);
            pm->dump_dfg(g, dfg_output);
            dfg_output.close();
            cc_id++;
        }
        new_insn_id++;
    }
}