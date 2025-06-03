#include "PatternManager.h"

#include <fstream>

// Function to build the data dependency graph
DependencyGraph PatternManager::build_data_dependency_graph(const std::vector<Instruction *> &instructions)
{
    DependencyGraph dep_graph;

    // Create a set of instructions for fast lookup
    std::set<Instruction *> instruction_set(instructions.begin(), instructions.end());

    // Iterate over each instruction in the input vector
    for (auto *insn : instructions)
    {
        // Ensure the instruction has an entry in the graph
        dep_graph[insn] = vector<Instruction *>();

        // Iterate over each operand of the current instruction
        for (auto &operand : insn->operands())
        {
            // Check if the operand is also an instruction and in the input set
            if (auto *operand_inst = dyn_cast<Instruction>(operand))
            {
                if (instruction_set.find(operand_inst) != instruction_set.end())
                {
                    // Add an edge from operandInst (producer) to insn (consumer)
                    // porducer uid < consumer uid to avoid cycle
                    if (insn_uid_map[operand_inst] < insn_uid_map[insn])
                    {
                        dep_graph[operand_inst].push_back(insn);
                    }
                }
            }
        }
    }

    return dep_graph;
}

// Function to print the dependency graph in the required format
void PatternManager::dump_graph_data(ofstream &f, DependencyGraph &graph, std::vector<Instruction *> &instructions, int gid, double score)
{

    // Map to store instruction to vertex ID
    std::map<Instruction *, int> instruction_to_id;
    int id = 0;

    f << "t # " << gid << "\n"; // start with 0

    // Assign IDs to each instruction
    for (auto &insn : instructions)
    {
        instruction_to_id[insn] = id++;
    }

    // Print the vertices
    for (auto &instruction : instructions)
    {
        int vid = instruction_to_id[instruction];
        int uid = insn_uid_map[instruction];

        f << "v " << vid << " " << instruction->getOpcode() << " " << instruction->getOpcodeName() << " "
          << "module" << " " << "func" << " "
          << instruction->getParent()->getName().str() << " " << vid << " " << uid << "\n";
    }

    // Print the edges
    for (auto &instruction : instructions)
    {
        auto dependents = graph[instruction];
        for (auto *dependent : dependents)
        {
            f << "e " << instruction_to_id[instruction] << " " << instruction_to_id[dependent] << " 2\n";
        }
    }
    f << "s " << score << "\n";
}

DependencyGraph PatternManager::build_data_dependency_graph(Graph *graph)
{

    std::vector<Instruction *> instructions;

    for (int j = 0; j < graph->vertices.size(); j++)
    {
        int uid = graph->vertices[j];
        Instruction *insn = uid_insn_map[uid];

        instructions.push_back(insn);
    }

    DependencyGraph dependency_graph = build_data_dependency_graph(instructions);
    return dependency_graph;
}

void PatternManager::dump_graph_data(string filename, vector<Graph *> graphs)
{

    ofstream f;
    f.open(filename);
    for (int i = 0; i < graphs.size(); i++)
    {
        auto g = graphs[i];
        std::vector<Instruction *> instructions;

        for (int j = 0; j < g->vertices.size(); j++)
        {
            int uid = g->vertices[j];
            Instruction *insn = uid_insn_map[uid];

            instructions.push_back(insn);
        }

        DependencyGraph dependency_graph = build_data_dependency_graph(instructions);

        // Print the graph in the required format
        dump_graph_data(f, dependency_graph, instructions, i, g->score);
    }
    f.close();
}

PatternManager::PatternManager(string ir_file, Module &m)
{
    int uid = 0;
    for (auto &func : m)
    {
        for (auto &bb : func)
        {
            Graph *graph = new Graph();
            for (auto &insn : bb)
            {
                uid_insn_map.insert(std::make_pair(uid, &insn));
                insn_uid_map.insert(std::make_pair(&insn, uid));

                graph->add_vertex(uid);
                graph->bbname = bb.getName().str();

                uid++;
            }
            graphs.push_back(graph);
            name_bb_map.insert(make_pair(bb.getName().str(), &bb));
        }
    }
}

Graph *PatternManager::build_subgraph(string code, Graph *g)
{
    Graph *ret = new Graph();
    ret->bbname = g->bbname;
    ret->itervalue = g->itervalue;
    ret->score = g->score;

    for (int i = 0; i < g->vertices.size(); i++)
    {
        if (code[i] == '0')
            continue;

        ret->add_vertex(g->vertices[i]);
    }

    return ret;
}

Graph *PatternManager::sort_graph_vertices(Graph *g)
{
    Graph *ret = new Graph();
    ret->bbname = g->bbname;

    std::sort(g->vertices.begin(), g->vertices.end());
    for (int i = 0; i < g->vertices.size(); i++)
    {
        ret->add_vertex(g->vertices[i]);
    }

    return ret;
}

string PatternManager::rm_graph_ops_code(vector<string> ops, Graph *g)
{
    string code = string(g->vertices.size(), '1');

    for (int i = 0; i < g->vertices.size(); i++)
    {
        int uid = g->vertices[i];
        assert(uid_insn_map.find(uid) != uid_insn_map.end());
        Instruction *insn = uid_insn_map[uid];
        string opname = insn->getOpcodeName();
        if (count(ops.begin(), ops.end(), opname))
        {
            code[i] = '0';
        }
    }

    return code;
}

Graph *PatternManager::rm_graph_ops(vector<string> ops, Graph *g)
{
    Graph *ret;
    string code = rm_graph_ops_code(ops, g);
    ret = build_subgraph(code, g);
    return ret;
}

