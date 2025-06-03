#include "../mining/utils.h"
#include "../mining/graph.h"
#include "../mining/PatternManager.h"
#include "../cost_model/cost_model.h"

#include <map>
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;
using namespace llvm;

struct Match
{
    int clique_id = 0;
    vector<int> match;

    Match(vector<int> _match, int _clique_id) : match(_match), clique_id(_clique_id) {}
};

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

int get_max_lvl(PatternManager *pm, Graph *graph, map<int, int> &uid_lvl_map)
{
    int max_lvl = 0;

    for (int uid : graph->vertices)
    {
        int lvl = uid_lvl_map[uid];
        max_lvl = max(max_lvl, lvl);
    }

    return max_lvl;
}

struct Vertex
{
    int uid;
    int unique_id;
    string opname;

    Vertex(int uid, int unique_id, string opname) : uid(uid), unique_id(unique_id), opname(opname) {}
};

void icg_gen_step2(PatternManager *pm, vector<Graph *> graphs, map<int, int> &uid_lvl_map,
                   map<pair<string, string>, double> &E)
{

    for (Graph *graph : graphs)
    {
        map<int, string> uid_name_map;
        int max_lvl = get_max_lvl(pm, graph, uid_lvl_map);

        for (int k = 0; k < max_lvl; k++)
        {
            vector<int> insts;

            double tn_sum = 0.;
            double max_tn = 0.;

            for (int uid : graph->vertices)
            {
                // constraint
                string opcode_name = pm->uid_insn_map[uid]->getOpcodeName();
                if (count(pm->ops.begin(), pm->ops.end(), opcode_name) != 0)
                    continue;

                if (uid_lvl_map[uid] != k)
                    continue;

                int num = 0;
                uid_name_map[uid] = opcode_name + to_string(num);
                for (int uniq_uid : insts)
                {
                    string opname = pm->uid_insn_map[uniq_uid]->getOpcodeName();
                    if (opname == opcode_name)
                    {
                        num++;
                        uid_name_map[uniq_uid] = opname + to_string(num);
                    }
                }

                insts.push_back(uid);
                double tn = pm->get_hw_delay(pm->uid_insn_map[uid]->getOpcode());
                tn_sum += tn;

                if (tn > max_tn)
                {
                    max_tn = tn;
                }
            }

            int insn_size = insts.size();
            double wk = graph->itervalue * (tn_sum - max_tn) / ((insn_size * (insn_size - 1)) / 2);

            for (int i = 0; i < insts.size(); i++)
            {
                string frm = uid_name_map[insts[i]];
                for (int j = i + 1; j < insts.size(); j++)
                {
                    string to = uid_name_map[insts[j]];

                    assert(!(E.find({frm, to}) != E.end() && E.find({to, frm}) != E.end()));

                    if (E.find({frm, to}) == E.end() && E.find({to, frm}) == E.end())
                    {
                        E.insert({{frm, to}, wk});
                    }
                    else if (E.find({frm, to}) != E.end() && E.find({to, frm}) == E.end())
                    {
                        E[{frm, to}] += wk;
                    }
                    else if (E.find({to, frm}) != E.end() && E.find({frm, to}) == E.end())
                    {
                        E[{to, frm}] += wk;
                    }
                }
            }
        }
        // break;
    }
}

void icg_gen_step1(PatternManager *pm, vector<Graph *> graphs, map<int, int> &uid_lvl_map, vector<Vertex *> &V)
{

    for (Graph *graph : graphs)
    {
        int max_lvl = get_max_lvl(pm, graph, uid_lvl_map);

        for (int k = 0; k < max_lvl; k++)
        {
            vector<Vertex *> insts;
            for (int uid : graph->vertices)
            {
                // constraint
                string opcode_name = pm->uid_insn_map[uid]->getOpcodeName();
                if (count(pm->ops.begin(), pm->ops.end(), opcode_name) != 0)
                    continue;

                if (uid_lvl_map[uid] != k)
                    continue;

                int num = 0;

                Vertex *v = new Vertex(uid, num, opcode_name);

                for (Vertex *unique_insn : insts)
                {
                    if (unique_insn->opname == opcode_name)
                    {
                        num += 1;
                        unique_insn->unique_id = num;
                    }
                }
                insts.push_back(v);
            }

            for (Vertex *l : insts)
            {
                V.push_back(l);
            }
        }
        // break;
    }
}

