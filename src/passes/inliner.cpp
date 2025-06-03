#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace
{
    static cl::opt<std::string> targetFunc("targetFunc", cl::desc("targetFunc"), cl::Optional);

    struct BottomUpInliner : public CallGraphSCCPass
    {
        static char ID;

        BottomUpInliner() : CallGraphSCCPass(ID) {}

        bool inlineFunc(Function *F)
        {
            bool changed = false;
            std::vector<CallBase *> instructionsToInline;
            for (auto &BB : *F)
            {
                for (auto &I : BB)
                {
                    if (auto *CB = dyn_cast<CallBase>(&I))
                    {
                        Function *callee = CB->getCalledFunction();
                        if (callee && !callee->isDeclaration())
                        {
                            instructionsToInline.push_back(CB);
                        }
                    }
                }
            }

            // Inline the call instruction if possible
            for (auto CB : instructionsToInline)
            {
                Function *callee = CB->getCalledFunction();
                // errs() << *CB << "\n";

                // inline all called functions first
                inlineFunc(callee);
            }

            for (auto CB : instructionsToInline)
            {
                InlineFunctionInfo IFI;
                auto result = InlineFunction(*CB, IFI);
                if (result.isSuccess())
                {
                    changed = true;
                }
            }

            return changed;
        }

        bool runOnSCC(CallGraphSCC &SCC) override
        {
            bool Changed = false;

            // Iterate over the functions in the SCC in reverse order
            for (llvm::CallGraphNode *CGN : llvm::reverse(SCC))
            {
                Function *F = CGN->getFunction();
                if (!F)
                    continue;
                
                // std::string fname = F->getName().str();
                // std::string::size_type idx=fname.find("shiftup");//在a中查找b.
                // if (idx != std::string::npos)
                // {
                //     errs() << "Intersect function found: " << fname << "\n";
                // }

                // errs() << "F.name=" << F->getName().str() << "\n";
                if (F->getName().str() != targetFunc)
                    continue;
                if (F->isDeclaration())
                    continue;

                Changed = inlineFunc(F);

                // Inline the function if it has calls that can be inlined
            }
            return Changed;
        }
    };
}

char BottomUpInliner::ID = 0;
static RegisterPass<BottomUpInliner> X("bottom-up-inline", "Recursively Inline Functions and Callees", false, false);