void PatternManager::init_graphs(vector<Graph *> graphs)
{
    for (auto g : graphs)
    {
        assert(bbname_graph_map.find(g->bbname) == bbname_graph_map.end());
        bbname_graph_map[g->bbname] = g;
    }
}

int PatternManager::get_proper_gid(Pattern patt)
{
    int max_size = INT_MIN;
    int max_size_gid = -1;

    for (int i = 0; i < patt.size(); i++)
    {
        auto cc = patt[i];
        auto bb = bbname_graph_map[cc->bbname];

        int sz = bb->vertices.size();
        if (sz > max_size)
        {
            max_size = bb->vertices.size();
            max_size_gid = i;
        }
    }

    return max_size_gid;
}

string PatternManager::encode_candidate_component(Graph *cc, Graph *bb)
{
    string code = string(bb->vertices.size(), '0');

    for (auto uid : cc->vertices)
    {
        assert(bb->uid_vid_map.count(uid));
        int vid = bb->uid_vid_map[uid];
        code[vid] = '1';
    }

    return code;
}

bool PatternManager::has_dependency_ending_in_graph(Instruction *inst, set<Instruction *> inst_set)
{
    stack<Instruction *> worklist;
    set<Instruction *> visited;

    worklist.push(inst);

    while (!worklist.empty())
    {
        Instruction *current = worklist.top();
        worklist.pop();

        if (visited.count(current))
            continue;
        visited.insert(current);

        // If current is within the graph, dependency ends within the graph
        if (inst_set.count(current))
        {
            return true;
        }

        // Continue exploring operands of current instruction
        for (Use &U : current->operands())
        {
            if (Instruction *operandInst = dyn_cast<Instruction>(U.get()))
            {
                if (!visited.count(operandInst) && !isa<PHINode>(operandInst))
                {
                    worklist.push(operandInst);
                }
            }
        }
    }

    return false;
}

// return true if non-convex
bool PatternManager::dfs_check_convex(set<Instruction *> insn_set, Instruction *insn)
{
    bool ret = false;
    for (Use &U : insn->operands())
    {
        if (Instruction *operand_inst = dyn_cast<Instruction>(U.get()))
        {
            if (isa<PHINode>(operand_inst))
            {
                continue;
            }

            if (!insn_set.count(operand_inst))
            {
                // std::cout<<"check ending\n";
                // If operand is outside the graph, check if the dependency chain ends in the graph
                if (has_dependency_ending_in_graph(operand_inst, insn_set))
                {
                    return true; // Found an external dependency that ends in the graph
                }
            }
            else
            {
                // std::cout<<"go on dfs\n";
                ret |= dfs_check_convex(insn_set, operand_inst);
            }
        }
    }

    return ret; // No problematic dependencies found
}

bool PatternManager::dfs_check_convex(Graph *graph)
{
    vector<Instruction *> insns;
    for (auto uid : graph->vertices)
    {
        assert(uid_insn_map.find(uid) != uid_insn_map.end());
        insns.push_back(uid_insn_map[uid]);
    }

    set<Instruction *> insn_set(insns.begin(), insns.end());

    bool not_convex = false;
    for (auto I : insns)
        not_convex |= dfs_check_convex(insn_set, I);

    return !not_convex;
}

void PatternManager::dump_pattern_data(vector<Graph *> graphs, int t, std::string filename, bool append)
{
    std::ofstream f;
    if (append)
        f.open(filename, std::fstream::app);
    else
        f.open(filename, std::fstream::out);

    f << "t # " << t << "\n";
    for (int p = 0; p < graphs.size(); p++)
    {
        f << "p # " << p << "\n";
        auto g = graphs[p];

        std::vector<Instruction *> instructions;
        for (int j = 0; j < g->vertices.size(); j++)
        {
            int uid = g->vertices[j];
            instructions.push_back(uid_insn_map[uid]);
        }
        DependencyGraph dependency_graph = build_data_dependency_graph(instructions);

        for (auto &instruction : instructions)
        {
            auto dependents = dependency_graph[instruction];
            for (auto *dependent : dependents)
            {
                f << "a " << 0 << " " << 0 << " " << "module" << " " << "func" << " " << g->bbname << " " << instruction->getOpcodeName()
                  << " " << dependent->getOpcodeName() << " " << insn_uid_map[instruction] << " " << insn_uid_map[dependent] << "\n";
            }
        }
    }
    f.close();
}

bool PatternManager::check_no_forbidden_insn(string code, int pid, int gid)
{
    Graph *cc = (*patterns[pid])[gid];
    Graph *bb = bbname_graph_map[cc->bbname];
    Graph *subgraph = build_subgraph(code, bb);

    set<string> ops = {"store", "phi", "load",
                       "ret",
                       "br", "alloca", "call", "invoke", "landingpad"};

    for (int uid : subgraph->vertices)
    {
        assert(uid_insn_map.find(uid) != uid_insn_map.end());
        string opname = uid_insn_map[uid]->getOpcodeName();
        if (ops.count(opname))
        {
            return false;
        }
    }

    return true;
}

vector<int> get_new_vertex_uids(string code, string cc_code, Graph *cc, Graph *bb)
{
    vector<int> ret;
    // find vertices that are not in cc
    for (int i = 0; i < code.size(); i++)
    {
        if (code[i] == '1' && cc_code[i] == '0')
        {
            int uid = bb->vertices[i];
            ret.push_back(uid);
        }
    }

    return ret;
}

