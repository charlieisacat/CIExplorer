#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <assert.h>
#include <string>
#include <algorithm>

#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <sstream>

#include "../mining/maps.h"

using namespace std;

class Edge;
class Vertex;
class Graph;

class Edge
{
public:
    int64_t eid = -1;
    int64_t frm = -1;
    int64_t to = -1;

    Edge(int64_t _eid, int64_t _frm, int64_t _to) : eid(_eid), frm(_frm), to(_to) {}
    Edge(int64_t _frm, int64_t _to) : frm(_frm), to(_to) {}

    void setFrmVertex(int64_t _frm)
    {
        this->frm = _frm;
    }
    void setToVertex(int64_t _to)
    {
        this->to = _to;
    }
};

class Vertex
{
public:
    int64_t vid = -1;
    int64_t opcode = -1;
    std::string opname = "";
    int64_t lnbr = -1;
    int64_t topoid = -1;

    int64_t inDegree = 0, outDegree = 0;
    int64_t tmpInDegree = 0, tmpOutDegree = 0;

    int64_t bbInstId = -1;
    int64_t glbid = -1;

    double hwDelay = 0.0;
    int fuID = -1;

    Vertex(int64_t _vid) : vid(_vid) {}
    Vertex(int64_t _vid,
           int64_t _opcode,
           std::string _opname,
           int64_t _lnbr) : vid(_vid),
                            opcode(_opcode),
                            opname(_opname),
                            lnbr(_lnbr) {}

    std::vector<int64_t> outEdges;
    std::vector<int64_t> inEdges;

    void deleteInEdges(int vid)
    {
        auto eIter = std::find(inEdges.begin(), inEdges.end(), vid);
        if (eIter != inEdges.end())
        {
            inEdges.erase(eIter);
        }
    }

    void deleteOutEdges(int vid)
    {
        auto eIter = std::find(outEdges.begin(), outEdges.end(), vid);
        if (eIter != outEdges.end())
        {
            outEdges.erase(eIter);
        }
    }

    std::vector<std::shared_ptr<Vertex>> acrssBBInVertex;
};

class Graph
{
public:
    int64_t gid = -1;
    std::vector<std::shared_ptr<Edge>> edges;
    std::vector<std::shared_ptr<Vertex>> vertices;
    std::unordered_map<int64_t, int64_t> lnbrToVid;

    int pattRepeatTime = 1;

    // map  merged vertices' lnbr to new node
    std::unordered_map<int64_t, int64_t> lnbrMapping;

    std::string funcName = "";
    std::string moduleName = "";
    std::string bbName = "";

    // used for merging
    std::vector<std::shared_ptr<Graph>> glist;

    // only for patt
    std::string code = "";

    double sigvalue = 0.;
    double tvalue = 0.;
    long itervalue = 0;

    double score = 0.;
    double savedArea = 0.;
    double savedAreaMargin = 0.;
    int pattID = -1;

    Graph(int64_t _gid) : gid(_gid) {}

    void addVertex(std::shared_ptr<Vertex> v)
    {
        vertices.push_back(v);
        lnbrToVid.insert(std::make_pair(v->lnbr, v->vid));
    }
    void addEdge(std::shared_ptr<Edge> e)
    {
        edges.push_back(e);
    }
    void print();
    void updateDegree();
    void updateHWDelay();
    void sortVertices();
    void topsort();

    void updateLnbrToVid()
    {
        lnbrToVid.clear();
        for (auto v : this->vertices)
        {
            lnbrToVid.insert(std::make_pair(v->lnbr, v->vid));
        }
    }

    void updateGraphEdges(std::unordered_map<int64_t, int64_t> idMap)
    {
        for (int i = 0; i < this->edges.size(); i++)
        {
            auto e = this->edges[i];
            if (i != e->eid)
            {
                e->eid = i;
            }

            auto firstIter = idMap.find(e->frm);
            if (firstIter != idMap.end())
                e->frm = firstIter->second;

            auto secondIter = idMap.find(e->to);
            if (secondIter != idMap.end())
                e->to = secondIter->second;
        }
    }

    std::unordered_map<int64_t, int64_t> updateVertexID()
    {
        std::unordered_map<int64_t, int64_t> idMap;
        for (int i = 0; i < this->vertices.size(); i++)
        {
            auto v = this->vertices[i];
            if (i != v->vid)
            {
                idMap.insert(std::make_pair(v->vid, i));
                v->vid = i;
            }
            else
                idMap.insert(std::make_pair(v->vid, i));
        }
        return idMap;
    }
    void updateEdgeID()
    {
        for (int i = 0; i < this->edges.size(); i++)
        {
            auto e = this->edges[i];
            if (i != e->eid)
            {
                e->eid = i;
            }
        }
    }

    int **getAdjMatrix(bool flag = false);

    static vector<int> getTopSortID(Graph *g)
    {
        queue<int> q;
        vector<int> top;

        int vn = g->vertices.size();
        vector<int> in(vn, 0);

        for (int i = 0; i < vn; i++)
        {
            auto v = g->vertices[i];
            in[i] = v->inEdges.size();
        }

        int t = -1;
        int cnt = 0;

        for (int i = 0; i < vn; ++i) // 将所有入度为0的点加入队列
            if (in[i] == 0)
                q.push(i);

        while (q.size())
        {
            t = q.front(); // 每次取出队列的首部
            top.push_back(t); // 加入到 拓扑序列中
            cnt++;            // 序列中的元素 ++
            q.pop();

            auto v = g->vertices[t];

            for (auto eid : v->outEdges)
            {
                auto e = g->edges[eid];
                // cout << "eid=" << eid << " e->frm=" << e->frm << " e->to" << e->to << "\n";

                // 遍历 t 点的出边
                int j = e->to;
                in[j]--; // j 的入度 --
                if (in[j] == 0)
                    q.push(j); // 如果 j 入度为0，加入队列当中
            }
        }

        // for (int i : top)
        // {
        //     auto v = g->vertices[i];
        //     cout << "i= " << i << " v->opname=" << v->opname << " v->lnbr=" << v->lnbr << "\n";
        // }
        return top;
    }
};



#endif