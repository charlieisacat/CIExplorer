#include "graph.h"

#include "llvm/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "llvm/IR/IRBuilder.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicInst.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <map>
#include <fstream>

using namespace llvm;
using namespace std;

static cl::opt<std::string> outdir("outdir", cl::desc("Specify output dir"), cl::value_desc("output dir"));
static cl::opt<bool> removeMemOp("removeMemOp", cl::desc("Specify if remove memory ops"), cl::value_desc("remove mem op"));
static cl::opt<bool> removeGepOp("removeGepOp", cl::desc("Specify if remove GEP ops"), cl::value_desc("remove Gep op"));
static cl::opt<bool> removePHIOp("removePHIOp", cl::desc("Specify if remove PHI ops"), cl::value_desc("remove PHI op"));
static cl::opt<string> includeList("include",
                                   cl::desc("File with the inclusion list for functions"), cl::Optional);
// static cl::opt<string> dynamicInfoFile("dynInf", cl::desc("File with profiling information per BB"), cl::Optional);

void readDynInfo(string filename, map<string, double> *profileMap, map<string, double> *tCycleMap,
                 map<string, long> *iterMap)
{
    fstream file;

    if (filename.empty())
    {
        errs() << "No dynamic info specified - assuming equal weights\n";
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
        // errs() << "sigvalue=" << sigvalue << " tvalue=" << tvalue << " itervalue=" << itervalue << "\n";
    }
}

namespace
{
    struct bbDFG : public ModulePass
    {
    private:
    public:
        static char ID;
        edge_list inst_edges; // 存储每条指令之间的先后执行顺序
        edge_list edges;      // 存储data flow的边
        node_list nodes;      // 存储每条指令
        int emptyVarNum;
        bbDFG() : ModulePass(ID) {}

        // map<string, double> profileMap;
        // map<string, double> totalCycleMap;
        // map<string, long> iterMap;

        bool runOnModule(Module &M) override
        {

            set<string> include;
            bool filter = false;
            if (!includeList.empty())
            {
                fstream inclfile;
                string fname;
                inclfile.open(includeList);
                while (inclfile >> fname)
                    include.insert(fname);
                inclfile.close();

                filter = true;
            }

            errs()<<"filter="<<filter<<"\n";

            emptyVarNum = 0;
            int offset = 0;
            for (Function &F : M)
            {
                std::string funcName = F.getName().str();
                if (F.isIntrinsic() || F.isDeclaration())
                {
                    // errs() << "bbDFG Pass: Function " << funcName << " is marked always inline so skip...\n";
                    continue;
                }
                // errs() << " Processing function " << funcName << "\n";

                std::error_code error;
                enum llvm::sys::fs::OpenFlags F_None;

                Module *m = F.getParent();
                std::string filename = splitStr(m->getName().str(), '/', -1);
                std::string moduleName = splitStr(filename, '.', 0);

                std::string fileNameStr = outdir + "/" + moduleName + "." + funcName + ".data";
                llvm::StringRef fileName(fileNameStr);
                llvm::raw_fd_ostream file(fileName, error, F_None);

                // readDynInfo(dynamicInfoFile, &profileMap, &totalCycleMap, &iterMap);

                int bbNum = 0;
                // errs() << "Processing on Function " << F.getName().str() << "\n";
                for (Function::iterator BB = F.begin(), BEnd = F.end(); BB != BEnd; ++BB)
                {
                    edges.clear();
                    nodes.clear();
                    inst_edges.clear();

                    BasicBlock *curBB = &*BB;
                    std::string bbName = curBB->getName().str();

                    buildBBDataDep(curBB, inst_edges, edges, nodes, emptyVarNum);

                    // dumpDFG(outdir, nodes, edges, inst_edges, F.getName().str(), bbNum, emptyVarNum);

                    std::shared_ptr<Graph> bbGraph = std::make_shared<Graph>(bbNum, edges, nodes, offset);
                    offset += bbGraph->vertices.size();

                    bbGraph->computeVidInGraph(removeMemOp, removeGepOp, removePHIOp);
                    if (!filter || (filter && include.count(bbName)))
                    {
                        bbGraph->dumpDFGData(file, moduleName, funcName, bbNum);
                    }
                    bbNum++;
                }
                file.close();
            }

            return true;
        }
    };