bool PatternManager::contains(std::string s1, std::string s2)
{
    if (s1.size() != s2.size())
        return false;

    for (int i = 0; i < s1.size(); i++)
    {
        if (s1[i] == '1' && s2[i] != '1')
            return false;
    }

    return true;
}

void PatternManager::get_code_support(vector<std::string> &code_per_bb,
                                      std::vector<int> &gids,
                                      std::string code, int pid, int gid)
{
    Pattern *patt = patterns[pid];
    Graph *cc = (*patt)[gid];
    Graph *bb = bbname_graph_map[cc->bbname];

    string cc_code = encode_candidate_component(cc, bb);
    assert(cc_code.size() == code.size());

    if (!contains(cc_code, code))
    {
        code_per_bb.clear();
        gids.clear();
        return;
    }

    vector<int> new_vertex_uids = get_new_vertex_uids(code, cc_code, cc, bb);
    auto graph_visit_map = build_visit_vector(*patt);

    if (new_vertex_uids.size() == 0)
    {
        for (Graph *g : *patt)
        {
            Graph *g_bb = bbname_graph_map[g->bbname];
            code_per_bb.push_back(encode_candidate_component(g, g_bb));
        }
        for (int j = 0; j < patt->size(); j++)
        {
            gids.push_back(j);
        }
        return;
    }

    if (force_connect)
    {
        std::vector<int> tmp_new_vertex_uids = new_vertex_uids;
        std::vector<int> tmp_vertex_uids;
        for (auto uid : cc->vertices)
        {
            tmp_vertex_uids.push_back(uid);
        }

        int tmp = 0;

        while (true)
        {
            int checkDepControl = 0;
            std::vector<int> id_to_remove;
            int sz = tmp_new_vertex_uids.size();
            for (int i = 0; i < tmp_new_vertex_uids.size(); i++)
            {
                int bb_uid = tmp_new_vertex_uids[i];
                int idx = check_dep(tmp_vertex_uids, bb, bb_uid);
                if (idx != -1)
                {
                    checkDepControl++;
                    tmp_vertex_uids.push_back(bb_uid);
                    id_to_remove.push_back(i);
                    tmp++;
                }
            }

            for (int i = 0; i < id_to_remove.size(); i++)
            {
                int x = id_to_remove[i] - i;
                tmp_new_vertex_uids.erase(tmp_new_vertex_uids.begin() + x);
            }

            if (checkDepControl == 0)
                break;
        }

        if (tmp != new_vertex_uids.size())
        {
            code_per_bb.clear();
            gids.clear();
            return;
        }
    }

    compute_support(code_per_bb, gids, *patt, new_vertex_uids, graph_visit_map, gid);
    // std::cout<<"after compute_support\n";
}

int PatternManager::check_dep(std::vector<int> vertex_uids,
                              Graph *graph,
                              int new_uid)
{
    std::vector<Instruction *> bb_insns;
    std::vector<Instruction *> subgraph_insns;

    for (int uid : graph->vertices)
    {
        Instruction *insn = uid_insn_map[uid];
        bb_insns.push_back(insn);
    }

    for (int uid : vertex_uids)
    {
        Instruction *insn = uid_insn_map[uid];
        subgraph_insns.push_back(insn);
    }

    for (Instruction *I : subgraph_insns)
    {
        for (auto &Use : I->uses())
        {
            if (Instruction *user_insn = dyn_cast<Instruction>(Use.getUser()))
            {
                // Check if the user is not in InstructionsToMove
                if (count(bb_insns.begin(), bb_insns.end(), user_insn))
                {
                    if (insn_uid_map[user_insn] == new_uid)
                    {
                        return new_uid;
                    }
                }
            }
        }
    }

    for (Instruction *I : subgraph_insns)
    {
        for (unsigned i = 0; i < I->getNumOperands(); ++i)
        {
            Value *op = I->getOperand(i);
            if (Instruction *op_insn = dyn_cast<Instruction>(op))
            {
                if (count(bb_insns.begin(), bb_insns.end(), op_insn))
                {
                    if (insn_uid_map[op_insn] == new_uid)
                    {
                        return new_uid;
                    }
                }
            }
        }
    }

    return -1;
}

void PatternManager::compute_support(vector<string> &code_per_bb,
                                     vector<int> &gids,
                                     vector<Graph *> graphs,
                                     vector<int> new_vertex_uids,
                                     map<Graph *, vector<int>> &graph_visit_map, int gid)
{
    Graph *x_cc = graphs[gid];
    Graph *x_bb = bbname_graph_map[x_cc->bbname];
    auto &x_vis = graph_visit_map[x_cc];
    string x_code(x_bb->vertices.size(), '0');

    for (int uid : x_cc->vertices)
    {
        int vid = x_bb->uid_vid_map[uid];
        x_vis[vid] = 1;
        x_code.replace(vid, 1, "1");
    }

    for (int uid : new_vertex_uids)
    {
        int vid = x_bb->uid_vid_map[uid];
        x_vis[vid] = 1;
        x_code.replace(vid, 1, "1");
    }
    vector<int> prev_visit = x_vis;

    code_per_bb.push_back(x_code);
    gids.push_back(gid);

    for (int j = 0; j < graphs.size(); j++)
    {
        // std::cout << "compute_support j=" << j << "\n";
        Graph *cc = graphs[j];
        Graph *bb = bbname_graph_map[cc->bbname];

        auto &vis = graph_visit_map[cc];

        bool skip = false;
        Graph *prev_cc = nullptr;

        if (j > 0)
        {
            prev_cc = graphs[j - 1];
        }
        else
        {
            prev_cc = x_cc;
        }

        if (cc->bbname == prev_cc->bbname &&
            vis.size() == prev_visit.size())
        {
            for (int i = 0; i < vis.size(); i++)
            {
                // vertex of this graph is added to another graph
                // skip this graph
                if (vis[i] == 1 && (vis[i] == prev_visit[i]))
                {
                    skip = true;
                }

                // set vertex visited in previous cc
                if (prev_visit[i] == 1)
                    vis[i] = 1;
            }
        }

        if (j == gid)
        {
            prev_visit = vis;
            continue;
        }

        // std::cout<<"before skip\n";

        vector<int> vertex_added_vids;
        if (!skip)
        {
            vertex_added_vids = compute_graph_similarity(cc, bb, cc->vertices, new_vertex_uids, vis);
            if (vertex_added_vids.size() == new_vertex_uids.size())
            {
                string code(bb->vertices.size(), '0');
                for (auto vid : vertex_added_vids)
                {
                    code.replace(vid, 1, "1");
                }
                for (auto uid : cc->vertices)
                {
                    int vid = bb->uid_vid_map[uid];
                    code.replace(vid, 1, "1");
                }
                code_per_bb.push_back(code);
                gids.push_back(j);
            }
        }

        // std::cout<<"after skip\n";

        if (skip || vertex_added_vids.size() != new_vertex_uids.size())
        {
            gids.push_back(-1);
            code_per_bb.push_back("");
        }

        prev_visit = vis;
    }
}

