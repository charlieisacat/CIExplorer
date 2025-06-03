#include "graph.h"

#include <memory>
#include <set>
#include <stack>
#include <map>
#include <queue>
#include <filesystem>
#include <unordered_map>
#include <cstring>

namespace fs = std::filesystem;

using namespace std;

void readDepData(std::string filename, map<int, set<int>> &inMap, map<int, set<int>> &outMap)
{
    string label;
    std::fstream f;
    f.open(filename);

    string moduleName, funcName, bbName, instName1, instName2;
    int lnbrBB, lnbrGlb;

    string frmBBName, toBBName;
    int frmLnbrBB, toLnbrBB, frmLnbrGlb, toLnbrGlb;

    int dummyID = -1;
    map<string, int> dummyIDMap;

    while (!(f >> label).eof())
    {
        if (label == "d")
        {
            f >> moduleName >> funcName >> frmBBName >> toBBName >> frmLnbrBB >> toLnbrBB >> frmLnbrGlb >> toLnbrGlb;

            // input number of toLnbrFunc should ++
            auto inIter = inMap.find(toLnbrGlb);
            if (inIter == inMap.end())
            {
                inMap.insert(make_pair(toLnbrGlb, set<int>()));
            }

            inMap[toLnbrGlb].insert(frmLnbrGlb);

            // output number of frmLnbrFunc should ++
            auto outIter = outMap.find(frmLnbrGlb);
            if (outIter == outMap.end())
            {
                outMap.insert(make_pair(frmLnbrGlb, set<int>()));
            }

            outMap[frmLnbrGlb].insert(toLnbrGlb);
        }
        else if (label == "a")
        {
            // one of instName1 and instname2 should be dummy
            f >> moduleName >> funcName >> bbName >> instName1 >> instName2 >> lnbrBB >> lnbrGlb;

            // frm dummy to instName2
            // so input number of instname2 should ++
            if (instName1.find("dummy") != string::npos)
            {
                auto iter = inMap.find(lnbrGlb);
                if (iter == inMap.end())
                {
                    inMap.insert(make_pair(lnbrGlb, set<int>()));
                }

                auto dmyIter = dummyIDMap.find(instName1);
                if (dmyIter == dummyIDMap.end())
                {
                    dummyIDMap.insert(make_pair(instName1, dummyID));
                    dummyID -= 1;
                }

                inMap[lnbrGlb].insert(dummyIDMap[instName1]);
            }
            // frm instname1 to dummy
            else if (instName2.find("dummy") != string::npos)
            {
                auto iter = outMap.find(lnbrGlb);
                if (iter == outMap.end())
                {
                    outMap.insert(make_pair(lnbrGlb, set<int>()));
                }

                auto dmyIter = dummyIDMap.find(instName2);
                if (dmyIter == dummyIDMap.end())
                {
                    dummyIDMap.insert(make_pair(instName2, dummyID));
                    dummyID -= 1;
                }

                outMap[lnbrGlb].insert(dummyIDMap[instName2]);
            }
        }
    }

    f.close();
}

std::vector<std::shared_ptr<Graph>> readGraphData(std::string filename, bool useFuncLnbr = false)
{
    std::vector<std::shared_ptr<Graph>> graphs;
    std::fstream f;
    f.open(filename);

    int64_t gid = 0, vid = 0, eid = 0;
    int64_t opcode, bbLnbr, frm, to, glbid, lnbr, pattRepeatTime;
    std::string opname, funcName, label, dummy, elb, moduleName, bbName;
    double score = 0.;

    std::shared_ptr<Graph> g = nullptr;

    while (!(f >> label).eof())
    {
        if (label == "t")
        {
            f >> dummy >> gid;

            if (g)
                graphs.push_back(g);

            g = std::make_shared<Graph>(gid);
            eid = 0;
            vid = 0;
        }
        else if (label == "e")
        {
            f >> frm >> to >> elb;
            auto e = std::make_shared<Edge>(eid, frm, to);
            g->addEdge(e);
            eid++;
        }
        else if (label == "v")
        {
            f >> vid >> opcode >> opname >> moduleName >> funcName >> bbName >> bbLnbr >> glbid;

            auto v = std::make_shared<Vertex>(vid, opcode, opname, glbid);
            // in func lnbr and glbvid will be same
            v->glbid = glbid;
            v->bbInstId = bbLnbr;
            g->addVertex(v);
            g->funcName = funcName;
            g->bbName = bbName;
            g->moduleName = moduleName;

            vid++;
        }
        else if (label == "n")
        {
            f >> pattRepeatTime;
            g->pattRepeatTime = pattRepeatTime;
        }
        else if (label == "s")
        {
            f >> score;
        }
    }

    if (g)
        graphs.push_back(g);

    f.close();
    return graphs;
}


