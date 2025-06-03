#include "graph.h"
#include <queue>
#include <algorithm>
#include <set>
#include <string>

void Edge::checkEdgeIsValid()
{
    if (this->frm && this->to)
        this->isValidEdge = 1;
    else
        this->isValidEdge = 0;
}

void Vertex::updateVertexDegree()
{
    for (auto e : this->edges)
    {
        if (!e->isValidEdge)
            continue;

        if (e->frm->vid == this->vid)
            this->outDegree++;
        if (e->to->vid == this->vid)
            this->inDegree++;
    }
}

void Graph::updateVertexDegree()
{
    for (auto v : this->vertices)
    {
        v->updateVertexDegree();
    }
}

void Vertex::addEdge(std::shared_ptr<Edge> e)
{
    assert(e);
    assert(e->frm || e->to);

    edges.push_back(e);
}

std::shared_ptr<Vertex> Graph::findVertexByInst(llvm::Instruction *_inst)
{
    for (auto v : vertices)
    {
        if (v->inst == _inst)
        {
            return v;
        }
    }
    return NULL;
}

void Graph::findRoots()
{
    for (auto v : this->vertices)
    {
        if (v->inDegree == 0)
        {
            this->roots.push_back(v);
        }
    }
}

void Graph::initialize(int64_t _gid, edge_list _edges, node_list _nodes, int startOffset = -1)
{
    this->gid = _gid;
    int nodeId = 0;
    int glbID = startOffset == -1 ? 0 : startOffset;
    int emptyValueNum = 0;
    for (node_list::iterator node = _nodes.begin(), node_end = _nodes.end(); node != node_end; ++node)
    {
        auto inst = llvm::dyn_cast<llvm::Instruction>(node->first);
        if (inst)
        {
            auto v = std::make_shared<Vertex>(nodeId++, inst);
            this->addVertex(v);
            v->glbID = glbID++;
        }
        else
        {
            // TODO
            // deal with exception
        }
    }

    int edgeId = 0;
    for (edge_list::iterator edge = _edges.begin(), edge_end = _edges.end(); edge != edge_end; ++edge)
    {
        auto firstInst = llvm::dyn_cast<llvm::Instruction>(edge->first.first);
        auto secondInst = llvm::dyn_cast<llvm::Instruction>(edge->second.first);

        auto firstIter = this->findVertexByInst(firstInst);
        auto secondIter = this->findVertexByInst(secondInst);

        auto e = std::make_shared<Edge>(edgeId++);

        if (firstIter != NULL && secondIter != NULL)
        {
            std::string bb_n1 = firstIter->inst->getParent()->getName().str();
            std::string bb_n2 = secondIter->inst->getParent()->getName().str();
            if ((firstIter->vid >= secondIter->vid) && (bb_n1 == bb_n2))
            {
                continue;
            }
        }

        // call seperateBr function and data dependency across different bb
        // will cause firstIter being NULL
        if (firstIter != NULL)
        {
            e->setFrmVertex(firstIter);
            firstIter->addEdge(e);
        }
        if (secondIter != NULL)
        {
            e->setToVertex(secondIter);
            secondIter->addEdge(e);
        }

        e->checkEdgeIsValid();
        this->addEdge(e);
    }
}

void Graph::computeVidInGraph(bool removeMemOp, bool removeGepOp, bool removePHIOp)
{
    int idx = 0;
    for (int i = 0; i < vertices.size(); i++)
    {
        auto v = vertices[i];
        if (removePHIOp && v->isPHIOp)
            continue;
        if (removeMemOp && v->isMemOp)
            continue;
        if (removeGepOp && v->isGepOp)
            continue;
        v->vidInGraph = idx++;
        this->vidInGraphToVid.insert(std::make_pair(v->vidInGraph, v->vid));
    }
}

void Graph::computeWeights()
{

    std::queue<std::shared_ptr<Vertex>> q;

    for (auto r : this->roots)
    {
        q.push(r);
    }

    while (!q.empty())
    {
        auto v = q.front();
        q.pop();
        for (auto e : v->edges)
        {
            if (!e->isValidEdge)
            {
                continue;
            }

            // e->to = v, update v->weight
            if (e->to->vid == v->vid)
            {
                v->weight = std::max(v->weight, e->frm->weight + (int)(10000 * (v->hwDelay)));
                continue;
            }
            q.push(e->to);
        }
    }
}

void Graph::dumpDFGData(llvm::raw_fd_ostream &file, std::string moduleName, std::string funcName, int graphID)
{
    file << "t # " << graphID << "\n"; // start with 0

    for (auto v : this->vertices)
    {
        if (v->vidInGraph == -1)
            continue;

        int opcode = 0;
        llvm::StringRef instName = "";
        opcode = v->inst->getOpcode();
        instName = llvm::Instruction::getOpcodeName(v->inst->getOpcode());
        auto bb = v->inst->getParent();

        file << "v " << v->vidInGraph
             << " " << opcode << " " << instName
             << " " << moduleName << " " << funcName
             << " " << bb->getName().str() << " " << v->vid << " " << v->glbID << "\n";
    }

    for (auto e : this->edges)
    {
        auto frm = e->frm; // vertex
        auto to = e->to;

        // depend on inst from other BBs
        llvm::StringRef frmInstName = "UnknownValue", toInstName = "UnknownValue";
        int64_t frmId = -1, toId = -1;
        int64_t weight = 2;

        if (frm)
        {
            if (frm->vidInGraph == -1)
                continue;

            frmInstName = llvm::Instruction::getOpcodeName(frm->inst->getOpcode());
            frmId = frm->vidInGraph;
        }

        if (to)
        {
            if (to->vidInGraph == -1)
                continue;

            toInstName = llvm::Instruction::getOpcodeName(to->inst->getOpcode());
            toId = to->vidInGraph;
        }
        if (frmId == -1 || toId == -1)
            continue;
        file << "e " << frmId << " " << toId << " " << weight << "\n";
    }

    // llvm::errs() << "GRAPH: " << graphID << " DFG Data Write Done\n";
}

Graph::Graph(int64_t _gid, edge_list _edges, node_list _nodes)
{
    this->initialize(_gid, _edges, _nodes, -1);
}

Graph::Graph(int64_t _gid, edge_list _edges, node_list _nodes, int _startOffset)
{
    this->initialize(_gid, _edges, _nodes, _startOffset);
}

std::shared_ptr<Edge> Graph::getEdge(int64_t frmVid, int64_t toVid)
{
    auto frm = this->vertices[frmVid];
    for (auto e : frm->edges)
    {
        if (e->frm->vid == frm->vid && e->to->vid == toVid)
        {
            return e;
        }
    }
    return NULL;
}

void Vertex::deleteEdge(std::shared_ptr<Edge> e)
{
    auto eIter = find(this->edges.begin(), this->edges.end(), e);
    if (eIter != this->edges.end())
    {
        this->edges.erase(eIter);
    }
}

std::unordered_map<int64_t, int64_t> Graph::updateVertexID()
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

void Graph::updateEdgeID()
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
