#include "cost_model.h"
#include "../mining/utils.h"
#include "packing.h"

#include "yaml_parser.h"

#include <limits>

void CostModel::build_rob()
{
    resource_names.insert(make_pair("rob", vector<string>()));
    for (int i = 0; i < rob_size; i++)
    {
        resource_names["rob"].push_back("rob_" + to_string(i));
    }
}

void CostModel::build_issueq()
{
    resource_names.insert(make_pair("issueq", vector<string>()));
    for (int i = 0; i < issueq_size; i++)
    {
        resource_names["issueq"].push_back("issueq_" + to_string(i));
    }
}

void CostModel::build_ldstq()
{
    resource_names.insert(make_pair("ldq", vector<string>()));
    for (int i = 0; i < ldq_size; i++)
    {
        resource_names["ldq"].push_back("ldq_" + to_string(i));
    }

    resource_names.insert(make_pair("stq", vector<string>()));
    for (int i = 0; i < stq_size; i++)
    {
        resource_names["stq"].push_back("stq_" + to_string(i));
    }
}

void CostModel::build_issue_ports()
{
    Params *params = new Params();
    loadConfig(config_file, params);

    InstConfig *pack_config = nullptr;
    InstConfig *cstm_config = nullptr;

#if 0 // pack using new ports
    // add a port for packing
    // opcode is 1023
    int port_id = params->portConfigs.size();
    pack_config = new InstConfig();
    pack_config->opcode_num = 1023;
    pack_config->ports = {{1, {port_id}}};
    pack_config->fus = {{"pack"}};
    params->portConfigs[port_id] = {"pack"};

    // add a port for custom op
    // opcode is 1024
    cstm_config = new InstConfig();
    cstm_config->opcode_num = 1024;
    cstm_config->ports = {{1, {port_id + 1}}};
    cstm_config->fus = {{"custom"}};
    params->portConfigs[port_id + 1] = {"custom"};
#endif

    // pack using existing ports
    // opcode is 1023
    pack_config = new InstConfig();
    pack_config->opcode_num = 1023;
    pack_config->ports = {{1, {3, 4}}};
    pack_config->fus = {{"PACK"}};

    // add a port for custom op
    // opcode is 1025
    int port_id = params->portConfigs.size();
    cstm_config = new InstConfig();
    cstm_config->opcode_num = 1025;
    cstm_config->ports = {{1, {port_id}}};
    cstm_config->fus = {{"CUSTOM"}};
    params->portConfigs[port_id] = {"CUSTOM"};

    params->notPipelineFUs.push_back("CUSTOM");

    map<string, int> fu_count;

    resource_names.insert(make_pair("issue_ports", vector<string>()));

    for (auto iter = params->portConfigs.begin(); iter != params->portConfigs.end(); iter++)
    {
        string res_issue_port = "issue_port_" + to_string(iter->first);
        resource_names["issue_ports"].push_back(res_issue_port);
        port_fus[res_issue_port] = vector<string>();

        for (string fu : iter->second)
        {
            if (resource_names.find(fu) == resource_names.end())
            {
                resource_names.insert(make_pair(fu, vector<string>()));
                fu_count[fu] = 0;
            }

            string res_fu = fu + "_" + to_string(fu_count[fu]);
            resource_names[fu].push_back(res_fu);
            port_fus[res_issue_port].push_back(res_fu);

            fu_port_map[res_fu] = res_issue_port;

            fu_count[fu] += 1;
        }
    }

    insn_config = params->insnConfigs;
    rob_size = params->robSize;
    issueq_size = params->issueQSize;
    ldq_size = params->ldqSize;
    stq_size = params->stqSize;
    pipe_width = params->pipe_width;
    not_pipeline_fus = params->notPipelineFUs;

    insn_config[1023] = pack_config;
    insn_config[1025] = cstm_config;

    for (auto iter = insn_config.begin(); iter != insn_config.end(); iter++)
    {
        int opcode = iter->first;
        double lat = iter->second->runtime_cycles * 1.0;
        opcode_delay[opcode] = lat;
    }
}

void CostModel::build_rdwt_ports()
{
    // read ports
    for (int i = 0; i < read_ports; i++)
    {
        if (i == 0)
        {
            resource_names.insert(make_pair("read_ports", vector<string>()));
        }

        resource_names["read_ports"].push_back("read_ports_" + to_string(i));
    }

    // write ports
    for (int i = 0; i < write_ports; i++)
    {
        if (i == 0)
        {
            resource_names.insert(make_pair("write_ports", vector<string>()));
        }

        resource_names["write_ports"].push_back("write_ports_" + to_string(i));
    }

    // rdwt ports
    for (int i = 0; i < rdwt_ports; i++)
    {
        resource_names["read_ports"].push_back("rdwt_ports_" + to_string(i));
        resource_names["write_ports"].push_back("rdwt_ports_" + to_string(i));
    }
}