vector<int> PatternManager::compute_graph_similarity(
    Graph *cc,
    Graph *bb,
    vector<int> cc_vertices,
    vector<int> new_vertex_uids,
    vector<int> &vis)
{
    assert(new_vertex_uids.size() != 0);

    int control = 0;
    vector<int> tmp_vertices;
    vector<int> recover_ids;
    vector<int> found(new_vertex_uids.size(), 0);

    vector<int> vids;

    for (auto uid : cc_vertices)
    {
        tmp_vertices.push_back(uid);
    }

    while (true)
    {
        control = 0;

        for (int j = 0; j < new_vertex_uids.size(); j++)
        {
            if (found[j] == 1)
                continue;

            int new_uid = new_vertex_uids[j];
            Instruction *new_insn = uid_insn_map[new_uid];

            for (int i = 0; i < bb->vertices.size(); i++)
            {
                if (vis[i])
                    continue;

                int bb_uid = bb->vertices[i];
                Instruction *bb_insn = uid_insn_map[bb_uid];

                if (new_insn->getOpcode() == bb_insn->getOpcode())
                {
                    bool should_break = false;

                    if (force_connect)
                    {
                        if (check_dep(tmp_vertices, bb, bb_uid) != -1)
                        {
                            should_break = true;
                        }
                    }
                    else
                    {
                        should_break = true;
                    }

                    if (should_break)
                    {
                        control++;
                        found[j] = 1;
                        tmp_vertices.push_back(bb_uid);
                        vis[i] = 1;
                        recover_ids.push_back(i);
                        vids.push_back(i);
                        break;
                    }
                }
            }
        }

        if (control == 0)
            break;
    }

    bool recover = count(found.begin(), found.end(), 1) != new_vertex_uids.size();
    assert(count(found.begin(), found.end(), 1) == vids.size());

    if (recover)
    {
        for (int id : recover_ids)
        {
            vis[id] = 0;
        }
    }

    return vids;
}

map<Graph *, std::vector<int>> PatternManager::build_visit_vector(vector<Graph *> graphs)
{
    map<Graph *, std::vector<int>> visited;

    for (int i = 0; i < graphs.size(); i++)
    {
        Graph *cc = graphs[i];
        Graph *bb = bbname_graph_map[cc->bbname];

        vector<int> vis = vector<int>(bb->vertices.size(), 0);
        for (int uid : cc->vertices)
        {
            int vid = bb->uid_vid_map[uid];
            vis[vid] = 1;
        }

        visited.insert(make_pair(cc, vis));
    }

    return visited;
}

bool PatternManager::check_ops_unkown_type(string code, int pid, int gid)
{
    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    Graph *cc = (*patterns[pid])[gid];
    Graph *bb = bbname_graph_map[cc->bbname];
    Graph *subgraph = build_subgraph(code, bb);

    Graph *clean_subgraph = rm_graph_ops(ops, subgraph);

    for (int uid : clean_subgraph->vertices)
    {
        Instruction *I = uid_insn_map[uid];

        for (unsigned i = 0; i < I->getNumOperands(); ++i)
        {
            Value *Op = I->getOperand(i);

            auto op_type = Op->getType();
            if (!op_type->isSized())
            {
                return true;
            }
        }
    }

    return false;
}
vector<int> PatternManager::get_io_number(string code, int pid, int gid)
{
    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};
    Graph *cc = (*patterns[pid])[gid];
    Graph *bb = bbname_graph_map[cc->bbname];
    Graph *subgraph = build_subgraph(code, bb);

    Graph *clean_subgraph = rm_graph_ops(ops, subgraph);
    return compute_io_number(clean_subgraph);
}

