#include "utils.h"

std::vector<Pattern *> read_pattern_data(std::string filename)
{
    std::vector<Pattern *> patterns;
    std::fstream f;
    f.open(filename);

    if (!f.is_open()){
        f.close();
        return patterns;
    }

    Graph *graph = nullptr;
    Pattern *pattern = nullptr;
    double score;

    int frm_uid, to_uid;
    string label, _, bbname;

    while (!(f >> label).eof())
    {
        if (label == "t")
        {
            f >> _ >> _;

            if (pattern)
            {
                if (graph)
                {
                    pattern->push_back(graph);
                    graph = nullptr;
                }
                if (pattern->size() > 0)
                    patterns.push_back(pattern);
            }
            pattern = new Pattern();
        }
        else if (label == "p")
        {
            f >> _ >> _;
            if (graph)
                pattern->push_back(graph);
            graph = new Graph();
        }
        else if (label == "a")
        {
            f >> _ >> _ >> _ >> _ >> bbname >> _ >> _ >> frm_uid >> to_uid;
            if (!count(graph->vertices.begin(), graph->vertices.end(), frm_uid))
            {
                graph->add_vertex(frm_uid);
            }

            if (!count(graph->vertices.begin(), graph->vertices.end(), to_uid))
            {
                graph->add_vertex(to_uid);
            }
            graph->bbname = bbname;
        }
        else if (label == "s")
        {
            f >> score;
        }
    }

    if (pattern)
    {
        if (graph)
        {
            pattern->push_back(graph);
        }
        if (pattern->size() > 0)
            patterns.push_back(pattern);
    }

    f.close();

    return patterns;
}

void read_bblist(string filename, vector<string> &bblist)
{
    fstream file;
    file.open(filename);

    string bbname;
    while (file >> bbname)
    {
        bblist.push_back(bbname);
    }
}

std::vector<Graph *> read_graph_data(std::string filename)
{
    std::vector<Graph *> graphs;
    std::fstream f;
    f.open(filename);

    int64_t uid;
    string _, bbname, label;
    double score;

    Graph *g = nullptr;

    while (!(f >> label).eof())
    {
        if (label == "t")
        {
            f >> _ >> _;

            if (g)
                graphs.push_back(g);

            g = new Graph();
        }
        else if (label == "e")
        {
            f >> _ >> _ >> _;
        }
        else if (label == "v")
        {
            f >> _ >> _ >> _ >> _ >> _ >> bbname >> _ >> uid;
            g->add_vertex(uid);
            g->bbname = bbname;
        }
        else if (label == "n")
        {
            f >> _;
        }
        else if (label == "s")
        {
            f >> score;
            g->score = score;
        }
    }

    if (g)
        graphs.push_back(g);

    f.close();
    return graphs;
}

std::vector<Graph *> read_graph_data_v2(std::string filename)
{
    std::vector<Graph *> graphs;
    std::fstream f;
    f.open(filename);

    int64_t uid;
    string _, bbname, label;

    Graph *g = nullptr;

    while (!(f >> label).eof())
    {
        if (label == "t")
        {
            f >> _ >> bbname;
            g->bbname = bbname;

            if (g)
                graphs.push_back(g);

            g = new Graph();
        }
        else if (label == "v")
        {
            f >> uid;
            g->add_vertex(uid);
        }
    }

    if (g)
        graphs.push_back(g);

    f.close();
    return graphs;
}

void remove_illegal_patts(vector<Pattern *> &patts)
{
    vector<int> patt_id_to_remove;
    if (!patts.size())
        return;

    for (int j = 0; j < patts.size(); j++)
    {
        vector<set<int>> unique_set;
        vector<int> id_to_remove;

        auto patt = patts[j];

        for (int i = 0; i < patt->size(); i++)
        {
            bool add = true;
            auto g = (*patt)[i];
            set<int> qset;
            for (auto uid : g->vertices)
            {
                qset.insert(uid);
            }

            for (int j = 0; j < unique_set.size(); j++)
            {
                auto prev_qset = unique_set[j];
                bool flag = false;

                for (auto x : qset)
                {
                    if (prev_qset.count(x))
                    {
                        id_to_remove.push_back(i);
                        flag = true;
                        add = false;
                        break;
                    }
                }

                if (flag)
                    break;
            }

            if (add)
                unique_set.push_back(qset);
        }

        for (int i = 0; i < id_to_remove.size(); i++)
        {
            patt->erase(patt->begin() + id_to_remove[i] - i);
        }

        if (patt->size() < 2)
        {
            patt_id_to_remove.push_back(j);
        }
    }
    for (int i = 0; i < patt_id_to_remove.size(); i++)
    {
        patts.erase(patts.begin() + patt_id_to_remove[i] - i);
    }
}

string split_str(string str, char split, int idx)
{
    stringstream ss(str);
    string segment;
    vector<string> seglist;

    while (std::getline(ss, segment, split))
    {
        seglist.push_back(segment);
    }
    if (idx == -1)
        return seglist.back();
    return seglist[idx];
}

void read_dyn_info(string filename,
                   map<string, long> *iter_map)
{
    fstream file;
    file.open(filename);

    string bb_name;
    double sigvalue = 0;
    double tvalue = 0;
    long itervalue = 0;
    while (file >> bb_name >> sigvalue >> tvalue >> itervalue)
    {
        (*iter_map)[bb_name] = itervalue;
    }
}

void bind_dyn_info(
    map<string, long> *iter_map,
    std::vector<Graph *> graphs)

{
    for (auto g : graphs)
    {
        string bbname = split_str(g->bbname, '_', 0);
        g->itervalue = (*iter_map)[bbname];
    }
}

string bitwise_or(string s1, string s2)
{
    assert(s1.size() == s2.size());
    string result = "";
    for (int i = 0; i < s1.size(); ++i)
    {
        if (s1[i] == '1' || s2[i] == '1')
        {
            result += '1';
        }
        else
        {
            result += '0';
        }
    }

    return result;
}

string bitwise_reverse(string s)
{
    string ret = "";

    for(char c : s){
        if (c == '1')
            ret += '0';
        else
            ret += '1';
    }

    return ret;
}

std::string insn_to_string(Value *v)
{
    std::string str;
    llvm::raw_string_ostream rso(str);
    rso << *v;     // This is where the instruction is serialized to the string.
    return rso.str(); // Return the resulting string.
}
