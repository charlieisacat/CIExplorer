#include "graph.h"
#include <queue>
#include <algorithm>
#include <set>
#include <map>
#include <string>
#include <iostream>
#include <cstring>

using namespace std;

void Graph::print()
{
    std::cout << "t # " << this->gid << "\n";
    for (auto v : this->vertices)
    {
        std::cout << v->vid << " " << v->opcode << " " << v->opname << " " << this->funcName << " " << this->bbName << " " << v->lnbr << "\n";
    }

    for (auto e : this->edges)
    {
        std::cout << e->frm << " " << e->to << "\n";
    }
}

// in sort by line number
// to make vertices in topo order
void Graph::sortVertices()
{
    sort(vertices.begin(), vertices.end(),
         [](shared_ptr<Vertex> a, shared_ptr<Vertex> b)
         { return a->lnbr < b->lnbr; });

    map<int, int> vidMap;
    for (int i = 0; i < vertices.size(); i++)
    {
        vertices[i]->vid = i;
        int prevVid = lnbrToVid.find(vertices[i]->lnbr)->second;
        vidMap.insert(make_pair(prevVid, i));

        // update lnbrtovid
        lnbrToVid[vertices[i]->lnbr] = i;
    }

    // inedges and outedges only store edge id
    // so there is no need to update them

    // update edges in graph
    for (auto e : edges)
    {
        e->frm = vidMap[e->frm];
        e->to = vidMap[e->to];
    }
}

void Graph::topsort()
{
    vector<int> topo = getTopSortID(this);
    vector<int> revTopo(topo.size(), 0);

    for (int i = 0; i < topo.size(); i++)
    {
        vertices[topo[i]]->topoid = i;
        revTopo[topo[i]] = i;
    }

    sort(vertices.begin(), vertices.end(),
         [](shared_ptr<Vertex> a, shared_ptr<Vertex> b)
         { return a->topoid < b->topoid; });

    for (int i = 0; i < vertices.size(); i++)
    {
        vertices[i]->vid = i;
        assert(lnbrToVid.find(vertices[i]->lnbr) != lnbrToVid.end());
        lnbrToVid[vertices[i]->lnbr] = i;
    }

    // update edges in graph
    for (auto e : edges)
    {
        e->frm = revTopo[e->frm];
        e->to = revTopo[e->to];
    }
}

void Graph::updateHWDelay()
{
    for (auto v : this->vertices)
    {
        v->hwDelay = getHwDelay(v->opname);
    }
}

void Graph::updateDegree()
{
    for (auto v : this->vertices)
    {
        v->inDegree = 0;
        v->outDegree = 0;
        v->outEdges.clear();
        v->inEdges.clear();
    }

    for (int i = 0; i < this->edges.size(); i++)
    {
        auto e = this->edges[i];
        auto v1 = this->vertices[e->frm];
        auto v2 = this->vertices[e->to];

        v1->outEdges.push_back(i);
        v2->inEdges.push_back(i);

        v1->outDegree++;
        v2->inDegree++;
    }
}

int **Graph::getAdjMatrix(bool flag)
{
    int N = vertices.size();
    int **adjMat = (int **)malloc(sizeof(int *) * N);
    for (int i = 0; i < N; i++)
    {
        adjMat[i] = (int *)malloc(sizeof(int) * N);
        memset(adjMat[i], 0, N * sizeof(int));
    }

    for (int i = 0; i < N; i++)
    {
        auto v = vertices[i];
        for (auto eid : v->outEdges)
        {
            auto e = edges[eid];
            // cout<<"i="<<i<<" e->frm="<<e->frm<<" e->to="<<e->to<<"\n";
            assert(i == e->frm);
            adjMat[i][e->to] = 1;
            if (flag)
                adjMat[e->to][i] = 1;
        }
    }

    return adjMat;
}