vector<int> PatternManager::compute_io_number(Graph *graph)
{
    vector<Instruction *> insns;
    for (int uid : graph->vertices)
    {
        insns.push_back(uid_insn_map[uid]);
    }

    set<Instruction *> out_set;
    for (Instruction *I : insns)
    {
        bool is_out = false;

        for (auto &Use : I->uses())
        {
            if (Instruction *user_insn = dyn_cast<Instruction>(Use.getUser()))
            {
                // Check if the user is not in InstructionsToMove
                if (std::find(insns.begin(), insns.end(), user_insn) == insns.end())
                {
                    is_out = true;
                    break;
                }
            }
        }

        if (is_out)
        {
            out_set.insert(I);
        }
    }

    set<Instruction *> in_set;
    for (Instruction *I : insns)
    {
        for (unsigned i = 0; i < I->getNumOperands(); ++i)
        {
            Value *op = I->getOperand(i);
            if (Instruction *op_insn = dyn_cast<Instruction>(op))
            {
                if (!count(insns.begin(), insns.end(), op_insn) &&
                    in_set.find(op_insn) == in_set.end())
                {
                    in_set.insert(op_insn);
                }
            }
        }
    }

    return {in_set.size(), out_set.size()};
}

bool PatternManager::get_convexity(string code, int pid, int gid)
{
    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    Graph *cc = (*patterns[pid])[gid];
    Graph *bb = bbname_graph_map[cc->bbname];

    Graph *subgraph = build_subgraph(code, bb);

    Graph *clean_subgraph = rm_graph_ops(ops, subgraph);
    bool convex = dfs_check_convex(clean_subgraph);

    return convex;
}

double PatternManager::get_graph_cp_delay(string code, int pid, int gid)
{
    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    Graph *cc = (*patterns[pid])[gid];
    Graph *bb = bbname_graph_map[cc->bbname];

    Graph *subgraph = build_subgraph(code, bb);

    Graph *clean_subgraph = rm_graph_ops(ops, subgraph);

    return compute_graph_cp_delay(clean_subgraph);
}

int PatternManager::get_stage_delay(string code, int pid, int gid)
{
    Graph *cc = (*patterns[pid])[gid];
    Graph *bb = bbname_graph_map[cc->bbname];

    Graph *subgraph = build_subgraph(code, bb);

    Graph *clean_subgraph = rm_graph_ops(ops, subgraph);
    DependencyGraph dep_graph = build_data_dependency_graph(clean_subgraph);

    double max_delay = 0.;
    vector<uint64_t> cp_op_uids;

    for (auto uid : clean_subgraph->vertices)
    {
        vector<uint64_t> tmp_cp_op_uids;
        double delay = get_cp_ops(uid, clean_subgraph, tmp_cp_op_uids, dep_graph);
        if (delay >= max_delay)
        {
            max_delay = delay;
            cp_op_uids = tmp_cp_op_uids;
        }
    }

    int max_lat = 0;
    for (auto uid : cp_op_uids)
    {
        Instruction *insn = uid_insn_map[uid];
        int lat = (int)get_hw_delay(insn->getOpcode());
        max_lat = max(max_lat, lat);
    }

    return max_lat;
}

double PatternManager::get_graph_seq_delay(string code, int pid, int gid)
{
    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    Graph *cc = (*patterns[pid])[gid];
    Graph *bb = bbname_graph_map[cc->bbname];

    Graph *subgraph = build_subgraph(code, bb);

    Graph *clean_subgraph = rm_graph_ops(ops, subgraph);

    return compute_graph_seq_delay(clean_subgraph);
}

double PatternManager::compute_graph_cp_delay(Graph *graph)
{
    std::vector<Instruction *> instructions;

    for (int j = 0; j < graph->vertices.size(); j++)
    {
        int uid = graph->vertices[j];
        Instruction *insn = uid_insn_map[uid];
        instructions.push_back(insn);
    }

    DependencyGraph dependency_graph = build_data_dependency_graph(instructions);
    return find_critical_path(dependency_graph, instructions);
}

double PatternManager::compute_graph_seq_delay(Graph *graph)
{
    double delay = 0.;
    for (int uid : graph->vertices)
    {
        Instruction *insn = uid_insn_map[uid];
        delay += get_hw_delay(insn->getOpcode());
    }
    return delay;
}

// Function to compute the critical path length
double PatternManager::find_critical_path(DependencyGraph &graph, vector<llvm::Instruction *> &instructions)
{
    map<llvm::Instruction *, double> longest_path_length;
    map<llvm::Instruction *, int> in_degree;
    queue<llvm::Instruction *> zero_in_degree_queue;

    // Initialize in-degree and longest path lengths
    for (auto insn : instructions)
    {
        in_degree[insn] = 0;                                         // Initialize in-degree of each instruction to 0
        longest_path_length[insn] = get_hw_delay(insn->getOpcode()); // Initialize longest path length
    }

    // Calculate in-degrees
    for (auto insn : instructions)
    {
        auto dependents = graph[insn];
        for (auto *dependent : dependents)
        {
            in_degree[dependent]++;
        }
    }

    // Enqueue all instructions with in-degree 0 (no dependencies)
    for (auto insn : instructions)
    {
        int degree = in_degree[insn];
        if (degree == 0)
        {
            zero_in_degree_queue.push(insn);
        }
    }

    // Topological sort and compute the longest path
    while (!zero_in_degree_queue.empty())
    {
        llvm::Instruction *current = zero_in_degree_queue.front();
        zero_in_degree_queue.pop();

        // Process each dependent instruction
        for (auto *dependent : graph.at(current))
        {
            // Update the longest path length for the dependent instruction
            longest_path_length[dependent] = std::max(longest_path_length[dependent],
                                                      longest_path_length[current] +
                                                          get_hw_delay(dependent->getOpcode()));

            // Decrease in-degree and enqueue if it becomes 0
            if (--in_degree[dependent] == 0)
            {
                zero_in_degree_queue.push(dependent);
            }
        }
    }

    // Find the maximum length among all instructions
    double max_path_length = 0;
    for (const auto &[insn, length] : longest_path_length)
    {
        max_path_length = std::max(max_path_length, length);
    }

    return max_path_length;
}