void readBBList(string filename, vector<string>& bbLists)
{
    fstream file;

    if (filename.empty())
    {
        cout << "No bbList specified - \n";
        return;
    }
    else
        file.open(filename);

    string bb_name;
    while (file >> bb_name)
    {
        cout<<"reading bbname="<<bb_name<<"\n";
        bbLists.push_back(bb_name);
    }
}

void readDynInfo(string filename, map<string, double> *profileMap, map<string, double> *tCycleMap,
                 map<string, long> *iterMap)
{
    fstream file;

    if (filename.empty())
    {
        cout << "No dynamic info specified - assuming equal weights\n";
        return;
    }
    else
        file.open(filename);

    string bb_name;
    double sigvalue = 0;
    double tvalue = 0;
    long itervalue = 0;
    while (file >> bb_name >> sigvalue >> tvalue >> itervalue)
    {
        (*profileMap)[bb_name] = sigvalue;
        (*tCycleMap)[bb_name] = tvalue;
        (*iterMap)[bb_name] = itervalue;
    }
}

std::string split_str(std::string filename, char split, int idx)
{
    std::stringstream ss(filename);
    std::string segment;
    std::vector<std::string> seglist;

    while (std::getline(ss, segment, split))
    {
        seglist.push_back(segment);
    }
    if (idx == -1)
        return seglist.back();
    return seglist[idx];
}


void bindDynInfo(map<string, double> *profileMap,
                 map<string, double> *tCycleMap,
                 map<string, long> *iterMap,
                 std::vector<std::shared_ptr<Graph>> graphs)

{
    for (auto g : graphs)
    {
        string bbName = split_str(g->bbName, '_', 0);
        g->sigvalue = (*profileMap)[bbName];
        g->tvalue = (*tCycleMap)[bbName];
        g->itervalue = (*iterMap)[bbName];
    }
}

bool searchDFS(int i, int *color, int **adjMat, int N)
{
    bool noLoop = true;

    color[i] = 1;

    for (int j = 0; j < N; j++)
    {
        if (adjMat[i][j] != 0)
        {
            if (color[j] == 1)
            {
                noLoop = false;
            }
            else if (color[j] == -1)
            {
                continue;
            }
            else
            {
                noLoop &= searchDFS(j, color, adjMat, N);
            }
        }
    }

    color[i] = -1;
    return noLoop;
}

string getCode2Score(shared_ptr<Graph> g)
{
    string code = string(g->vertices.size(), '1');

    for (auto v : g->vertices)
    {
        if (v->opname == "input" || v->opname == "output")
        {
            code[v->vid] = '0';
        }
    }
    return code;
}
std::vector<int64_t> getVids(std::string diff)
{
    std::vector<int64_t> vids;

    for (int i = 0; i < diff.size(); i++)
    {
        if (diff[i] == '0')
            continue;
        vids.push_back(i);
    }

    return vids;
}


double bfsDelay(int64_t vid, std::string code, std::shared_ptr<Graph> g1, std::shared_ptr<Graph> g2)
{
    if (code[vid] == '0')
        return 0.;

    double ret = 0.;
    auto v2 = g2->vertices[vid];
    auto iter = g1->lnbrToVid.find(v2->lnbr);
    auto v1 = g1->vertices[iter->second];

    auto delay = getHwDelay(v2->opname);
    ret = delay;

    for (auto eid : v2->outEdges)
    {
        auto e = g2->edges[eid];
        assert(e->frm < e->to);
        if (code[e->to] == '0')
            continue;
        // g2 may have loops
        // cout << "bfsDelay: g2 have a loop\n";
        if (e->frm >= e->to)
            continue;
        auto childDelay = bfsDelay(e->to, code, g1, g2);
        ret = std::max(ret, childDelay + delay);
    }

    return ret;
}