void match(vector<vector<string>> cliques, vector<int> insts, map<int, string> uid_name_map, vector<vector<int>> &ret, vector<int> &clique_ids)
{
    int idx = 0;

    for (int id = 0; id < cliques.size(); id++)
    {
        vector<string> clique = cliques[id];

        vector<int> visit(clique.size(), 0);
        vector<int> match;
        for (int uid : insts)
        {
            string name = uid_name_map[uid];
            bool found = false;
            for (int i = 0; i < clique.size(); i++)
            {
                if (visit[i])
                    continue;

                if (clique[i] == name)
                {
                    // cout << "clique[i]=" << clique[i] << " name=" << name << "\n";
                    found = true;
                    visit[i] = true;
                    break;
                }
            }

            if (found)
                match.push_back(uid);
        }

        if (match.size() >= 2)
        {
            ret.push_back(match);
            clique_ids.push_back(id);
        }
    }
}

vector<Match *> extract_pattern(PatternManager *pm, Graph *graph, map<int, int> uid_lvl_map, vector<vector<string>> cliques)
{
    map<int, string> uid_name_map;

    int max_lvl = get_max_lvl(pm, graph, uid_lvl_map);

    vector<Match *> matches;

    for (int k = 0; k < max_lvl; k++)
    {
        vector<int> insts;
        for (int uid : graph->vertices)
        {
            // constraint
            string opcode_name = pm->uid_insn_map[uid]->getOpcodeName();
            if (count(pm->ops.begin(), pm->ops.end(), opcode_name) != 0)
                continue;

            if (uid_lvl_map[uid] != k)
                continue;

            int num = 0;
            uid_name_map[uid] = opcode_name + to_string(num);
            for (int uniq_uid : insts)
            {
                string opname = pm->uid_insn_map[uniq_uid]->getOpcodeName();
                if (opname == opcode_name)
                {
                    num++;
                    uid_name_map[uniq_uid] = opname + to_string(num);
                }
            }

            insts.push_back(uid);
        }

        vector<int> clique_ids;
        vector<vector<int>> ret;
        match(cliques, insts, uid_name_map, ret, clique_ids);
        if (ret.size())
        {
            for (int i = 0; i < ret.size(); i++)
            {
                matches.push_back(new Match(ret[i], clique_ids[i]));
            }
        }
    }

    return matches;
}

map<string, int> build_vid_map(vector<string> V, map<pair<string, string>, double> E)
{
    int N = V.size();
    map<string, int> v_id_map;

    for (int i = 0; i < N; i++)
    {
        v_id_map.insert(make_pair(V[i], i));
    }
    return v_id_map;
}

int **build_adjmat(vector<string> V, map<pair<string, string>, double> E, map<string, int> v_id_map, vector<int> visit)
{
    int N = V.size();

    int **adj_mat = (int **)malloc(sizeof(int *) * N);
    for (int i = 0; i < N; i++)
    {
        adj_mat[i] = (int *)malloc(sizeof(int) * N);
        memset(adj_mat[i], 0, N * sizeof(int));
    }

    for (auto iter = E.begin(); iter != E.end(); iter++)
    {
        string frm = iter->first.first;
        string to = iter->first.second;

        int frm_id = v_id_map[frm];
        int to_id = v_id_map[to];

        if (!visit[frm_id] && !visit[to_id])
        {
            adj_mat[frm_id][to_id] = 1;
            adj_mat[to_id][frm_id] = 1;
        }
    }

    return adj_mat;
}

