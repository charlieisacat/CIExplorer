#include "llvm/Pass.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/CommandLine.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include "llvm/Support/raw_ostream.h"

#include <string>
#include <map>
#include <fstream>
#include <set>

using namespace llvm;
using namespace std;

static cl::opt<string> excludeList("excl",
                                   cl::desc("File with the exclusion list for functions"), cl::Optional);
static cl::opt<int> inlineSteps("inlStep",
                                cl::desc("Fraction of the number of functions to inline"), cl::Optional);

struct listBBs : public FunctionPass
{
    static char ID;

    listBBs() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override
    {
        for (BasicBlock &BB : F)
            errs() << BB.getName() << '\n';
        return false;
    }
};

struct markInline : public ModulePass
{
    static char ID;

    markInline() : ModulePass(ID) {}

    bool runOnModule(Module &M) override
    {
        // Get the exclusion list
        set<string> exclude;
        if (!excludeList.empty())
        {
            fstream exclfile;
            string fname;
            exclfile.open(excludeList);
            while (exclfile >> fname)
                exclude.insert(fname);
            exclfile.close();
        }

        // int counter = 0;
        // int max_inline_func = M.size() / inlineSteps;
        for (Function &F : M)
            if (!exclude.count(F.getName().str()) and !F.isIntrinsic())
            {
                F.removeFnAttr(Attribute::OptimizeNone);
                if (F.getName().str() != "main")
                {
                    F.removeFnAttr(Attribute::NoInline);
                    F.addFnAttr(Attribute::AlwaysInline);
                }
                // counter++;
            }

        return true;
    }
};

struct renameBBs : public ModulePass
{
    static char ID;

    map<string, unsigned> Names;

    renameBBs() : ModulePass(ID) {}

    bool runOnModule(Module &M) override
    {
        int i = 0;
        for (Function &F : M)
        {
            for (BasicBlock &BB : F)
            {
                BB.setName("r" + to_string(i));
                i++;
            }
        }
        return true;
    }
};

char renameBBs::ID = 0;
static RegisterPass<renameBBs> a("renameBBs", "Renames BBs with  unique names",
                                 false, false);
char listBBs::ID = 1;
static RegisterPass<listBBs> b("listBBs", "List the names of all BBs", false,
                               false);
char markInline::ID = 2;
static RegisterPass<markInline> c("markInline", "Mark functions as always inline",
                                  false, false);

static RegisterStandardPasses A(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM)
    { PM.add(new renameBBs()); });

static RegisterStandardPasses B(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM)
    { PM.add(new listBBs()); });

static RegisterStandardPasses C(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM)
    { PM.add(new markInline()); });