std::shared_ptr<Graph> buildPattGraph(std::string code, std::shared_ptr<Graph> bbGraph)
{
    std::vector<int64_t> vids = getVids(code);

    auto pattGraph = std::make_shared<Graph>(0);
    for (auto vid : vids)
    {
        auto v2 = bbGraph->vertices[vid];
        auto v1 = std::make_shared<Vertex>(pattGraph->vertices.size(), v2->opcode, v2->opname, v2->lnbr);
        v1->glbid = v2->glbid;
        v1->bbInstId = v2->bbInstId;
        v1->hwDelay = v2->hwDelay;
        pattGraph->addVertex(v1);
    }

    for (auto vid : vids)
    {
        auto v2 = bbGraph->vertices[vid];
        auto iter = pattGraph->lnbrToVid.find(v2->lnbr);
        auto v1 = pattGraph->vertices[iter->second];

        for (auto eid : v2->outEdges)
        {
            auto e = bbGraph->edges[eid];
            auto outV = bbGraph->vertices[e->to];
            if (code[outV->vid] != '1')
                continue;

            auto iter = pattGraph->lnbrToVid.find(outV->lnbr);
            auto outVInPG = pattGraph->vertices[iter->second];

            auto newE = std::make_shared<Edge>(pattGraph->edges.size(), v1->vid, outVInPG->vid);
            pattGraph->addEdge(newE);
        }
    }
    pattGraph->updateDegree();
    pattGraph->sortVertices();
    pattGraph->updateDegree();

    pattGraph->funcName = bbGraph->funcName;
    pattGraph->moduleName = bbGraph->moduleName;
    pattGraph->bbName = bbGraph->bbName;
    pattGraph->itervalue = bbGraph->itervalue;
    pattGraph->tvalue = bbGraph->tvalue;
    pattGraph->sigvalue = bbGraph->sigvalue;
    pattGraph->code = code;
    pattGraph->pattRepeatTime = bbGraph->pattRepeatTime;

    return pattGraph;
}

double computeCPDelay(std::string code, std::shared_ptr<Graph> bbGraph)
{

    double ret = 0;
    auto pattGraph = buildPattGraph(code, bbGraph);

    for (auto v : pattGraph->vertices)
    {
        if (v->inDegree != 0)
            continue;

        auto iter = bbGraph->lnbrToVid.find(v->lnbr);
        auto vid = iter->second;

        double delay = bfsDelay(vid, code, pattGraph, bbGraph);

        // std::cout << "delay=" << delay <<"\n";

        ret = std::max(ret, delay);
    }

    return ret;
}



double getMerit(shared_ptr<Graph> g)
{

    int64_t totalCount = 0;
    double s2 = 0.;
    for (auto cand : g->glist)
    {
        // double tmp = computeSeqDelay(getCode2Score(cand), cand) * 1.0;
        double tmp = computeCPDelay(getCode2Score(cand), cand);
        // double tmp = computeSWDelay(getCode2Score(cand), cand);
        s2 += tmp * cand->itervalue;
        // std::cout << "tmp=" << tmp << " count=" << cand->itervalue << " prod=" << tmp * cand->itervalue << " size=" << cand->vertices.size() << "\n";
        totalCount += cand->itervalue;
    }

    double s1 = computeCPDelay(getCode2Score(g), g) * totalCount;
    // std::cout << "cp=" << computeCPDelay(getCode2Score(g), g) << "\n";
    // std::cout << "s2=" << s2 << " s1=" << s1 << "\n";
    return s2 - s1;
}


bool hasDependencyEndingInGraph(int vid, std::string code, std::shared_ptr<Graph> bbGraph)
{
    std::stack<int> worklist;
    std::set<int> visited;

    worklist.push(vid);
    while (!worklist.empty())
    {
        int current = worklist.top();
        worklist.pop();

        if (visited.count(current))
            continue;
        visited.insert(current);

        // If current is within the vector, dependency ends within the vector
        if (code[current] == '1')
        {
            return true;
        }

        auto v = bbGraph->vertices[current];
        for (auto eid : v->outEdges)
        {
            auto e = bbGraph->edges[eid];
            if (!visited.count(e->to))
            {
                worklist.push(e->to);
            }
        }
    }

    return false;
}