CostModel::CostModel(string config_file) : config_file(config_file)
{
    build_rdwt_ports();
    build_issue_ports();
    build_rob();
    build_issueq();
    build_ldstq();

    // for (auto iter = resource_names.begin(); iter != resource_names.end(); iter++)
    // {
    //     cout << iter->first << "\n";
    //     for (auto name : iter->second)
    //     {
    //         cout << "   name=" << name << "\n";
    //     }
    // }

    // for (auto iter = port_fus.begin(); iter != port_fus.end(); iter++)
    // {
    //     cout << "port " << iter->first << "\n";
    //     for (auto fu : iter->second)
    //     {
    //         cout << "   fu=" << fu << "\n";
    //     }
    // }
}

uint64_t ArchGraph::test_issue_cycle(Instruction *insn, int id)
{
    uint64_t ret = 0;

    uint64_t pipe_cycle = 0;
    if (id - pipe_width >= 0)
    {
        ArchNode *pred_fetch_node = get_fetch_node(id - pipe_width);
        pipe_cycle = pred_fetch_node->start_cycle + 3; // from fetch to port
    }
    ret = max(ret, pipe_cycle);
    // cout << "pipe_cycle=" << pipe_cycle << "\n";

    string rob_found = "";
    uint64_t rob_cycle = 0;
    find_rob_entry(rob_found, rob_cycle);
    ret = max(ret, rob_cycle);
    // cout << "rob_cycle=" << rob_cycle << "\n";

    string issueq_found = "";
    uint64_t issueq_cycle = 0;
    find_issueq_entry(issueq_found, issueq_cycle);
    ret = max(ret, issueq_cycle);

    if (insn->getOpcodeName() == "load")
    {
        string ldq_found = "";
        uint64_t ldq_cycle = 0;
        find_ldq_entry(ldq_found, ldq_cycle);
        ret = max(ret, ldq_cycle);
    }

    if (insn->getOpcodeName() == "store")
    {
        string stq_found = "";
        uint64_t stq_cycle = 0;
        find_stq_entry(stq_found, stq_cycle);
        ret = max(ret, stq_cycle);
    }

    // data dependency
    uint64_t data_dep_cycle = 0;
    assert(data_deps.find(id) != data_deps.end());

    vector<int> deps = data_deps[id];
    int livein = deps.size();

    for (int op_id : deps)
    {
        ArchNode *pred_issue_node = get_issue_node(op_id);
        if (pred_issue_node)
        {
            data_dep_cycle = max(pred_issue_node->start_cycle + pred_issue_node->duration, data_dep_cycle);
        }
        else
        {
            // cannot find issue node, because there is a data dependency
            data_dep_cycle = UINT_MAX;
        }
    }
    ret = max(data_dep_cycle, ret);
    // cout << "data_dep_cycle=" << data_dep_cycle << "\n";

    // read_port
    uint64_t read_port_cycle = 0;
    set<string> ban;
    for (int i = 0; i < livein; i++)
    {
        string found = round_robin_find_min_usage_res("read_ports", ban);
        if (resource_use_node.find(found) != resource_use_node.end())
        {
            ArchNode *pred_node = resource_use_node[found];
            read_port_cycle = max(read_port_cycle, pred_node->start_cycle + 1);
        }
        ban.insert(found);
    }
    ret = max(ret, read_port_cycle);
    // cout << "read_port_cycle=" << read_port_cycle << "\n";

#if 0
    // issue port
    uint64_t issue_port_cycle = 0;
    auto ports = insn_config[insn->getOpcode()]->ports;
    int ports_n = resource_names["issue_ports"].size();
    ban.clear();
    for (auto iter = ports.begin(); iter != ports.end(); iter++)
    {
        set<string> temp = ban;

        for (int i = 0; i < ports_n; i++)
        {
            if (!count(iter->second.begin(), iter->second.end(), i))
            {
                temp.insert("issue_port_" + to_string(i));
            }
        }
        string found = round_robin_find_min_usage_res("issue_ports", temp);
        if (resource_use_node.find(found) != resource_use_node.end())
        {
            ArchNode *pred_node = resource_use_node[found];
            issue_port_cycle = max(issue_port_cycle, pred_node->start_cycle + port_delay_map[found]);
        }
        ban.insert(found);
    }
    
    ret = max(ret, issue_port_cycle);
#endif

    uint64_t port_fu_cycle = UINT_MAX;
    // uint64_t port_fu_cycle = 0;

    vector<string> found_ports;
    vector<string> found_fus;
    vector<int> found_port_cycles;

    find_min_port_fu(insn, found_ports, found_fus, port_fu_cycle, found_port_cycles);

    ret = max(ret, port_fu_cycle);
    // cout << " port_fu_cycle=" << port_fu_cycle << "\n";

    return ret;
}