    struct funcDFG : public FunctionPass
    {
    private:
    public:
        static char ID;
        edge_list inst_edges; // 存储每条指令之间的先后执行顺序
        edge_list edges;      // 存储data flow的边
        node_list nodes;      // 存储每条指令
        int emptyVarNum;
        funcDFG() : FunctionPass(ID) { emptyVarNum = 0; }

        // map<string, double> profileMap;
        // map<string, double> totalCycleMap;
        // map<string, long> iterMap;

        bool runOnFunction(Function &F) override
        {
            std::string funcName = F.getName().str();
            if (F.hasFnAttribute(llvm::Attribute::AlwaysInline) || F.isIntrinsic())
            {
                // errs() << "funcDFG Pass: Function " << funcName << " is marked always inline so skip...\n";
                return 0;
            }
            errs() << " Processing function " << funcName << "\n";

            std::error_code error;
            enum llvm::sys::fs::OpenFlags F_None;

            Module *m = F.getParent();
            std::string filename = splitStr(m->getName().str(), '/', -1);
            std::string moduleName = splitStr(filename, '.', 0);

            std::string fileNameStr = outdir + "/" + moduleName + "." + funcName + ".data";
            llvm::StringRef fileName(fileNameStr);
            llvm::raw_fd_ostream file(fileName, error, F_None);

            // readDynInfo(dynamicInfoFile, &profileMap, &totalCycleMap, &iterMap);

            edges.clear();
            nodes.clear();
            inst_edges.clear();

            // errs() << "Processing on Function " << F.getName().str() << "\n";
            for (Function::iterator BB = F.begin(), BEnd = F.end(); BB != BEnd; ++BB)
            {
                BasicBlock *curBB = &*BB;
                buildBBDataDep(curBB, inst_edges, edges, nodes, emptyVarNum);
            }

            std::shared_ptr<Graph> funcGraph = std::make_shared<Graph>(0, edges, nodes);

            funcGraph->computeVidInGraph(removeMemOp, removeGepOp, removePHIOp);
            funcGraph->dumpDFGData(file, moduleName, funcName, 0);
            // dumpDFG(outdir, nodes, edges, inst_edges, F.getName().str(), 0, emptyVarNum);

            file.close();
            return false;
        }
    };

    struct inOutDep : public ModulePass
    {
    private:
    public:
        static char ID;
        int emptyVarNum;
        inOutDep() : ModulePass(ID) {}