bool dfsCheckConvex(int vid, std::string code, std::shared_ptr<Graph> bbGraph)
{
    bool ret = false;
    auto v = bbGraph->vertices[vid];
    for (auto eid : v->outEdges)
    {
        auto e = bbGraph->edges[eid];
        if (code[e->to] != '1')
        {
            if (hasDependencyEndingInGraph(e->to, code, bbGraph))
            {
                return true;
            }
        }
        else
        {
            ret |= dfsCheckConvex(e->to, code, bbGraph);
        }
    }

    return ret;
}


bool checkIO(int ion0, int ion1, int Nin, int Nout, int noConst)
{
    if(noConst)
        return true;
    return ion0 <= Nin && ion1 <= Nout;
}


// return false if non-convex
bool checkConvexV2(std::string code, std::shared_ptr<Graph> bbGraph)
{
    bool ret = false;

    auto pattGraph = buildPattGraph(code, bbGraph);
    for (auto v : pattGraph->vertices)
    {
        uint64_t vid = bbGraph->lnbrToVid.find(v->lnbr)->second;
        ret |= dfsCheckConvex(vid, code, bbGraph);
    }

    return !ret;
}

shared_ptr<Graph> copyGraph(shared_ptr<Graph> g)
{
    auto newG = std::make_shared<Graph>(0);

    for (auto v : g->vertices)
    {
        auto newV = std::make_shared<Vertex>(newG->vertices.size(), v->opcode, v->opname, v->lnbr);
        newV->glbid = v->glbid;
        newV->bbInstId = v->bbInstId;
        newV->hwDelay = v->hwDelay;
        newG->addVertex(newV);
    }

    for (auto e : g->edges)
    {
        auto newE = std::make_shared<Edge>(newG->edges.size(), e->frm, e->to);
        newG->addEdge(newE);
    }

    newG->updateDegree();
    // newG->topsort();
    newG->funcName = g->funcName;
    newG->moduleName = g->moduleName;
    newG->bbName = g->bbName;
    newG->itervalue = g->itervalue;
    newG->tvalue = g->tvalue;
    newG->sigvalue = g->sigvalue;
    newG->code = g->code;
    newG->pattRepeatTime = g->pattRepeatTime;
    newG->glist = g->glist;
    newG->savedArea = g->savedArea;
    newG->savedAreaMargin = g->savedAreaMargin;
    return newG;
}

bool checkInstMergable(int i, int j, shared_ptr<Graph> ori, shared_ptr<Graph> merged)
{
    // TODO
    // check not only opcode
    // cout << "check "
    //      << "opcode=" << ori->vertices[i]->opcode << " opname=" << ori->vertices[i]->opname << "\n";
    // cout << "opcode=" << merged->vertices[j]->opcode << " opname=" << merged->vertices[j]->opname << "\n";
    return ori->vertices[i]->opname == merged->vertices[j]->opname;
}

void dump_area_ops(string filename, std::map<uint64_t, uint64_t> ops_count_map)
{

    ofstream f;
    f.open(filename);

    f << "t ";
    for (auto iter = ops_count_map.begin(); iter != ops_count_map.end(); iter++)
    {
        uint64_t op = iter->first;
        uint64_t count = iter->second;
        for (int i = 0; i < count; i++)
        {
            f << op << " ";
        }
    }
    f << "\n";

    f.close();
}

void dump_fu_ops(string filename, shared_ptr<Graph> g)
{

    ofstream f;
    f.open(filename);

    f << "t ";
    for (auto v : g->vertices)
    {
        f << v->opcode << " ";
    }
    f << "\n";

    f.close();
}
bool checkNoLoop(shared_ptr<Graph> g)
{
    int N = g->vertices.size();
    int **adjMat = g->getAdjMatrix();

    // dfs
    int *color = (int *)malloc(sizeof(int) * N);
    memset(color, 0, N * sizeof(int));

    bool noLoop = true;
    for (int t = 0; t < N; t++)
        noLoop &= searchDFS(t, color, adjMat, N);

    // free
    for (int t = 0; t < N; t++)
    {
        free(adjMat[t]);
    }
    free(adjMat);
    free(color);
    return noLoop;
}