int ArchGraph::get_earliest_issue_insn(vector<Instruction *> insns, vector<int> ids)
{
    uint64_t cycle = UINT_MAX;
    int ret = -1;

    for (int i = 0; i < insns.size(); i++)
    {
        uint64_t test_cycle = test_issue_cycle(insns[i], ids[i]);
        // cout << "id=" << ids[i] << " test_cycle=" << test_cycle << " cycle=" << cycle << " insn=" << insn_to_string(insns[i]) << "\n";
        if (test_cycle < cycle)
        {
            cycle = test_cycle;
            ret = i;
        }
    }
    return ret;
}

void ArchGraph::update_commit_edges(vector<int> ids)
{
    for (int id : ids)
    {
        ArchNode *node = get_commit_node(id);
        uint64_t retire_cycle = node->start_cycle + 1;
        if (id == 0)
        {
            node->retire_cycle = retire_cycle;
            continue;
        }

        ArchNode *pred_commit_node = get_commit_node(id - 1);

        // in same retire window
        if (id / pipe_width == (id - 1) / pipe_width)
        {
            retire_cycle = max(pred_commit_node->retire_cycle, retire_cycle);
        }
        else
        {
            retire_cycle = max(pred_commit_node->retire_cycle + 1, retire_cycle);
        }
        node->retire_cycle = retire_cycle;

        critial_path = max(retire_cycle, critial_path);
        // cout << "id=" << id << " retire=" << retire_cycle << "\n";
    }
}

double CostModel::evaluate_o3(vector<Instruction *> insns, int N)
{
    ArchGraph *arch_graph = new ArchGraph();

    arch_graph->pipe_width = pipe_width;
    arch_graph->resource_names = resource_names;
    arch_graph->resource_use_node = resource_use_node;
    arch_graph->port_fus = port_fus;
    arch_graph->fu_port_map = fu_port_map;

    arch_graph->read_ports = read_ports;
    arch_graph->write_ports = write_ports;
    arch_graph->rdwt_ports = rdwt_ports;

    arch_graph->insn_config = insn_config;
    arch_graph->bb_size = insns.size();

    arch_graph->not_pipeline_fus = not_pipeline_fus;

    int size = insns.size();
    for (int i = 0; i < size * N; i += pipe_width)
    {
        vector<Instruction *> insns_to_issue;

        vector<int> ids;
        for (int j = i; j < min(i + pipe_width, size * N); j++)
        {
            Instruction *insn = insns[j % size];
            insns_to_issue.push_back(insn);
            // global idx
            ids.push_back(j);

            // data depdendency should be built in sequence
            arch_graph->insn_uid_map[insn] = j;
            arch_graph->build_data_deps(j, insn);
        }

        vector<int> ids_inorder = ids;

        while (insns_to_issue.size())
        {
            int idx = arch_graph->get_earliest_issue_insn(insns_to_issue, ids);
            // errs() << "idx=" << idx << "\n";
            // errs() << "add node idx=" << ids[idx] << "\n";

            arch_graph->add_nodes(ids[idx], insns_to_issue[idx]);

            // errs() << "add nodes======" << *(insns_to_issue[idx]) << "\n";

            insns_to_issue.erase(insns_to_issue.begin() + idx);
            ids.erase(ids.begin() + idx);
        }

        arch_graph->update_commit_edges(ids_inorder);
    }

    // cout << "critial path=" << arch_graph->critial_path << "\n";

    // ofstream dfg_output;
    // string dfg_file = "arch.dfg";
    // dfg_output.open(dfg_file);
    // arch_graph->visualize(dfg_output);
    // dfg_output.close();

    return arch_graph->critial_path * 1.0;
}

void ArchGraph::build_data_deps(int id, Instruction *insn)
{
    data_deps[id] = vector<int>();

    for (auto &operand : insn->operands())
    {
        if (auto *operand_insn = dyn_cast<Instruction>(operand))
        {
            // same basic block
            if (operand_insn->getParent()->getName() == insn->getParent()->getName())
            {
                if (insn_uid_map.find(operand_insn) != insn_uid_map.end())
                {
                    int op_id = insn_uid_map[operand_insn];
                    data_deps[id].push_back(op_id);
                }
            }
        }
    }
}

