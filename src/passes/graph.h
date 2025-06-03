#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>
#include <memory>
#include <vector>
#include <unordered_map>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Use.h>
#include <llvm/Analysis/CFG.h>
#include <list>

#include <sstream>

typedef std::pair<llvm::Value *, llvm::StringRef> node;
// 指令与指令之间的边
typedef std::pair<node, node> edge;
// 指令的集合
typedef std::list<node> node_list;
// 边的集合
typedef std::list<edge> edge_list;
// 如果是变量则获得变量的名字，如果是指令则获得指令的内容

void dumpDFG(std::string outdir, node_list &nodes, edge_list &edges, edge_list &inst_edges, std::string funcName, int bbId, int &num)
{
    std::error_code error;
    enum llvm::sys::fs::OpenFlags F_None;
    std::string fileNameStr = outdir + "/" + funcName + "_" + std::to_string(bbId) + ".dot";
    llvm::StringRef fileName(fileNameStr);
    llvm::raw_fd_ostream file(fileName, error, F_None);

    file << "digraph \"DFG for'" + funcName + "_" + std::to_string(bbId) + "\' function & basic block\" {\n";
    // 将node节点dump
    for (node_list::iterator node = nodes.begin(), node_end = nodes.end(); node != node_end; ++node)
    {
        if (llvm::dyn_cast<llvm::Instruction>(node->first))
            file << "\tNode" << node->first << "[shape=record, label=\"" << *(node->first) << "\"];\n";
        else
            file << "\tNode" << node->first << "[shape=record, label=\"" << node->second << "\"];\n";
    }

    for (edge_list::iterator edge = edges.begin(), edge_end = edges.end(); edge != edge_end; ++edge)
    {
        file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "\n";
    }
    // llvm::errs() << "DFG Write Done\n";
    file << "}\n";
    file.close();
}

llvm::StringRef getValueName(llvm::Value *v, int &num)
{
    std::string temp_result = "val";
    if (!v)
    {
        return "undefined";
    }
    if (v->getName().empty())
    {
        temp_result += std::to_string(num);
        num++;
    }
    else
    {
        temp_result = v->getName().str();
    }
    llvm::StringRef result(temp_result);
    return result;
}

void buildBBDataDep(llvm::BasicBlock *curBB, edge_list &inst_edges, edge_list &edges, node_list &nodes, int &emptyVarNum)
{
    for (llvm::BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end(); II != IEnd; ++II)
    {
        bool flag = false;
        llvm::Instruction *curII = &*II;
        switch (curII->getOpcode())
        {
        // 由于load和store对内存进行操作，需要对load指令和stroe指令单独进行处理
        case llvm::Instruction::Load:
        {
            llvm::LoadInst *linst = llvm::dyn_cast<llvm::LoadInst>(curII);
            llvm::Value *loadValPtr = linst->getPointerOperand();
            edges.push_back(edge(node(loadValPtr, getValueName(loadValPtr, emptyVarNum)), node(curII, getValueName(curII, emptyVarNum))));
            break;
        }
        case llvm::Instruction::Store:
        {
            llvm::StoreInst *sinst = llvm::dyn_cast<llvm::StoreInst>(curII);
            llvm::Value *storeValPtr = sinst->getPointerOperand();
            llvm::Value *storeVal = sinst->getValueOperand();
            edges.push_back(edge(node(storeVal, getValueName(storeVal, emptyVarNum)), node(curII, getValueName(curII, emptyVarNum))));
            edges.push_back(edge(node(storeValPtr, getValueName(storeValPtr, emptyVarNum)), node(curII, getValueName(curII, emptyVarNum))));
            break;
        }
        // case llvm::Instruction::PHI:
        // {
        //     continue;
        //     break;
        // }
        default:
        {
            if (curII->getOpcode() == llvm::Instruction::Call)
            {
                llvm::CallInst *ci = llvm::cast<llvm::CallInst>(curII);
                if (ci)
                {
                    auto func = ci->getCalledFunction();
                    if (func)
                    {
                        // if (func->isDeclaration() || func->isIntrinsic())
                        // if (!func->isDeclaration() || func->isIntrinsic())
                        // {
                        //     continue;
                        // }
                    }
                }
            }

            for (llvm::Instruction::op_iterator op = curII->op_begin(), opEnd = curII->op_end(); op != opEnd; ++op)
            {
                llvm::Instruction *tempIns;
                // if (llvm::dyn_cast<llvm::Instruction>(*op))
                // {
                edges.push_back(edge(node(op->get(), getValueName(op->get(), emptyVarNum)), node(curII, getValueName(curII, emptyVarNum))));
                // }
            }
            break;
        }
        }
        llvm::BasicBlock::iterator next = II;
        nodes.push_back(node(curII, getValueName(curII, emptyVarNum)));
        ++next;
        if (next != IEnd)
        {
            inst_edges.push_back(edge(node(curII, getValueName(curII, emptyVarNum)), node(&*next, getValueName(&*next, emptyVarNum))));
        }
    }
}