bool checkNoLoop(int i, int j, shared_ptr<Graph> ori, shared_ptr<Graph> merged, map<int, int> vidInNewGraph)
{
    int N = merged->vertices.size();

    // get adj max
    int **adjMat = merged->getAdjMatrix();

    // add new edges
    auto oriV = ori->vertices[i];
    for (int eid : oriV->inEdges)
    {
        auto e = ori->edges[eid];
        if (vidInNewGraph.find(e->frm) == vidInNewGraph.end())
        {
            // cout<<"====== access unkown key\n";
            continue;
        }
        int frmVid = vidInNewGraph[e->frm];
        adjMat[frmVid][j] = 1;
    }

    // dfs
    int *color = (int *)malloc(sizeof(int) * N);
    memset(color, 0, N * sizeof(int));

    bool noLoop = true;
    for (int t = 0; t < N; t++)
        noLoop &= searchDFS(t, color, adjMat, N);

    // free
    for (int t = 0; t < N; t++)
    {
        free(adjMat[t]);
    }
    free(adjMat);
    free(color);

    return noLoop;
}

void addNewDep(map<int, int> vidInNewGraph, shared_ptr<Graph> ori, shared_ptr<Graph> merged, int i, int j)
{
    // merge ops
    auto oriV = ori->vertices[i];
    for (int eid : oriV->inEdges)
    {
        auto e = ori->edges[eid];
        // in topo order, e->frm should be already in merged
        assert(vidInNewGraph.find(e->frm) != vidInNewGraph.end());
        int frmVid = vidInNewGraph[e->frm];

        bool exist = false;
        for (auto e : merged->edges)
        {
            if (e->frm == frmVid && e->to == j)
            {
                exist = true;
                break;
            }
        }

        if (exist)
            continue;

        auto newE = std::make_shared<Edge>(merged->edges.size(), frmVid, j);
        // cout << "new edge frm=" << frmVid << " to=" << j << "\n";
        merged->addEdge(newE);
    }

    merged->updateDegree();
}


bool checkShareNodes(shared_ptr<Graph> g1, shared_ptr<Graph> g2)
{
    for (auto v1 : g1->vertices)
    {
        if (v1->opname == "input" ||
            v1->opname == "output" ||
            v1->opname == "call" ||
            v1->opname == "br" ||
            // v1->opname == "load" ||
            v1->opname == "store" ||
            v1->opname == "ret")
            continue;

        for (auto v2 : g2->vertices)
        {
            if (v1->opname == v2->opname)
            {
                // cout << "v1->opname=" << v1->opname << " v2->opname=" << v2->opname << "\n";
                return true;
            }
        }
    }
    return false;
}

shared_ptr<Graph> mergeStep(shared_ptr<Graph> ori, shared_ptr<Graph> merged)
{
    auto newG = copyGraph(merged);
    set<int> visited;
    bool instMerge = false;
    map<int, int> vidInNewGraph;
    for (int i = 0; i < ori->vertices.size(); i++)
    {
        instMerge = false;
        for (int j = 0; j < newG->vertices.size(); j++)
        {
            auto v2 = newG->vertices[j];
            bool noVis = visited.count(j) == 0;
            bool mergable = checkInstMergable(i, j, ori, newG);

            if (!noVis || !mergable)
                continue;

            bool noLoop = checkNoLoop(i, j, ori, newG, vidInNewGraph);
            // cout << "j=" << j << " opname=" << v2->opname << " novis=" << noVis << " merag=" << mergable << " noloop=" << noLoop << "\n";

            if (noLoop)
            {
                visited.insert(j);
                instMerge = true;

                // j is the corresponding vertex
                vidInNewGraph.insert(make_pair(i, j));
                addNewDep(vidInNewGraph, ori, newG, i, j);
                break;
            }
        }

        if (!instMerge)
        {
            // add a new vertex into merged
            auto oriV = ori->vertices[i];
            int newVid = newG->vertices.size();
            auto newV = std::make_shared<Vertex>(newVid, oriV->opcode, oriV->opname, oriV->lnbr);
            newV->glbid = oriV->glbid;
            newV->bbInstId = oriV->bbInstId;
            newV->hwDelay = oriV->hwDelay;

            newG->addVertex(newV);
            vidInNewGraph.insert(make_pair(i, newVid));
            ;
            // TODO
            // add new vertex will also create cycle
            // and we check it out after merge step
            addNewDep(vidInNewGraph, ori, newG, i, newVid);
            visited.insert(newVid);
        }
    }

    return newG;
}