        bool runOnModule(Module &M) override
        {
            int glbInstID = 0;
            std::unordered_map<Instruction *, int> glbInstOrder;

            for (Function &F : M)
            {
                emptyVarNum = 0;
                edge_list inst_edges; // 存储每条指令之间的先后执行顺序
                edge_list edges;      // 存储data flow的边
                node_list nodes;      // 存储每条指令

                map<llvm::Value *, string> valueDummyMap;

                std::string funcName = F.getName().str();
                if (F.hasFnAttribute(llvm::Attribute::AlwaysInline) || F.isIntrinsic())
                {
                    // errs() << "inOutDep Pass: Function " << funcName << " is marked always inline so skip...\n";
                    continue;
                }

                errs() << " Processing function " << funcName << "\n";

                std::error_code error;
                enum llvm::sys::fs::OpenFlags F_None;

                Module *m = F.getParent();
                std::string filename = splitStr(m->getName().str(), '/', -1);
                std::string moduleName = splitStr(filename, '.', 0);

                std::string fileNameStr = outdir + "/" + moduleName + "." + funcName + ".dep";
                llvm::StringRef fileName(fileNameStr);
                llvm::raw_fd_ostream file(fileName, error, F_None);

                std::unordered_map<Instruction *, int> bbInstOrder;

                int bbInstID = 0;
                for (Function::iterator BB = F.begin(), BEnd = F.end(); BB != BEnd; ++BB)
                {
                    BasicBlock *curBB = &*BB;

                    buildBBDataDep(curBB, inst_edges, edges, nodes, emptyVarNum);

                    bbInstID = 0;
                    for (llvm::BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end(); II != IEnd; ++II)
                    {
                        llvm::Instruction *curII = &*II;

                        // if (curII->getOpcode() == llvm::Instruction::PHI)
                        // {
                        //     continue;
                        // }

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

                        bbInstOrder.insert(std::make_pair(curII, bbInstID));
                        bbInstID++;

                        glbInstOrder.insert(std::make_pair(curII, glbInstID));
                        glbInstID++;
                    }
                }
                std::shared_ptr<Graph> funcGraph = std::make_shared<Graph>(0, edges, nodes);

                // dump cross bb inst dep
                for (auto e : funcGraph->edges)
                {
                    auto frmV = e->frm;
                    auto toV = e->to;

                    if (!frmV || !toV)
                        continue;

                    llvm::Instruction *frmInst = frmV->inst;
                    llvm::Instruction *toInst = toV->inst;

                    BasicBlock *frmInstBB = frmInst->getParent();
                    BasicBlock *toInstBB = toInst->getParent();

                    auto n1 = llvm::Instruction::getOpcodeName(frmInst->getOpcode());
                    auto n2 = llvm::Instruction::getOpcodeName(toInst->getOpcode());

                    string frmBBName = frmInstBB->getName().str();
                    string toBBName = toInstBB->getName().str();

                    if (frmInstBB->getName().str() != toInstBB->getName().str())
                    {
                        int frmLnbrBB = bbInstOrder.find(frmInst)->second;
                        int toLnbrBB = bbInstOrder.find(toInst)->second;

                        int frmLnbrGlb = glbInstOrder.find(frmInst)->second;
                        int toLnbrGlb = glbInstOrder.find(toInst)->second;

                        file << "d " << moduleName << " " << funcName << " " << frmBBName << " " << toBBName << " " << frmLnbrBB << " " << toLnbrBB << " " << frmLnbrGlb << " " << toLnbrGlb << "\n";
                    }
                }
                int dummyID = 0;

                // dump params dep
                for (edge_list::iterator edge = edges.begin(), edge_end = edges.end(); edge != edge_end; ++edge)
                {
                    auto frmInst = llvm::dyn_cast<llvm::Instruction>(edge->first.first);
                    auto toInst = llvm::dyn_cast<llvm::Instruction>(edge->second.first);

                    if (frmInst && !toInst)
                    {
                        auto lnbrBB = bbInstOrder.find(frmInst)->second;
                        auto lnbrGlb = glbInstOrder.find(frmInst)->second;
                        string bbName = frmInst->getParent()->getName().str();
                        string instName = llvm::Instruction::getOpcodeName(frmInst->getOpcode());

                        auto iter = valueDummyMap.find(edge->second.first);
                        if (iter == valueDummyMap.end())
                        {
                            valueDummyMap.insert(make_pair(edge->second.first, "dummy" + to_string(dummyID)));
                            dummyID++;
                        }

                        string dummy = valueDummyMap[edge->second.first];

                        file << "a " << moduleName << " " << funcName << " " << bbName << " " << instName << " " << dummy << " " << lnbrBB << " " << lnbrGlb << "\n";
                    }
                    else if (toInst && !frmInst)
                    {
                        auto lnbrBB = bbInstOrder.find(toInst)->second;
                        auto lnbrGlb = glbInstOrder.find(toInst)->second;
                        string bbName = toInst->getParent()->getName().str();
                        string instName = llvm::Instruction::getOpcodeName(toInst->getOpcode());

                        auto iter = valueDummyMap.find(edge->first.first);
                        if (iter == valueDummyMap.end())
                        {
                            valueDummyMap.insert(make_pair(edge->first.first, "dummy" + to_string(dummyID)));
                            dummyID++;
                        }

                        string dummy = valueDummyMap[edge->first.first];
                        file << "a " << moduleName << " " << funcName << " " << bbName << " " << dummy << " " << instName << " " << lnbrBB << " " << lnbrGlb << "\n";
                    }
                }
                file.close();
            }

            return false;
        }
    };
}

char bbDFG::ID = 0;
static RegisterPass<bbDFG> a("bbDFG", "dump dfg per bb",
                             false, false);
char funcDFG::ID = 1;
static RegisterPass<funcDFG> b("funcDFG", "dump dfg per func", false,
                               false);

char inOutDep::ID = 2;
static RegisterPass<inOutDep> c("inOutDep", "input/output depdency", false,
                                false);

static RegisterStandardPasses A(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM)
    { PM.add(new bbDFG()); });

static RegisterStandardPasses B(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM)
    { PM.add(new funcDFG()); });

static RegisterStandardPasses C(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM)
    { PM.add(new inOutDep()); });