double CostModel::evaluate_ino(vector<Instruction *> insns, int N)
{
    ArchGraph *arch_graph = new ArchGraph();

    arch_graph->pipe_width = 1;
    arch_graph->resource_names = resource_names;
    arch_graph->resource_use_node = resource_use_node;
    arch_graph->port_fus = port_fus;
    arch_graph->fu_port_map = fu_port_map;

    arch_graph->read_ports = read_ports;
    arch_graph->write_ports = write_ports;
    arch_graph->rdwt_ports = rdwt_ports;

    arch_graph->insn_config = insn_config;
    arch_graph->bb_size = insns.size();

    arch_graph->not_pipeline_fus = not_pipeline_fus;

    int size = insns.size();
    for (int i = 0; i < size * N; i++)
    {
        Instruction *insn = insns[i % size];

        arch_graph->insn_uid_map[insn] = i;
        arch_graph->build_data_deps(i, insn);
        arch_graph->add_nodes(i, insn);
        arch_graph->update_commit_edges({i});
        // errs() << "i=" << i << " insn=" << insn->getOpcodeName() << "\n";
    }

    // cout << "critial path=" << arch_graph->critial_path << "\n";

    // ofstream dfg_output;
    // string dfg_file = "arch.dfg";
    // dfg_output.open(dfg_file);
    // arch_graph->visualize(dfg_output);
    // dfg_output.close();

    return arch_graph->critial_path * 1.0;
}

void ArchGraph::add_fetch_dec_node(int id, Instruction *insn)
{
    // ArchNode *node = new ArchNode(insn, "F/D");
    string name = insn->getOpcodeName();
    ArchNode *node = new ArchNode(id, insn, name + ", F/D");
    nodes.push_back(node);

    insn_fetch_node[id] = node;

    // edges between F/Ds becauses of pipeline width
    if (id - pipe_width >= 0)
    {
        ArchNode *pred_fetch_node = get_fetch_node(id - pipe_width);
        pred_fetch_node->out_edge_nodes.push_back(node);

        node->start_cycle = pred_fetch_node->start_cycle + 1;
    }
    // cout << "====== fetch= " << node->start_cycle << "\n";
}

void ArchGraph::add_dp_node(int id, Instruction *insn)
{
    string name = insn->getOpcodeName();
    ArchNode *node = new ArchNode(id, insn, name + ", DP");
    nodes.push_back(node);

    insn_dp_node[id] = node;

    // create an edge from f/d to dp
    ArchNode *pred_fetch_node = get_fetch_node(id);
    pred_fetch_node->out_edge_nodes.push_back(node);

    node->start_cycle = pred_fetch_node->start_cycle + 1;

    string rob_found = "";
    uint64_t rob_found_cycle = 0;
    find_rob_entry(rob_found, rob_found_cycle);
    if (resource_use_node.find(rob_found) != resource_use_node.end())
    {
        ArchNode *dp_node = resource_use_node[rob_found];
        ArchNode *commit_node = get_commit_node(dp_node->uid);
        commit_node->out_edge_nodes.push_back(node);
    }
    resource_use_node[rob_found] = node;
    node->start_cycle = max(node->start_cycle, rob_found_cycle);

    if (insn->getOpcodeName() == "load")
    {
        string ldq_found = "";
        uint64_t ldq_found_cycle = 0;
        find_ldq_entry(ldq_found, ldq_found_cycle);
        if (resource_use_node.find(ldq_found) != resource_use_node.end())
        {
            ArchNode *dp_node = resource_use_node[ldq_found];
            ArchNode *commit_node = get_commit_node(dp_node->uid);
            commit_node->out_edge_nodes.push_back(node);
        }
        resource_use_node[ldq_found] = node;
        node->start_cycle = max(node->start_cycle, ldq_found_cycle);
    }

    if (insn->getOpcodeName() == "store")
    {
        string stq_found = "";
        uint64_t stq_found_cycle = 0;
        find_stq_entry(stq_found, stq_found_cycle);
        if (resource_use_node.find(stq_found) != resource_use_node.end())
        {
            ArchNode *dp_node = resource_use_node[stq_found];
            ArchNode *commit_node = get_commit_node(dp_node->uid);
            commit_node->out_edge_nodes.push_back(node);
        }
        resource_use_node[stq_found] = node;
        node->start_cycle = max(node->start_cycle, stq_found_cycle);
    }

    string issueq_found = "";
    uint64_t issueq_found_cycle = 0;
    find_issueq_entry(issueq_found, issueq_found_cycle);
    if (resource_use_node.find(issueq_found) != resource_use_node.end())
    {
        ArchNode *dp_node = resource_use_node[issueq_found];
        ArchNode *issue_node = get_issue_node(dp_node->uid);
        issue_node->out_edge_nodes.push_back(node);
    }
    resource_use_node[issueq_found] = node;
    node->start_cycle = max(node->start_cycle, issueq_found_cycle);

    // cout << "====== dp= " << node->start_cycle << "\n";
}