void makeUniqLnbrsForInOut(vector<shared_ptr<Graph>> &graphs, int inputLnbr, int outputLnbr)
{
    for (auto g : graphs)
    {
        for (auto v : g->vertices)
        {
            if (v->opname == "input")
            {
                v->lnbr = inputLnbr;
                v->glbid = inputLnbr;
                inputLnbr--;
            }
            else if (v->opname == "output")
            {
                v->lnbr = outputLnbr;
                v->glbid = outputLnbr;
                outputLnbr++;
            }
        }
    }
}

bool checkSingleNodeGraph(shared_ptr<Graph> g)
{
    int n = 0;
    for (auto v : g->vertices)
    {
        if (v->opname != "input" && v->opname != "output")
            n++;
    }

    return n <= 1;
}

string filterOps(string code, shared_ptr<Graph> g2)
{
    string ret = code;

    for (int i = 0; i < code.size(); i++)
    {
        if (code[i] == '1')
        {
            auto v = g2->vertices[i];
            if (
                v->opname == "load" ||
                v->opname == "store" ||
                v->opname == "input" ||
                v->opname == "output" ||
                v->opname == "phi" ||
                v->opname == "br" ||
                v->opname == "alloca" ||
                v->opname == "call" ||
                v->opname == "invoke" ||
                v->opname == "landingpad" ||
                v->opname == "ret")
                ret[i] = '0';
        }
    }
    return ret;
}

std::vector<int> getIONumberV3(shared_ptr<Graph> g2)
{
    int Nin = 0;
    int Nout = 0;

    set<int> outNodeSet;

    for (auto v : g2->vertices)
    {
        if (v->opname == "input")
            Nin++;
        else if (v->opname == "output")
        {
            for (auto eid : v->inEdges)
            {
                auto e = g2->edges[eid];
                auto frmV = g2->vertices[e->frm];
                outNodeSet.insert(frmV->lnbr);
            }
        }
    }

    Nout = outNodeSet.size();

    return {Nin, Nout};
}

void dumpDFGData(string filename, vector<shared_ptr<Graph>> graphs)
{

    ofstream f;
    f.open(filename);
    for (int i = 0; i < graphs.size(); i++)
    {
        auto g = graphs[i];
        f << "t # " << i << "\n"; // start with 0

        for (auto v : g->vertices)
        {

            f << "v " << v->vid
              << " " << v->opcode << " " << v->opname
              << " " << g->moduleName << " " << g->funcName
              << " " << g->bbName << " " << v->bbInstId << " " << v->glbid << "\n";
        }

        for (auto e : g->edges)
        {
            auto frm = e->frm; // vertex
            auto to = e->to;

            f << "e " << frm << " " << to << " " << 2 << "\n";
        }

        if (g->pattRepeatTime > 1)
        {
            f << "n " << g->pattRepeatTime << "\n";
        }
    }
    f.close();
}

double computeArea(std::shared_ptr<Graph> g)
{
    double area = 0;
    std::vector<int64_t> vids = getVids(getCode2Score(g));
    for (auto vid : vids)
    {
        // cout<<"opanme="<<g->vertices[vid]->opname<<"\n";
        area += getArea(g->vertices[vid]->opname);
    }
    return area;
}

double getSavedArea(shared_ptr<Graph> g)
{
    double a2 = 0.;
    for (auto cand : g->glist)
    {
        a2 += computeArea(cand);
    }

    double a1 = computeArea(g);
    return a2 - a1;
}