void compute_node_weight(vector<string> V, map<pair<string, string>, double> E, map<string, double> &v_w_map, vector<int> visit, map<string, int> v_id_map)
{
    for (int i = 0; i < V.size(); i++)
    {
        if (visit[i])
            continue;
        string v = V[i];
        double total_w = 0.;
        for (auto e : E)
        {
            string frm = e.first.first;
            string to = e.first.second;

            if (visit[v_id_map[frm]] || visit[v_id_map[to]])
                continue;

            double w = e.second;

            if (frm == v || to == v)
            {
                // cout << "v=" << v << " frm=" << frm << " to=" << to << " w=" << w << "\n";
                total_w += w;
            }
        }
        v_w_map.insert(make_pair(v, total_w));
    }
}

vector<int> get_top_5_clique(vector<string> V, map<string, double> v_w_map, vector<int> clique)
{
    vector<double> clique_weights;
    for (int vid : clique)
    {
        clique_weights.push_back(v_w_map[V[vid]]);
    }

    std::vector<std::pair<double, int>> paired;
    for (size_t i = 0; i < clique_weights.size(); ++i)
    {
        paired.emplace_back(clique_weights[i], clique[i]);
    }

    // Sort by the double values
    std::sort(paired.begin(), paired.end(), std::greater<>());

    vector<int> sorted_clique;
    // Extract the sorted indices
    int i = 0;
    for (const auto &pair : paired)
    {
        if (i >= 5)
            break;

        sorted_clique.push_back(pair.second);
        i++;
    }

    return sorted_clique;
}

vector<int> get_max_set(vector<string> V, map<string, double> v_w_map, vector<vector<int>> cliques)
{
    double max_clique_w;
    vector<int> ret;
    for (auto clique : cliques)
    {
        auto sorted_clique = get_top_5_clique(V, v_w_map, clique);

        double clique_w = 0.;
        for (int vid : sorted_clique)
        {
            clique_w += v_w_map[V[vid]];
        }
        if (max_clique_w < clique_w)
        {
            max_clique_w = clique_w;
            ret = sorted_clique;
        }
    }
    return ret;
}

vector<vector<int>> merge_cliques(vector<string> V,
                                  vector<vector<int>> to_merge_cliques)
{

    for (auto &v : V)
    {
        v.erase(std::remove_if(v.begin(), v.end(), ::isdigit), v.end());
    }

    vector<int> visit_cliques(to_merge_cliques.size(), 0);
    vector<vector<int>> merged_clique_ids;

    for (int i = 0; i < to_merge_cliques.size(); i++)
    {
        if (visit_cliques[i])
            continue;

        vector<int> clique_ids = {i};

        visit_cliques[i] = 1;
        vector<int> clique = to_merge_cliques[i];
        vector<string> vertices;
        for (int vid : clique)
        {
            vertices.push_back(V[vid]);
        }

        for (int j = i + 1; j < to_merge_cliques.size(); j++)
        {
            if (visit_cliques[j])
                continue;
            vector<int> visit(clique.size(), 0);
            vector<int> to_merge_clique = to_merge_cliques[j];

            int merged_n = 0;

            for (int vid : to_merge_clique)
            {
                bool no_merge = true;
                string v = V[vid];
                for (int k = 0; k < vertices.size(); k++)
                {
                    if (!visit[k])
                    {
                        if (v == vertices[k])
                        {
                            merged_n++;
                            visit[k] = 1;
                            no_merge = false;
                            break;
                        }
                    }
                }

                if (no_merge)
                {
                    break;
                }
            }

            if (merged_n == to_merge_clique.size())
            {
                visit_cliques[j] = 1;
                clique_ids.push_back(j);
            }
        }

        merged_clique_ids.push_back(clique_ids);
    }

    return merged_clique_ids;
}