int ArchNode::get_port_cycle(string port)
{
    for (int i = 0; i < found_ports.size(); i++)
    {
        if (port == found_ports[i])
        {
            return found_port_cycles[i];
        }
    }
    return 0;
}

void ArchGraph::add_port_node(int id, Instruction *insn)
{
    string name = insn->getOpcodeName();
    ArchNode *node = new ArchNode(id, insn, name + ", P");
    nodes.push_back(node);

    insn_port_node[id] = node;

    // create an edge frm dp to port
    ArchNode *pred_dp_node = get_dp_node(id);
    pred_dp_node->out_edge_nodes.push_back(node);

    node->start_cycle = pred_dp_node->start_cycle + 1;
    compute_duration(node, insn);
    find_min_port_fu(node->insn, node->found_ports, node->found_fus, node->found_min_cycle, node->found_port_cycles);

    // port, rr, issue are all dominate by data deps and fu
    build_data_dependency(id, node);

    // fu
    ArchNode *pred_port_node = get_port_node(id);
    for (auto fu : pred_port_node->found_fus)
    {
        if (resource_use_node.find(fu) != resource_use_node.end())
        {
            ArchNode *pred_node = resource_use_node[fu];
            pred_node->out_edge_nodes.push_back(node);
            // cout << "===fu start cycle=" << node->start_cycle << " pred->start_cycle=" << pred_node->start_cycle << "\n";
            uint64_t stage_cycle = pred_node->stage_cycle;
            if (fu != "CUSTOM" && count(not_pipeline_fus.begin(), not_pipeline_fus.end(), fu))
            {
                stage_cycle = pred_node->duration;
            }
            // cout<<"stage cycle="<<stage_cycle<<"\n";
            // uint64_t stage_cycle = 1;
            node->start_cycle = max(node->start_cycle, pred_node->start_cycle + stage_cycle);
        }
        resource_use_node[fu] = node;
    }

    // issue port
    for (int i = 0; i < node->found_ports.size(); i++)
    {
        string port = node->found_ports[i];

        if (resource_use_node.find(port) != resource_use_node.end())
        {
            ArchNode *pred_node = resource_use_node[port];
            pred_node->out_edge_nodes.push_back(node);
            uint64_t port_cycle = pred_node->get_port_cycle(port);
            node->start_cycle = max(node->start_cycle, pred_node->start_cycle + port_cycle);
        }
        resource_use_node[port] = node;
    }

// cout << "====== port= " << node->start_cycle << "\n";
}

void ArchGraph::add_rr_node(int id, Instruction *insn)
{
    string name = insn->getOpcodeName();
    ArchNode *node = new ArchNode(id, insn, name + ", RR");
    nodes.push_back(node);

    insn_rr_node[id] = node;

    // create an edge frm port to rr
    ArchNode *pred_port_node = get_port_node(id);
    pred_port_node->out_edge_nodes.push_back(node);

    node->start_cycle = pred_port_node->start_cycle + 0;

    // rd ports
    read_port_contention(node);

    // record the cycle of reading reg
    for (int dep : data_deps[node->uid])
    {
        reg_node_map[dep] = node;
    }

    // cout << "====== rr= " << node->start_cycle << "\n";
}

void ArchGraph::compute_duration(ArchNode * node,  Instruction *insn){

    node->duration = insn_config[insn->getOpcode()]->runtime_cycles;

    if (isa<CallInst>(insn))
    {
        CallInst *call = dyn_cast<CallInst>(insn);
        auto func = call->getCalledFunction();
        if (MDNode *md = func->getMetadata("encaps_func"))
        {
            int i = 0;
            for (const auto &op : md->operands())
            {
                auto *meta = llvm::dyn_cast<llvm::MDString>(op.get());
                if (i == 0)
                {
                    node->duration = stoi(meta->getString().str());
                    i++;
                }
                else if (i == 1)
                {
                    node->stage_cycle = stoi(meta->getString().str());
                    // only two ops
                    break;
                }
            }
        }
        else if (MDNode *md = func->getMetadata("packing_func"))
        {
            node->duration = 1;
        }
    }
}

