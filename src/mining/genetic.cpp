#include <unordered_map>
#include <set>
#include <algorithm>
#include <assert.h>
#include <list>
#include <vector>
#include <limits>
#include <memory>
#include <time.h>

#include "utils.h"
#include "graph.h"
#include "PatternManager.h"
#include "../galgo/Galgo.hpp"
#include "../cost_model/cost_model.h"
#include "../cost_model/packing.h"

using namespace std;

enum SearchType
{
    BBLDST,
    FuncLDST,
    BBSupport
};

class PatternSearch
{
public:
    PatternManager *pm;
    CostModel *cost_model;
    int curr_patt_id;
    int curr_gid; // Pattern has many graphs
    double alpha = 1.0;
    int n_in = 1;
    int n_out = 1;
    int dup = 4;

    // no constraint
    bool no_constraint = false;

    PatternSearch() {
    };

    void set_patt_and_graph_id(int patt_id, int gid)
    {
        this->curr_patt_id = patt_id;
        this->curr_gid = gid;
    }

    unordered_map<string, double> bb_score_map;
    unordered_map<string, vector<string>> x_code_map;
};

auto ps = std::make_unique<PatternSearch>();

template <typename T>
class BBSupportObjective
{
public:
    static std::vector<std::string> Objective(const std::vector<T> &x)
    {
        double delay_score = 0.0;

        // 这里可能为空
        std::vector<std::string> code_per_bb;
        std::vector<int> gids;
        bool no_forbid_insn = ps->pm->check_no_forbidden_insn(x[0], ps->curr_patt_id, ps->curr_gid);
        // bool no_forbid_insn = true;

        // in case loop in patt
        if (!no_forbid_insn)
        {
            code_per_bb = {"0.0", "0"};
            return code_per_bb;
        }

        bool io_flag = true;
        bool convex = true;

        ps->pm->get_code_support(code_per_bb, gids, x[0], ps->curr_patt_id, ps->curr_gid);
        int support = 0;
        vector<double> cp_delays;
        vector<double> seq_delays;
        vector<double> io_scores;

        double cp_delay = 0., seq_delay = 0.;
        long bb_exe_count = 0;
        int width = 0;

        int pid = ps->curr_patt_id;

        assert(code_per_bb.size() == gids.size());

        for (int i = 0; i < code_per_bb.size(); i++)
        {
            std::string code = code_per_bb[i];
            if (code != "")
            {
                assert(gids[i] >= 0);

                bool tmp_convex = ps->pm->get_convexity(code, pid, gids[i]);
                convex = convex & tmp_convex;

                if (!tmp_convex)
                    continue;

                vector<int> ion = ps->pm->get_io_number(code, pid, gids[i]);

                // bool tmp_io_flag = true;
                // if (!ps->no_constraint)
                // {
                //     tmp_io_flag = (ion[0] <= ps->n_in) && (ion[1] <= ps->n_out);
                // }
                // io_flag = io_flag & tmp_io_flag;

                // if (!tmp_io_flag)
                //     break;

                // double io_score = ps->n_in - ion[0] + ps->n_out - ion[1];
                // we don't use alpha memtioned in paper, becasue we dont want io score be negative
                // double io_score = 1 - (ps->n_in - ion[0] + ps->n_out - ion[1]) / (ps->n_in + ps->n_out);

                support += 1;
                cp_delay = ps->pm->get_graph_cp_delay(code, pid, gids[i]);
                seq_delay = ps->pm->get_graph_seq_delay(code, pid, gids[i]);

                // std::cout << "code=" << code << " cpdelay=" << cp_delay << " seq_delay=" << seq_delay << " tmp_convex " << tmp_convex << " ion[0]=" << ion[0] << " ion[1]=" << ion[1] << "\n";

                cp_delays.push_back(cp_delay);
                seq_delays.push_back(seq_delay);
                // io_scores.push_back(io_score);
                io_scores.push_back(1.);
            }
        }

        if (!io_flag || !convex)
        {
            code_per_bb = {"0.0", "0"};
            return code_per_bb;
        }

        for (int i = 0; i < cp_delays.size(); i++)
        {
            delay_score += seq_delays[i] / cp_delays[i] * io_scores[i];
        }

        assert(convex == true && io_flag == true);

        double fitness = delay_score;

        code_per_bb.insert(code_per_bb.begin(), std::to_string(code_per_bb.size()));
        code_per_bb.insert(code_per_bb.begin(), std::to_string(fitness));

        for (auto gid : gids)
        {
            code_per_bb.push_back(to_string(gid));
        }
        return code_per_bb;
    }
};

LLVMContext context;
SMDiagnostic error;
std::unique_ptr<Module> m;