std::string splitStr(std::string filename, char split, int idx)
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

class Edge;
class Vertex;
class Graph;

class Edge
{
public:
    int64_t weight = 2;
    int64_t eid;
    std::shared_ptr<Vertex> frm;
    std::shared_ptr<Vertex> to;
    int isValidEdge = 0; // 1 valid

    Edge(int64_t _eid, std::shared_ptr<Vertex> _frm, std::shared_ptr<Vertex> _to) : eid(_eid), frm(_frm), to(_to) {}
    Edge(int64_t _eid) : eid(_eid)
    {
        frm = NULL;
        to = NULL;
    }

    // edges connect to insts that are from other bb or removed bb (seperateBr) are invalid
    void checkEdgeIsValid();

    void setFrmVertex(std::shared_ptr<Vertex> _frm)
    {
        this->frm = _frm;
    }
    void setToVertex(std::shared_ptr<Vertex> _to)
    {
        this->to = _to;
    }
};

class Vertex
{
public:
    int64_t vid = -1;
    int64_t inDegree = 0, outDegree = 0;
    llvm::Instruction *inst;
    int64_t weight = 0;
    double hwDelay = 0;
    double mergedHWDelay = 0.;
    int64_t vidInGraph = -1;
    int64_t fuID = -1;
    int64_t glbID = -1;

    std::string moduleName = "";

    // gep, load, store
    bool isMemOp = false;
    bool isGepOp = false;
    bool isCallOp = false;
    bool isPHIOp = false;

    std::vector<std::shared_ptr<Edge>> edges;

    Vertex(int64_t _vid, llvm::Instruction *_inst) : vid(_vid), inst(_inst)
    {
        int opcode = this->inst->getOpcode();
        if (opcode == llvm::Instruction::GetElementPtr)
        {
            this->isGepOp = true;
        }
        if (opcode == llvm::Instruction::Load ||
            opcode == llvm::Instruction::Store)
        {
            this->isMemOp = true;
        }

        if (opcode == llvm::Instruction::Call)
            this->isCallOp = true;

        if (opcode == llvm::Instruction::PHI)
            this->isPHIOp = true;
    }

    Vertex(int64_t _vid) : vid(_vid) {}
    void addEdge(std::shared_ptr<Edge> ePtr);
    void updateVertexDegree();
    void deleteEdge(std::shared_ptr<Edge> e);
};

class Graph
{
public:
    int64_t gid = -1, startOffset = -1, glbID = -1;
    std::vector<std::shared_ptr<Edge>> edges;
    std::vector<std::shared_ptr<Vertex>> vertices;
    std::unordered_map<llvm::Instruction *, std::shared_ptr<Vertex>> instToVertex;
    std::unordered_map<int64_t, int64_t> vidInGraphToVid;

    std::list<std::shared_ptr<Vertex>> roots;

    std::unordered_map<int64_t, int64_t> idMap;

    Graph(int64_t _gid) : gid(_gid) {}
    Graph(int64_t _gid, edge_list _edges, node_list _nodes);
    Graph(int64_t _gid, edge_list _edges, node_list _nodes, int _startOffset);

    void updateVertexDegree();

    void addVertex(std::shared_ptr<Vertex> vPtr)
    {
        vertices.push_back(vPtr);
        instToVertex.insert(std::make_pair(vPtr->inst, vPtr));
    }
    void addEdge(std::shared_ptr<Edge> ePtr)
    {
        edges.push_back(ePtr);
    }
    std::shared_ptr<Vertex> findVertexByInst(llvm::Instruction *_inst);
    void initialize(int64_t _gid, edge_list _edges, node_list _nodes, int _startOffset);
    void findRoots();
    void dumpDFGData(llvm::raw_fd_ostream &file, std::string moduleName, std::string funcName, int graphID);
    void computeWeights();
    void computeVidInGraph(bool removeMemOp, bool removeGepOp, bool removePHIOp);

private:
    std::shared_ptr<Edge> getEdge(int64_t frmVid, int64_t toVid);
    std::unordered_map<int64_t, int64_t> updateVertexID();
    void updateEdgeID();
    void updateEdges(std::unordered_map<int64_t, int64_t> idMap);
};

#endif