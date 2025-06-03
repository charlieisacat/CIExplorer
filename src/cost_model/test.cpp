#include "packing.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

#include <filesystem>

#include "../mining/llvm.h"
#include "cost_model.h"
#include "../mining/PatternManager.h"

using namespace llvm;
namespace fs = std::filesystem;

int main(int argc, char **argv)
{
    string ir_file = string(argv[1]);

    LLVMContext context;
    SMDiagnostic error;
    unique_ptr<Module> m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);

    vector<BasicBlock *> bblist;

    for (auto &func : *m)
    {
        for (auto &bb : func)
        {
            bblist.push_back(&bb);
        }
    }

    CostModel *cost_model = new CostModel("../config_broadwell.yml");

    for (BasicBlock *bb : bblist)
    {
        if (bb->getName().str() != "r59")
            continue;

        // this basic block will never access, and may not pass verification
        TempBasicBlock *new_bb = copy_bb(bb);

        errs() << "name=" << new_bb->bb->getName() << " orig=" << bb->getName() << "\n";
        for (auto &insn : *(new_bb->bb))
        {
            Instruction *orig = new_bb->copy_to_orig[&insn];
            int uid = pm->insn_uid_map[orig];
            errs() << *orig << " uid=" << uid << "\n";
        }

        customize({{new_bb->orig_to_copy[pm->uid_insn_map[1353]], new_bb->orig_to_copy[pm->uid_insn_map[1354]]},
                   {new_bb->orig_to_copy[pm->uid_insn_map[1366]], new_bb->orig_to_copy[pm->uid_insn_map[1368]], new_bb->orig_to_copy[pm->uid_insn_map[1369]]},
                   {new_bb->orig_to_copy[pm->uid_insn_map[1363]], new_bb->orig_to_copy[pm->uid_insn_map[1364]]}},
                  *m, context, 3, {2, 2, 2}, {1,1,1});

        vector<Instruction*> insns;
        for (Instruction &I : *new_bb->bb)
        {
            insns.push_back(&I);
            errs()<<"push "<<I<<"\n";
        }
        // errs()<<"size="<<insns.size()<<"\n";

        double score = cost_model->evaluate_o3(insns, 100);
        // double score = cost_model->evaluate_ino(insns, 100);
        errs()<<"score="<<score<<"\n";

        // Verify the module
        // if (llvm::verifyModule(*m, &llvm::errs()))
        // {
        //     llvm::errs() << "Error: Component verification failed!\n";
        // }
    }

    // Write the modified module to a file
    std::error_code EC;
    raw_fd_ostream OS("test.ll", EC, sys::fs::OF_None);
    if (EC)
    {
        errs() << "Could not open file: " << EC.message();
        return 1;
    }

    m->print(OS, nullptr);
    OS.flush();
}