int main(int argc, char **argv)
{
    assert(argc >= 2);

    clock_t start, end;
    start = clock();
    std::string ir_file = std::string(argv[1]);
    std::string graph_path = std::string(argv[2]);
    std::string patt_dir = std::string(argv[3]);

    int popsize = std::stol(std::string(argv[4]));
    int nbgen = std::stol(std::string(argv[5]));

    int genstep = std::stol(std::string(argv[6]));
    int elitpop = std::stol(std::string(argv[7]));
    int matsize = std::stol(std::string(argv[8]));
    int tntsize = std::stol(std::string(argv[9]));

    std::string selection = std::string(argv[10]);
    std::string mutation = std::string(argv[11]);
    std::string crossover = std::string(argv[12]);
    std::string adaptation = std::string(argv[13]);

    double covrate = std::stod(std::string(argv[14]));
    double mutrate = std::stod(std::string(argv[15]));
    double SP = std::stod(std::string(argv[16]));
    double tolerance = std::stod(std::string(argv[17]));
    double chrmutrate = std::stod(std::string(argv[18]));

    double alpha = std::stod(std::string(argv[19]));

    int n_in = std::stod(std::string(argv[20]));
    int n_out = std::stod(std::string(argv[21]));

    int earlystop = std::stoi(std::string(argv[22]));

    int no_constraint = std::stoi(std::string(argv[23]));
    std::string outdir = std::string(argv[24]);
    std::string dynamic_info_file = std::string(argv[25]);
    std::string config_file = std::string(argv[26]);

    std::vector<Graph *> graphs = read_graph_data(graph_path);
    std::vector<Pattern *> patterns = read_pattern_data(patt_dir + "/attrs.output");
    std::vector<Pattern *> seeds = read_pattern_data(patt_dir + "/seeds.data");

    m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);
    ps->pm = pm;
    ps->cost_model = new CostModel(config_file);

    pm->opcode_delay = ps->cost_model->opcode_delay;

    vector<string> bblist;
    read_bblist("../data/bblist.txt", bblist);

    cout << "Before remove Illegal patts, size=" << patterns.size() << "\n";
    remove_illegal_patts(patterns);
    cout << "After remove Illegal patts, size=" << patterns.size() << "\n";
    patterns.insert(patterns.end(), seeds.begin(), seeds.end());
    pm->patterns = patterns;

    cout << "Number with seeds=" << patterns.size() << "\n";

    ps->alpha = alpha;
    ps->n_in = n_in;
    ps->n_out = n_out;
    ps->no_constraint = no_constraint;

    double total_score = 0.;

    map<string, long> iter_map;
    read_dyn_info(dynamic_info_file, &iter_map);
    bind_dyn_info(&iter_map, graphs);

    pm->init_graphs(graphs);

    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "phi",
                          "ret",
                          "br", "alloca", "call", "invoke", "landingpad"};

    for (int pid = 0; pid < patterns.size(); pid++)
    {
        ps->x_code_map.clear();

        int gid = pm->get_proper_gid(*patterns[pid]);
        Graph *cc = (*patterns[pid])[gid];

        cout<<"pid="<<pid<<" gid="<<gid<<"\n";
        for (auto uid : cc->vertices)
        {
            cout << "   " << insn_to_string(pm->uid_insn_map[uid]) << "\n";
        }

        ps->set_patt_and_graph_id(pid, gid);

        Graph *bb = pm->bbname_graph_map[cc->bbname];
        string code = pm->encode_candidate_component(cc, bb);

        if(!ps->pm->check_no_forbidden_insn(code, pid, gid))
        {
            cout<<"skip...\n";
            continue;
        }

        std::string lowerBound = std::string(code.size(), '0');
        std::string upperBound = std::string(code.size(), '1');

        galgo::Parameter<std::string> par({lowerBound, upperBound, code});

        // default is bbsupportobjective
        galgo::GeneticAlgorithm<std::string> *ga;

        ga = new galgo::GeneticAlgorithm<std::string>(BBSupportObjective<std::string>::Objective, code, popsize, nbgen, true, code.size(), par);

        // SPM, BDM, UNM
        ga->Mutation = getMutFunc<std::string>(mutation);

        // P1XO, P2XO, UXO
        ga->CrossOver = getCrsvFunc<std::string>(crossover);

        // RWS, SUS, RNK, RSP, TNT, TRS
        ga->Selection = getSelFunc<std::string>(selection);

        ga->genstep = genstep;
        ga->covrate = covrate;
        ga->mutrate = mutrate;
        ga->chrmutrate = chrmutrate;
        ga->SP = SP;
        ga->tolerance = tolerance;
        ga->elitpop = elitpop;
        ga->tntsize = tntsize;
        ga->earlystop = earlystop;

        ga->pattid = pid;
        ga->graphid = gid;

        auto codes = ga->run();
        double score = std::stod(codes[0]);
        int code_len = std::stoi(codes[1]);
        int support = 0;

        std::vector<Graph *> subgraphs;

        subgraphs.clear();
        for (int j = 2; j < 2 + code_len; j++)
        {
            string code = codes[j];
            if (code == "")
            {
                continue;
            }
            support++;

            int gid = std::stoi(codes[j + code_len]);

            Graph *cc = (*patterns[pid])[gid];
            Graph *bb = pm->bbname_graph_map[cc->bbname];

            Graph *subgraph = pm->build_subgraph(code, bb);
            Graph *clean_subgraph = pm->rm_graph_ops(ops, subgraph);

            bool tmp_convex = pm->dfs_check_convex(clean_subgraph);
            // std::cout << "pattid=" << pid << " gid=" << gid << " convex=" << tmp_convex << " code=" << code << "\n";

            // cout << "name=" << bb->bbname << "\n";
            // for (int i = 0; i < code.size(); i++)
            // {
            //     if (code[i] == '1')
            //     {
            //         cout << "uid=" << bb->vertices[i] << "\n";
            //     }
            // }
            subgraph->score = score;
            clean_subgraph->score = subgraph->score;
            if (subgraph->score > 1e-5 && subgraph->vertices.size() > 1)
            {
                subgraphs.push_back(clean_subgraph);
            }
        }

        if (subgraphs.size() != 0)
        {
            std::cout << " Code=" << code << " Patt id=" << pid << " Support= " << support << " score=" << score << "\n";
            // pm->dump_pattern_data(subgraphs, 0, outdir + "/" + std::to_string(pid) + "_attrs.output");
            pm->dump_graph_data(outdir + "/" + to_string(pid) + ".patt", subgraphs);
            total_score += score;
        }

        free(ga);
        std::cout << "\n\n";
    }
    cout << "total_score=" << total_score << "\n";
    end = clock();
    cout << "searchtime: " << double(end - start) / CLOCKS_PER_SEC << " s\n";
    return 0;
}