Graph *build_subgraph(vector<int> match, Graph *bb)
{
    Graph *graph = new Graph();
    for (int uid : match)
    {
        graph->add_vertex(uid);
    }
    graph->bbname = bb->bbname;

    return graph;
}

int main(int argc, char **argv)
{
    string ir_file = string(argv[1]);
    string bblist_path = string(argv[2]);
    string dyn_info_file = string(argv[3]);

    vector<string> bblist;
    read_bblist(bblist_path, bblist);

    map<string, long> iter_map;
    read_dyn_info(dyn_info_file, &iter_map);

    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);

    CostModel *cost_model = new CostModel("/staff/haoxiaoyu/uarch_exp/patt_mining/src/cost_model/config_broadwell.yml");
    pm->opcode_delay = cost_model->opcode_delay;

    vector<Graph *> graphs;

    for (Graph *graph : pm->graphs)
    {
        if (count(bblist.begin(), bblist.end(), graph->bbname) != 0)
        {
            graphs.push_back(graph);
        }
    }

    bind_dyn_info(&iter_map, graphs);

    map<int, int> uid_lvl_map;

    for (auto graph : graphs)
    {
        group(pm, graph, uid_lvl_map);
    }

#if 0
    for (auto iter = uid_lvl_map.begin(); iter != uid_lvl_map.end(); iter++)
    {
        uint64_t uid = iter->first;
        int lvl = iter->second;

        Instruction *insn = pm->uid_insn_map[uid];
        errs() << "uid=" << uid << " insn=" << *insn << " lvl=" << lvl <<" bb="<<insn->getParent()->getName() << "\n";
    }
#endif

    vector<Vertex *> V0;
    icg_gen_step1(pm, graphs, uid_lvl_map, V0);

#if 0
    for (Vertex *v : V0)
    {
        cout << "uid=" << v->uid << " unqi=" << v->unique_id << " opname=" << v->opname << " lvl=" << uid_lvl_map[v->uid] << "\n";
    }

    for (auto iter = E.begin(); iter != E.end(); iter++)
    {
        pair<string, string> e = iter->first;
        int w = iter->second;

        cout << "frm=" << e.first << " to=" << e.second << " w=" << w << "\n";
    }
#endif

    map<pair<string, string>, double> E;
    icg_gen_step2(pm, graphs, uid_lvl_map, E);

    set<string> uniq_V;

    for (auto iter = E.begin(); iter != E.end(); iter++)
    {
        pair<string, string> e = iter->first;
        uniq_V.insert(e.first);
        uniq_V.insert(e.second);
    }

    int N = uniq_V.size();
    int visit_n = 0;
    vector<int> visit(N, 0);
    vector<string> V(uniq_V.begin(), uniq_V.end());

    map<string, int> v_id_map = build_vid_map(V, E);

    vector<vector<int>> to_merge_cliques;

    while (visit_n < N)
    {
        int **adj_mat = build_adjmat(V, E, v_id_map, visit);

#if 0
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            cout << adj_mat[i][j] << " ";
        }
        cout << "\n";
    }
#endif

        int **some = (int **)malloc(sizeof(int *) * N);
        int **all = (int **)malloc(sizeof(int *) * N);
        int **none = (int **)malloc(sizeof(int *) * N);
        for (int i = 0; i < N; i++)
        {
            some[i] = (int *)malloc(sizeof(int) * N);
            memset(some[i], 0, N * sizeof(int));

            all[i] = (int *)malloc(sizeof(int) * N);
            memset(all[i], 0, N * sizeof(int));

            none[i] = (int *)malloc(sizeof(int) * N);
            memset(none[i], 0, N * sizeof(int));
        }

        // init for BK
        for (int i = 0; i < N; ++i)
            some[0][i] = i;

        vector<vector<int>> cliques;
        BK(0, 0, N, 0, some, none, all, adj_mat, cliques, N);

        map<string, double> v_w_map;
        compute_node_weight(V, E, v_w_map, visit, v_id_map);

        vector<int> clique = get_max_set(V, v_w_map, cliques);

        if (clique.size() == 0)
            break;