int PatternManager::get_graph_width(string code, int pid, int gid)
{
    vector<string> ops = {"input", "output",
                          "store",
                          "load",
                          "ret",
                          "phi",
                          "br", "alloca", "call", "invoke", "landingpad"};

    Graph *cc = (*patterns[pid])[gid];
    Graph *bb = bbname_graph_map[cc->bbname];
    Graph *subgraph = build_subgraph(code, bb);
    Graph *clean_subgraph = rm_graph_ops(ops, subgraph);
    return compute_graph_width(clean_subgraph);
}

int PatternManager::compute_graph_width(Graph *graph)
{
    std::vector<Instruction *> instructions;

    for (int j = 0; j < graph->vertices.size(); j++)
    {
        int uid = graph->vertices[j];
        Instruction *insn = uid_insn_map[uid];
        instructions.push_back(insn);
    }

    DependencyGraph dependency_graph = build_data_dependency_graph(instructions);
    return find_graph_width(dependency_graph, instructions);
}

int PatternManager::find_graph_width(DependencyGraph &graph, vector<llvm::Instruction *> &instructions)
{
    map<llvm::Instruction *, int> in_degree;
    queue<Instruction *> q;
    set<Instruction *> visit;

    for (auto insn : instructions)
    {
        in_degree[insn] = 0; // Initialize in-degree of each instruction to 0
    }

    for (auto insn : instructions)
    {
        auto dependents = graph[insn];
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
            q.push(insn);
            visit.insert(insn);
        }
    }

    int max_width = 0;

    while (!q.empty())
    {
        int level_size = q.size();
        max_width = max(max_width, level_size);

        for (int i = 0; i < level_size; ++i)
        {
            Instruction *insn = q.front();
            q.pop();

            auto dependents = graph[insn];
            for (auto *dependent : dependents)
            {
                if (!visit.count(dependent))
                {
                    visit.insert(dependent);
                    q.push(dependent);
                }
            }
        }
    }
    return max_width;
}

// Bron-Kerbosch
// d为搜索深度，an、sn、nn分别为all(R)、some(P)、none(X)集合中顶点数，
void BK(int d, int an, int sn, int nn, int **some, int **none, int **all, int **mp, vector<vector<int>> &indep_sets, int N)
{
    // sn==0，nn==0时，是一个极大团，S为极大团数量
    if (!sn && !nn)
    {
        vector<int> iset;
        for (int i = 0; i < an; i++)
        {
            iset.push_back(all[d][i]);
        }

        if (iset.size())
        {
            indep_sets.push_back(iset);
        }
    }

    if (d + 1 >= N)
        return;

    int u = some[d][0];
    for (int i = 0; i < sn; ++i) // 遍历P中的结点，sn==0时，搜索到终点
    {
        int v = some[d][i]; // 取出P中的第i个结点

        if (mp[u][v])
            continue;

        // 这里是将取出的v结点，添加到下一个深度的R集合。
        for (int j = 0; j < an; ++j)
        {
            all[d + 1][j] = all[d][j];
        }
        all[d + 1][an] = v;

        // 用来分别记录下一层中P集合和X集合中结点的个数
        int tsn = 0, tnn = 0;

        // 更新P集合(下一层的P集合)，保证P集合中的点都能与R集合中所有的点相连接
        for (int j = 0; j < sn; ++j)
            if (mp[v][some[d][j]])
                some[d + 1][tsn++] = some[d][j];

        // 更新X集合(下一层的X集合)，保证X集合中的点都能与R集合中所有的点相连接
        for (int j = 0; j < nn; ++j)
            if (mp[v][none[d][j]])
                none[d + 1][tnn++] = none[d][j];

        // 递归进入下一层
        BK(d + 1, an + 1, tsn, tnn, some, none, all, mp, indep_sets, N);

        // 完成后，将操作的结点，放入X中，开始下面的寻找。
        some[d][i] = -1;
        none[d][nn++] = v;
    }
}

void PatternManager::mia(vector<Graph *> graphs, vector<string> bblist)
{
    int N = patterns.size();

    map<string, vector<Graph *>> bb_graphs_map;

#if 0
    int pid = 0;
    double max_score = 0.;
    int max_score_pid = 0;
    for (Pattern *pattern : patterns)
    {
        for (Graph *cc : *pattern)
        {
            if (bb_graphs_map.find(cc->bbname) == bb_graphs_map.end())
            {
                bb_graphs_map.insert(make_pair(cc->bbname, vector<Graph *>()));
            }
            bb_graphs_map[cc->bbname].push_back(cc);
            cc->pid = pid;

            if (cc->score > max_score)
            {
                max_score = cc->score;
                max_score_pid = pid;
            }
        }
        pid++;
    }
#endif

#if 1
    int **adj_mat = build_conflict_graph(bb_graphs_map);

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

    // get complement graph
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            cout << adj_mat[i][j] << " ";
            if (i != j)
                adj_mat[i][j] = adj_mat[i][j] == 1 ? 0 : 1;
        }
        cout << "\n";
    }

    vector<vector<int>> indep_sets;
    BK(0, 0, N, 0, some, none, all, adj_mat, indep_sets, N);

    for (int i = 0; i < N; i++)
    {
        indep_sets.push_back({i});
    }

    vector<int> max_score_set = get_max_score_indep_set(indep_sets);
#endif

#if 0
    // only save max score pattern in a indepedent set
    double max_score;
    int max_pid = -1;
    for (int pid : max_score_set)
    {
        auto pattern = patterns[pid];
        double patt_score = (*pattern)[0]->score;
        if (patt_score > max_score)
        {
            max_score = patt_score;
            max_pid = pid;
        }
    }
    assert(max_pid != -1);
    max_score_set.clear();
    max_score_set.push_back(max_pid);
