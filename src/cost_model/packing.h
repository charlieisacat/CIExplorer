#ifndef PACKING_H
#define PACKING_H

#include "../mining/llvm.h"
#include "../mining/utils.h"
#include "../mining/PatternManager.h"
#include "llvm/IR/IRBuilder.h"

#include <vector>
#include <map>

using namespace std;
using namespace llvm;

struct TempBasicBlock
{
    BasicBlock *bb;
    // orig to copy
    map<Instruction *, Instruction *> orig_to_copy;
    map<Instruction *, Instruction *> copy_to_orig;
};

TempBasicBlock *copy_bb(BasicBlock *original);
DependencyGraph build_data_dependency_graph(map<Instruction *, int> insn_uid_map, const std::vector<Instruction *> &instructions);

void customize(
    vector<vector<Instruction *>> candidate,
    Module &M,
    LLVMContext &context,
    int threshold, vector<double> cc_delays, vector<int> cc_stage_cycles);

#endif