void get_level_ops(std::shared_ptr<Graph> g, std::vector<uint64_t> &this_level_ops,
                                   std::vector<std::vector<uint64_t>> &ops_per_level, std::vector<int> &visited)
{
    if (this_level_ops.size() == 0)
        return;

    std::vector<uint64_t> next_level_ops;
    for (auto opid : this_level_ops)
    {
        visited[opid] = 1;
    }

    for (auto opid : this_level_ops)
    {
        auto v = g->vertices[opid];
        for (auto eid : v->outEdges)
        {
            auto e = g->edges[eid];

            auto next_v = g->vertices[e->to];
            // only if all prev nodes are visited
            bool skip = false;
            for (auto next_eid : next_v->inEdges)
            {
                auto next_e = g->edges[next_eid];
                if (visited[next_e->frm] == 0)
                {
                    skip = true;
                    break;
                }
            }

            if (!skip)
            {
                next_level_ops.push_back(e->to);
            }
        }
    }

    ops_per_level.push_back(next_level_ops);
    get_level_ops(g, next_level_ops, ops_per_level, visited);
}

std::vector<std::vector<uint64_t>> ops_per_level(std::shared_ptr<Graph> g)
{
    std::vector<std::vector<uint64_t>> ops_id;
    std::vector<uint64_t> first_level_ops;
    for (auto v : g->vertices)
    {
        if (v->inEdges.size() == 0)
        {
            first_level_ops.push_back(v->vid);
        }
    }

    std::vector<int> visited(g->vertices.size(), 0);
    ops_id.push_back(first_level_ops);
    get_level_ops(g, first_level_ops, ops_id, visited);

    /* vertex id to opcode */
    std::vector<std::vector<uint64_t>> ops;
    for (auto ops_id_per_level : ops_id)
    {
        std::vector<uint64_t> ops_per_level;
        for (auto ops_id : ops_id_per_level)
        {
            ops_per_level.push_back(g->vertices[ops_id]->opcode);
        }
        ops.push_back(ops_per_level);
    }

    return ops;
}

using Pattern = vector<std::shared_ptr<Graph>>;