#endif

#if 0
    // print bk results
    for (int i = 0; i < indep_sets.size(); i++)
    {
        auto v = indep_sets[i];

        for (auto x : v)
        {
            cout << x << " ";
        }
        cout << "\n";
    }
    
    for (auto x : max_score_set)
    {
        cout << x << " ";
    }
    cout << "\n";
#endif

    // vector<int> max_score_set = {max_score_pid};

    bb_graphs_map.clear();
    for (int pid : max_score_set)
    {
        Pattern *pattern = patterns[pid];
        for (Graph *g : *pattern)
        {
            if (bb_graphs_map.find(g->bbname) == bb_graphs_map.end())
            {
                bb_graphs_map.insert(make_pair(g->bbname, vector<Graph *>()));
            }
            bb_graphs_map[g->bbname].push_back(g);
        }

        vector<Graph *> to_dump_graphs;
        for (Graph *g : *pattern)
        {
            Graph *clean_subgraph = rm_graph_ops(ops, g);
            if (clean_subgraph->vertices.size() > 1)
                to_dump_graphs.push_back(clean_subgraph);
        }

        dump_graph_data("mia_output/patt_" + to_string(pid) + ".data", to_dump_graphs);
    }

    int dump_id = 0;
    vector<Graph *> subgraphs;
    for (auto iter = bb_graphs_map.begin(); iter != bb_graphs_map.end(); iter++)
    {
        Graph *bb = bbname_graph_map[iter->first];
        string visit(bb->vertices.size(), '0');

        for (Graph *graph : iter->second)
        {
            string code = encode_candidate_component(graph, bb);
            std::cout << "code=" << code << " visit=" << visit << " ";
            visit = bitwise_or(visit, code);
            cout << " or=" << visit << "\n";
        }
        string left_insn_code = bitwise_reverse(visit);
        cout << "left_insn_code=" << left_insn_code << "\n";

        Graph *left_subgraph = build_subgraph(left_insn_code, bb);

        if (left_subgraph->vertices.size() > 1)
            subgraphs.push_back(left_subgraph);
    }
    dump_graph_data("mia_output/left.data", subgraphs);

    vector<Graph *> graphs_to_dump;
    for (Graph *graph : graphs)
    {
        if (count(bblist.begin(), bblist.end(), graph->bbname))
        {
            if (bb_graphs_map.find(graph->bbname) == bb_graphs_map.end())
            {
                graphs_to_dump.push_back(graph);
            }
        }
    }
    dump_graph_data("mia_output/rest.data", graphs_to_dump);

#if 0
    for (int i = 0; i < N; i++)
    {
        free(adj_mat[i]);
        free(some[i]);
        free(all[i]);
        free(none[i]);
    }
    free(adj_mat);
    free(some);
    free(all);
    free(none);
#endif
}

int **PatternManager::build_conflict_graph(map<string, vector<Graph *>> bb_graphs_map)
{
    int N = patterns.size();
    int **adj_mat = (int **)malloc(sizeof(int *) * N);

    for (int i = 0; i < N; i++)
    {
        adj_mat[i] = (int *)malloc(sizeof(int) * N);
        memset(adj_mat[i], 0, N * sizeof(int));
    }

    for (auto iter = bb_graphs_map.begin(); iter != bb_graphs_map.end(); iter++)
    {
        vector<Graph *> graphs = iter->second;
        for (int i = 0; i < graphs.size(); i++)
        {
            Graph *g1 = graphs[i];
            for (int j = i + 1; j < graphs.size(); j++)
            {
                Graph *g2 = graphs[j];
                if (g1->pid != g2->pid)
                {
                    if (check_graph_overlap(g1, g2))
                    {
                        adj_mat[g1->pid][g2->pid] = 1;
                        adj_mat[g2->pid][g1->pid] = 1;
                    }
                }
            }
        }
    }

    return adj_mat;
}

bool PatternManager::check_graph_overlap(Graph *g1, Graph *g2)
{
    set<int> uid_set(g1->vertices.begin(), g1->vertices.end());

    // std::cout << "b1=" << g1->bbname << " id=" << g1->pid << " b2=" << g2->bbname << " id=" << g2->pid << "\n";

    for (int uid : g2->vertices)
    {
        if (uid_set.count(uid))
        {
            // std::cout<<"overlap at "<<uid<<"\n";
            return true;
        }
    }

    return false;
}

double PatternManager::score(Graph *graph)
{
    double cp_delay = compute_graph_cp_delay(graph) * 0.99;
    double seq_delay = compute_graph_seq_delay(graph);

    uint64_t bb_exe_count = bbname_graph_map[graph->bbname]->itervalue;

    double width = compute_graph_width(graph);
    double graph_score = (seq_delay - cp_delay) * bb_exe_count * width;

    return graph_score;
}

double PatternManager::score(Pattern *pattern)
{
    double patt_score = 0.;
    for (Graph *graph : *pattern)
    {
        Graph *clean_subgraph = rm_graph_ops(ops, graph);
        double graph_score = score(clean_subgraph);

        patt_score += graph_score;
    }

    return patt_score;
}