void ArchGraph::add_issue_node(int id, Instruction *insn)
{
    string name = insn->getOpcodeName();
    ArchNode *node = new ArchNode(id, insn, name + ", I");
    nodes.push_back(node);

    insn_issue_node[id] = node;

    // create an edge frm rr to issue
    ArchNode *pred_rr_node = get_rr_node(id);
    pred_rr_node->out_edge_nodes.push_back(node);

    node->start_cycle = pred_rr_node->start_cycle + 0;
    compute_duration(node, insn);
    
    // cout << "====== issue= " << node->start_cycle << "\n";
}

void ArchGraph::add_cp_node(int id, Instruction *insn)
{
    string name = insn->getOpcodeName();
    ArchNode *node = new ArchNode(id, insn, name + ", CP");
    nodes.push_back(node);

    insn_cp_node[id] = node;

    // create an edge frm issue to wb
    ArchNode *pred_issue_node = get_issue_node(id);
    pred_issue_node->out_edge_nodes.push_back(node);

    node->start_cycle = pred_issue_node->start_cycle + pred_issue_node->duration;

    // write port
    write_port_contention(node);
    // cout << "====== wb= " << node->start_cycle << "\n";
}

void ArchGraph::add_commit_node(int id, Instruction *insn)
{
    string name = insn->getOpcodeName();
    ArchNode *node = new ArchNode(id, insn, name + ", R");
    nodes.push_back(node);

    insn_commit_node[id] = node;

    // create an edge frm wb to commit
    ArchNode *pred_cp_node = get_cp_node(id);
    pred_cp_node->out_edge_nodes.push_back(node);

    node->start_cycle = pred_cp_node->start_cycle + 1;
    // cout << "====== reitre= " << node->start_cycle << "\n";

    // cout << insn->getOpcodeName() << " commit cycle=" << node->start_cycle + 1 << "\n";
    // critial_path = max(node->start_cycle + 1, critial_path);
}

void ArchGraph::add_nodes(int id, Instruction *insn)
{
    insns.push_back(insn);

    // cout << "Add node: " << insn_to_string(insn) << "\n";

    add_fetch_dec_node(id, insn);
    add_dp_node(id, insn);
    add_port_node(id, insn);
    add_rr_node(id, insn);
    add_issue_node(id, insn);
    add_cp_node(id, insn);
    add_commit_node(id, insn);
}

void ArchGraph::visualize(ofstream &file)
{
    file << "digraph \"Arch Graph\" {\n";
    for (ArchNode *node : nodes)
    {
        file << "\tNode" << node << "[shape=record, label=\"" << node->name << "\"];\n";
    }

    for (ArchNode *node : nodes)
    {
        for (ArchNode *out : node->out_edge_nodes)
        {
            file << "\tNode" << node << " -> Node" << out << " [label=" << out->start_cycle - node->start_cycle << "]" << "\n";
        }
    }

    file << "}\n";
}

void ArchGraph::build_data_dependency(int id, ArchNode *node)
{
    vector<int> deps = data_deps[id];

    for (int op_id : deps)
    {
        ArchNode *pred_issue_node = get_issue_node(op_id);
        pred_issue_node->out_edge_nodes.push_back(node);

        node->start_cycle = max(pred_issue_node->start_cycle + pred_issue_node->duration, node->start_cycle);
    }
}

void ArchGraph::find_rob_entry(string &found, uint64_t &found_cycle)
{
    found = round_robin_find_min_usage_res("rob");
    found_cycle = 0;
    if (resource_use_node.find(found) != resource_use_node.end())
    {
        ArchNode *dp_node = resource_use_node[found];
        ArchNode *commit_node = get_commit_node(dp_node->uid);
        if (commit_node->retire_cycle == 0)
            found_cycle = commit_node->start_cycle + 1;
        else
            found_cycle = commit_node->retire_cycle + 1;
    }
}

void ArchGraph::find_issueq_entry(string &found, uint64_t &found_cycle)
{
    found = round_robin_find_min_usage_res("issueq");
    found_cycle = 0;
    if (resource_use_node.find(found) != resource_use_node.end())
    {
        ArchNode *dp_node = resource_use_node[found];
        ArchNode *issue_node = get_issue_node(dp_node->uid);
        found_cycle = issue_node->start_cycle + 1;
    }
}

// entry is freed for ld/st when commited
void ArchGraph::find_ldq_entry(string &found, uint64_t &found_cycle)
{
    found = round_robin_find_min_usage_res("ldq");
    found_cycle = 0;
    if (resource_use_node.find(found) != resource_use_node.end())
    {
        ArchNode *dp_node = resource_use_node[found];
        ArchNode *commit_node = get_commit_node(dp_node->uid);
        found_cycle = commit_node->start_cycle + 1;
    }
}