#if 0
        // print bk results
        for (int i = 0; i < cliques.size(); i++)
        {
            auto v = cliques[i];

            for (auto x : v)
            {
                cout << x << " ";
            }
            cout << "\n";

            for (auto x : v)
            {
                cout << "x="<< x << " w=" << v_w_map[V[x]]<<" V[x]="<<V[x] << "\n";
            }
        }
#endif

#if 1
        cout << "max clique:\n";

        for (auto x : clique)
        {
            cout << x << " V[x]=" << V[x] << "\n";
        }
        cout << "\n";
#endif

        visit_n += clique.size();
        for (int vid : clique)
        {
            visit[vid] = 1;
        }

        to_merge_cliques.push_back(clique);
    }

    vector<vector<string>> cliques_str;

    for (auto clique : to_merge_cliques)
    {
        vector<string> clique_str;
        for (auto id : clique)
        {
            clique_str.push_back(V[id]);
        }
        cliques_str.push_back(clique_str);
    }

    vector<Graph *> subgraphs;
    map<int, vector<Graph *>> clique_subgraphs;

    for (auto graph : graphs)
    {
        auto matches = extract_pattern(pm, graph, uid_lvl_map, cliques_str);
        for (auto match : matches)
        {
            // cout << "clique id=" << match->clique_id << " size=" << match->match.size() << "\n";
            // for (auto uid : match->match)
            // {
            //     cout << "uid=" << uid << insn_to_string(pm->uid_insn_map[uid]) << "\n";
            // }
            // for (auto s : cliques_str[match->clique_id])
            // {
            //     cout << "s=" << s << "\n";
            // }
            // cout << "\n";

            Graph *subgraph = build_subgraph(match->match, graph);
            if (!clique_subgraphs.count(match->clique_id))
            {
                clique_subgraphs[match->clique_id] = vector<Graph *>();
            }
            clique_subgraphs[match->clique_id].push_back(subgraph);
        }
    }

    int i = 0;
    for (auto iter = clique_subgraphs.begin(); iter != clique_subgraphs.end(); iter++)
    {
        vector<Graph *> subgraphs = iter->second;
        pm->dump_graph_data("./to_debug_pass/" + to_string(i) + ".data", subgraphs);

        string cp_ops_file = "./cp_ops_data/" + to_string(i) + ".data";
        ofstream f;
        f.open(cp_ops_file);
        for (auto g : subgraphs)
        {
            int max_uid = 0;
            double max_delay = 0.;

            for (auto uid : g->vertices)
            {
                auto insn = pm->uid_insn_map[uid];
                double delay = pm->get_hw_delay(insn->getOpcode());
                if (delay > max_delay)
                {
                    max_uid = uid;
                    max_delay = delay;
                }
            }

            f << "t " << max_uid << "\n";
        }
        f.close();

        string fu_ops_file = "./fu_ops_data/" + to_string(i) + ".data";
        f.open(fu_ops_file);
        int max_ops_n = 0;
        vector<int> opcodes;

        for (auto g : subgraphs)
        {
            if (g->vertices.size() > max_ops_n)
            {
                max_ops_n = g->vertices.size();

                vector<int> ops;
                for (auto uid : g->vertices)
                {
                    auto opcode = pm->uid_insn_map[uid]->getOpcode();
                    ops.push_back(opcode);
                }

                opcodes = ops;
            }
        }

        f << "t ";
        for (auto opcode : opcodes)
        {
            f << opcode << " ";
        }
        f << "\n";

        f.close();

        i++;
    }

    // vector<vector<int>> merged_clique_ids = merge_cliques(V, to_merge_cliques);
    // cout << "clique_ids=\n";

    // for (auto clique_ids : merged_clique_ids)
    // {
    //     for (int id : clique_ids)
    //     {
    //         cout << id << " ";
    //     }

    //     cout << "\n";
    // }

    return 0;
}