vector<int> PatternManager::get_max_score_indep_set(vector<vector<int>> indep_sets)
{
    vector<int> ret;
    double max_score = 0.;
    for (int i = 0; i < indep_sets.size(); i++)
    {
        auto iset = indep_sets[i];
        double iset_score = 0.;

        for (auto pid : iset)
        {
            Pattern *pattern = patterns[pid];
            // double patt_score = score(pattern);
            double patt_score = (*pattern)[0]->score;
            // for (Graph *g : *pattern)
            // {
            //     patt_score += g->score;
            // }
            std::cout << "pid=" << pid << " score=" << patt_score << "\n";
            iset_score += patt_score;
        }

        if (iset_score > max_score)
        {
            ret = iset;
            max_score = iset_score;
        }
    }

    return ret;
}

void PatternManager::dump_dfg(Graph *g, ofstream &file)
{
    file << "digraph \"DFG for New Instruction\" {\n";

    for (int uid : g->vertices)
    {
        string uid_str = to_string(uid);
        file << "\tNode" << uid_str << "[shape=record, label=\"" << uid_insn_map[uid]->getOpcodeName() << "\"];\n";
    }

    std::vector<Instruction *> instructions;

    for (int uid : g->vertices)
    {
        Instruction *insn = uid_insn_map[uid];
        instructions.push_back(insn);
    }

    DependencyGraph dependency_graph = build_data_dependency_graph(instructions);

    for (Instruction *insn : instructions)
    {
        string frm_uid_str = to_string(insn_uid_map[insn]);

        auto dependents = dependency_graph[insn];
        for (auto *dependent : dependents)
        {
            string to_uid_str = to_string(insn_uid_map[dependent]);
            file << "\tNode" << frm_uid_str << " -> Node" << to_uid_str << "\n";
        }
    }

    file << "}\n";
}

double PatternManager::get_cp_ops(uint64_t uid, Graph *g, vector<uint64_t> &cp_ops, DependencyGraph dep_graph)
{
    double ret = 0.;

    Instruction *insn = uid_insn_map[uid];
    double delay = get_hw_delay(insn->getOpcode());

    ret = delay;
    vector<uint64_t> child_cp_uids;

    for (auto *dependent : dep_graph[insn])
    {
        if (isa<PHINode>(dependent))
            continue;

        uint64_t dep_uid = insn_uid_map[dependent];

        vector<uint64_t> tmp_child_cp_uids;
        auto child_delay = get_cp_ops(dep_uid, g, tmp_child_cp_uids, dep_graph);
        if (child_delay + delay > ret)
        {
            ret = child_delay + delay;
            child_cp_uids = tmp_child_cp_uids;
        }
    }
    cp_ops.push_back(uid);
    cp_ops.insert(cp_ops.end(), child_cp_uids.begin(), child_cp_uids.end());

    return ret;
}

vector<int> PatternManager::get_cc_uids(string code, Graph *bb)
{
    vector<int> uids;

    vector<string> ops = {"input", "output",
                          "store",
                          "phi",
                          "load",
                          "ret",
                          "br", "alloca", "call", "invoke", "landingpad"};

    Graph *subgraph = build_subgraph(code, bb);
    Graph *clean_subgraph = rm_graph_ops(ops, subgraph);

    for (int uid : clean_subgraph->vertices)
    {
        uids.push_back(uid);
    }

    return uids;
}

int **PatternManager::get_adj_matrix(Graph *g)
{
    int N = g->vertices.size();
    int **adjMat = (int **)malloc(sizeof(int *) * N);
    for (int i = 0; i < N; i++)
    {
        adjMat[i] = (int *)malloc(sizeof(int) * N);
        memset(adjMat[i], 0, N * sizeof(int));
    }

    std::vector<Instruction *> instructions;

    for (int j = 0; j < g->vertices.size(); j++)
    {
        int uid = g->vertices[j];
        Instruction *insn = uid_insn_map[uid];

        instructions.push_back(insn);
    }

    DependencyGraph dependency_graph = build_data_dependency_graph(instructions);

    for (auto &instruction : instructions)
    {
        auto dependents = dependency_graph[instruction];
        for (auto *dependent : dependents)
        {
            int frmID = g->uid_vid_map[insn_uid_map[instruction]];
            int toID = g->uid_vid_map[insn_uid_map[dependent]];

            adjMat[frmID][toID] = 1;
            adjMat[toID][frmID] = 1;
        }
    }
    return adjMat;
}

void PatternManager::dfs_conn(int v, vector<int> &visited, int **adjMat, vector<int> &subgraph)
{
    int n = visited.size();
    visited[v] = true;

    for (int j = 0; j < n; j++)
    {
        if (adjMat[v][j] && !visited[j])
        {
            subgraph.push_back(j);
            dfs_conn(j, visited, adjMat, subgraph);
        }
    }
}

vector<string> PatternManager::find_connected_componet(Graph *g)
{
    vector<string> codes;

    int n = g->vertices.size();
    vector<int> visited(n, 0);

    int **adjMat = get_adj_matrix(g);
    // for (int i = 0; i < n; i++)
    // {
    //     for (int j = 0; j < n; j++)
    //     {
    //         cout << adjMat[i][j] << " ";
    //     }
    //     cout << "\n";
    // }

    for (int i = 0; i < n; i++)
    {
        if (!visited[i])
        {
            vector<int> ret;
            ret.push_back(i);
            dfs_conn(i, visited, adjMat, ret);

            string code = string(n, '0');
            // cout<<"i="<<i<<"\n";
            for (auto j : ret)
            {
                code[j] = '1';
                // cout << " ret j=" << j << "\n";
            }

            // cout << "n=" << n << " code=" << code << "\n";
            codes.push_back(code);
        }
    }

    for (int i = 0; i < n; i++)
    {
        free(adjMat[i]);
    }
    free(adjMat);

    return codes;
}