void ArchGraph::find_stq_entry(string &found, uint64_t &found_cycle)
{
    found = round_robin_find_min_usage_res("stq");
    found_cycle = 0;
    if (resource_use_node.find(found) != resource_use_node.end())
    {
        ArchNode *dp_node = resource_use_node[found];
        ArchNode *commit_node = get_commit_node(dp_node->uid);
        found_cycle = commit_node->start_cycle + 1;
    }
}

int get_cstm_opcode(Instruction *insn)
{
    if (isa<CallInst>(insn))
    {
        CallInst *call = dyn_cast<CallInst>(insn);
        auto func = call->getCalledFunction();
        if (MDNode *md = func->getMetadata("encaps_func"))
        {
            return 1025;
        }
        else if (MDNode *md = func->getMetadata("packing_func"))
        {
            return 1023;
        }
    }

    return insn->getOpcode();
}

void ArchGraph::find_min_port_fu(Instruction *insn,
                                 vector<string> &found_ports,
                                 vector<string> &found_fus,
                                 uint64_t &found_min_cycle,
                                 vector<int> &found_port_cycles)
{
    int opcode = get_cstm_opcode(insn);

    auto fus = insn_config[opcode]->fus;
    auto ports = insn_config[opcode]->ports;

    vector<int> port_cycles;
    for (auto iter = ports.begin(); iter != ports.end(); iter++)
    {
        port_cycles.push_back(iter->first);
    }

    for (auto alternative : fus)
    {

        vector<uint64_t> cycles;
        vector<string> temp_found_fu_names;
        vector<string> temp_found_port_names;
        vector<int> temp_found_port_cycles;

        // last time all resources are avaliable
        uint64_t max_cycle = 0;

        assert(port_cycles.size() == alternative.size());
        // TODO: fix this, fu and port are not binded now
        set<string> ban_port;

        int i = 0;
        for (auto fu : alternative)
        {

            vector<int> fu_ports = ports[i].second;
            set<string> fu_port_names;
            for (int port_id : fu_ports)
            {
                fu_port_names.insert("issue_port_" + to_string(port_id));
            }

            string min_fu_name = "";
            string min_port_name = "";
            int min_port_cycle = 0;

            vector<string> fu_names = resource_names[fu];
            uint64_t min_cycle = UINT_MAX;

            // get earliest time that fu and port are avaliable
            for (string fu_name : fu_names)
            {
                string port_name = fu_port_map[fu_name];
                if (!fu_port_names.count(port_name))
                    continue;
                if (ban_port.count(port_name))
                    continue;

                uint64_t fu_cycle = 0;
                uint64_t port_cycle = 0;

                if (resource_use_node.find(fu_name) != resource_use_node.end())
                {
                    ArchNode *fu_pred_node = resource_use_node[fu_name];
                    // fu_cycle = fu_pred_node->start_cycle + 1;

                    uint64_t stage_cycle = fu_pred_node->stage_cycle;
                    if (fu_name != "CUSTOM" && count(not_pipeline_fus.begin(), not_pipeline_fus.end(), fu_name))
                    {
                        stage_cycle = fu_pred_node->duration;
                    }
                    // uint64_t stage_cycle = 1;
                    fu_cycle = fu_pred_node->start_cycle + stage_cycle;
                }

                if (resource_use_node.find(port_name) != resource_use_node.end())
                {
                    ArchNode *port_pred_node = resource_use_node[port_name];
                    port_cycle = port_pred_node->start_cycle + port_pred_node->get_port_cycle(port_name);
                }

                uint64_t cycle = max(fu_cycle, port_cycle);
                if (cycle < min_cycle)
                {
                    min_cycle = cycle;
                    min_fu_name = fu_name;
                    min_port_name = port_name;
                    min_port_cycle = port_cycles[i];

                    // cout << "opcode=" << opcode << " min_cycl=" << min_cycle << " min_fu_name=" << min_fu_name << " min_port_name=" << min_port_name << " min_port_cycle=" << min_port_cycle << "\n";
                }
            }
            ban_port.insert(min_port_name);

            cycles.push_back(min_cycle);
            temp_found_fu_names.push_back(min_fu_name);
            temp_found_port_names.push_back(min_port_name);
            temp_found_port_cycles.push_back(min_port_cycle);

            max_cycle = max(max_cycle, min_cycle);

            i++;
        }

        if (max_cycle < found_min_cycle)
        {
            found_ports = temp_found_port_names;
            found_fus = temp_found_fu_names;
            found_port_cycles = temp_found_port_cycles;
            found_min_cycle = max_cycle; 
        }
    }
}