int main(int argc, char **argv)
{
    std::string graphDir = std::string(argv[1]);
    std::string dynamicInfoFile = std::string(argv[2]);

    map<string, double> profileMap;
    map<string, double> totalCycleMap;
    map<string, long> iterMap;
    readDynInfo(dynamicInfoFile, &profileMap, &totalCycleMap, &iterMap);
    vector<Pattern*> patterns;

    for (const auto &entry : fs::directory_iterator(graphDir))
    {
        string patt_path = entry.path();
        std::cout<<"patt_path="<<patt_path<<"\n";
        Pattern pattern = readGraphData(patt_path);
        patterns.push_back(new Pattern(pattern.begin(), pattern.end()));
        bindDynInfo(&profileMap, &totalCycleMap, &iterMap, *(patterns.back()));
    }

    std::vector<std::shared_ptr<Graph>> graphs;
    for (Pattern *pattern : patterns)
    {
        auto graph = (*pattern)[0];
        for (auto g : *pattern)
        {
            g->updateDegree();
            g->topsort();
            g->updateDegree();

            g->score = getMerit(g);
            graph->glist.push_back(g);
        }
        graphs.push_back(graph);
    }

    std::vector<std::shared_ptr<Graph>> mergedList;

    // sort graphs by score
    sort(graphs.begin(), graphs.end(),
         [](shared_ptr<Graph> a, shared_ptr<Graph> b)
         { return a->score > b->score; });


    int gsz = graphs.size();
    while (gsz)
    {
        shared_ptr<Graph> candidate = graphs[0];

        bool improved = false;
        double bestAreaBenifit = 0.;
        shared_ptr<Graph> bestMerge = nullptr;
        int eraseId = -1;
        double max_merit = getMerit(candidate);
        double max_saved_area = getSavedArea(candidate);
        for (int i = 1; i < gsz; i++)
        {
            auto merged = mergeStep(candidate, graphs[i]);

            merged->updateDegree();
            assert(checkNoLoop(merged));
            merged->topsort();
            merged->updateDegree();

            merged->glist.insert(merged->glist.end(), candidate->glist.begin(), candidate->glist.end());

            // for (auto e : merged->edges)
            // {
            //     cout << "merged e->frm=" << e->frm << " e->to=" << e->to << "\n";
            // }

            assert(checkNoLoop(merged));

            string code2score = getCode2Score(merged);

            // in merge, getIoNumber will only count number of input/output nodes
            // auto ion = getIONumberV3(merged);
            bool convex = checkConvexV2(code2score, merged);

            // bool io_flag = checkIO(ion[0], ion[1], Nin, Nout, noConst);

            // cout << "convex " << convex << " ion[0]=" << ion[0] << " ion[1]=" << ion[1] << "\n";

            // if (!(convex && ion[0] <= Nin && ion[1] <= Nout))
            if (!convex)
            {
                continue;
            }
            double saved_area = getSavedArea(merged);
            double merit = getMerit(merged);

            std::cout << "merit=" << merit << " max_meric=" << max_merit << "\n";
            std::cout << "saved_area=" << saved_area << " max_saved_area=" << max_saved_area << "\n";
            // if (saved_area > max_saved_area)
            if (merit > max_merit)
            {
                max_merit = merit;
                max_saved_area = saved_area;

                improved = true;
                bestMerge = merged;
                eraseId = i;
                merged->savedArea = saved_area;
                merged->score = merit;
            }
            // else if (saved_area == max_saved_area)
            else if (merit == max_merit)
            {
                // if (merit >= max_merit)
                if (saved_area >= max_saved_area)
                {
                    max_merit = merit;
                    max_saved_area = saved_area;

                    improved = true;
                    bestMerge = merged;
                    eraseId = i;
                    merged->savedArea = saved_area;
                    merged->score = merit;
                }
            }
        }

        // no merge
        if (!improved)
        {
            mergedList.push_back(candidate);
        }
        else
        {
            graphs.erase(graphs.begin() + eraseId);
            graphs.push_back(bestMerge);
        }

        graphs.erase(graphs.begin());
        gsz = graphs.size();

        // sort graphs by score
        sort(graphs.begin(), graphs.end(),
             [](shared_ptr<Graph> a, shared_ptr<Graph> b)
             { return a->score > b->score; });
    }

    cout << "merged listsize=" << mergedList.size() << "\n";
    cout << "fusize: " << mergedList.size() << "\n";
    int dumpID = 0;
    for (auto g : mergedList)
    {
        g->pattRepeatTime = 1;
        // for (auto g1 : g->glist)
        // {
        //     cout << "funcName=" << g1->funcName << " bbName=" << g1->bbName << "\n";
        // }
        // cout << "===\n";
        vector<shared_ptr<Graph>> clean_g_list;
        for (auto tmpg : g->glist)
        {
            auto code = filterOps(string(tmpg->vertices.size(), '1'), tmpg);
            auto clean_g = buildPattGraph(code, tmpg);
            clean_g_list.push_back(clean_g);
        }

        dumpDFGData("to_debug_pass/" + std::to_string(dumpID) + ".data", clean_g_list);
        auto ion = getIONumberV3(g);
        cout << "inputperfu: " << ion[0] << "\n";
        cout << "outputperfu: " << ion[1] << "\n";

        auto code = filterOps(string(g->vertices.size(), '1'), g);
        auto tmp_g = buildPattGraph(code, g);

        auto ops = ops_per_level(tmp_g);

        // max count of same op across levels
        std::map<uint64_t, uint64_t> ops_max_count_map;
        for (auto ops_per_level : ops)
        {
            std::map<uint64_t, uint64_t> lvl_count_map;
            for (auto op : ops_per_level)
            {
                if (lvl_count_map.find(op) == lvl_count_map.end())
                {
                    lvl_count_map[op] = 0;
                }
                lvl_count_map[op]++;
            }

            for (auto iter = lvl_count_map.begin(); iter != lvl_count_map.end(); iter++)
            {
                uint64_t op = iter->first;
                uint64_t count = iter->second;

                ops_max_count_map[op] = std::max(ops_max_count_map[op], count);
            }
        }

        for (auto iter = ops_max_count_map.begin(); iter != ops_max_count_map.end(); iter++)
        {
            uint64_t op = iter->first;
            uint64_t count = iter->second;
            cout << "op=" << op << " count=" << count << "\n";
        }

        cout << "============\n";

        dump_area_ops("area_ops_data/" + std::to_string(dumpID) + ".data", ops_max_count_map);
        dump_fu_ops("fu_ops_data/" + std::to_string(dumpID) + ".data", tmp_g);
        dumpID++;
    }
    dumpDFGData("merge_output/new_insts.data", mergedList);
}