// round-robin to find min use count resources
string ArchGraph::round_robin_find_min_usage_res(string resource, set<string> ban)
{
    string found = "";
    vector<string> names = resource_names[resource];

    uint64_t min_cycle = UINT_MAX;
    for (string name : names)
    {
        if (ban.count(name))
            continue;

        if (resource_use_node.find(name) != resource_use_node.end())
        {
            uint64_t start_cycle = resource_use_node[name]->start_cycle;
            if (start_cycle < min_cycle)
            {
                min_cycle = start_cycle;
                found = name;
            }
        }
        else
        {
            // first touch the resource
            min_cycle = 0;
            found = name;
        }
    }

    return found;
}

int ArchGraph::get_reg_reads_count(ArchNode *node)
{
    vector<int> deps = data_deps[node->uid];
    int bypass_count = 0;

    for (int dep : deps)
    {
        ArchNode *issue_node = get_issue_node(dep);
        ArchNode *commit_node = get_commit_node(dep);
        // NOTE: not all data dependency will be forwarded
        // forward data from when execute finish to before commit
        if (issue_node->start_cycle + issue_node->duration <= node->start_cycle && node->start_cycle < commit_node->start_cycle)
        {
            bypass_count++;
        }

        // broadcast read regs in the same window
        // check if share regs data
        if (reg_node_map.find(dep) != reg_node_map.end())
        {
            ArchNode *issue_node = reg_node_map[dep];
            // check if in the same window
            if (node->uid / pipe_width == issue_node->uid / pipe_width)
            {
                // if (issue_node->start_cycle == node->start_cycle)
                // {
                bypass_count++;
                // }
            }
        }
    }

    return deps.size() - bypass_count;
}

void ArchGraph::issue_port_contention(ArchNode *node)
{
    Instruction *insn = node->insn;
    auto ports = insn_config[get_cstm_opcode(insn)]->ports;
    int ports_n = resource_names["issue_ports"].size();

    auto fus = insn_config[get_cstm_opcode(insn)]->fus;

    set<string> ban;
    for (auto iter = ports.begin(); iter != ports.end(); iter++)
    {
        set<string> temp = ban;

        for (int i = 0; i < ports_n; i++)
        {
            if (!count(iter->second.begin(), iter->second.end(), i))
            {
                temp.insert("issue_port_" + to_string(i));
            }
        }

        string found = round_robin_find_min_usage_res("issue_ports", temp);
        if (resource_use_node.find(found) != resource_use_node.end())
        {
            ArchNode *pred_node = resource_use_node[found];
            pred_node->out_edge_nodes.push_back(node);
            node->start_cycle = max(node->start_cycle, pred_node->start_cycle + port_delay_map[found]);
        }
        ban.insert(found);

        resource_use_node[found] = node;
        // cout << "port=" << found << " delay=" << iter->first << "\n";
        port_delay_map[found] = iter->first;
    }
}

void ArchGraph::read_port_contention(ArchNode *node)
{
    int live_ins = get_reg_reads_count(node);
    for (int i = 0; i < live_ins; i++)
    {
        string found = round_robin_find_min_usage_res("read_ports");
        if (resource_use_node.find(found) != resource_use_node.end())
        {
            ArchNode *pred_node = resource_use_node[found];
            pred_node->out_edge_nodes.push_back(node);

            // cout << "fount=" << found << " i=" << i << " node->start=" << node->start_cycle << " prenode=" << pred_node->start_cycle << "\n";
            // errs() << "insn=" << *(pred_node->insn) << "\n";
            node->start_cycle = max(node->start_cycle, pred_node->start_cycle + 1);
        }

        resource_use_node[found] = node;
    }
}

void ArchGraph::write_port_contention(ArchNode *node)
{
    for (int i = 0; i < node->live_outs; i++)
    {
        string found = round_robin_find_min_usage_res("write_ports");

        if (resource_use_node.find(found) != resource_use_node.end())
        {
            ArchNode *pred_node = resource_use_node[found];
            pred_node->out_edge_nodes.push_back(node);

            // errs() << "fount=" << found << " c=" << c << " i=" << i << " node->start=" << node->start_cycle << " prenode=" << pred_node->start_cycle << "\n";
            // errs() << "insn=" << *(pred_node->insn) << "\n";
            node->start_cycle = max(node->start_cycle, pred_node->start_cycle + 1);
        }

        resource_use_node[found] = node;
    }
}