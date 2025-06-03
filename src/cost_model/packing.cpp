#include "packing.h"
#include "llvm/IR/Verifier.h"
#include <stack>
#include <unordered_set>

using namespace std;

class OrderedInsnSet
{
public:
    void insert(Instruction *insn)
    {
        if (set.find(insn) == set.end())
        { // Only insert if not already present
            set.insert(insn);
            order.push_back(insn);
        }
    }

    std::unordered_set<Instruction *> set; // To check for duplicates
    std::vector<Instruction *> order;      // To maintain order
};

// this creates a fake basic block and will be removed afterward
TempBasicBlock *copy_bb(BasicBlock *original)
{
    TempBasicBlock *temp_block = new TempBasicBlock();
    BasicBlock *block = BasicBlock::Create(original->getContext(), "copy_block", original->getParent());

    temp_block->bb = block;
    IRBuilder<> builder(block);

    block->setName(original->getName() + "_copy");

    for (Instruction &insn : *original)
    {
        Instruction *copy = insn.clone();
        copy->copyMetadata(insn);
        copy->setName(insn.getName());

        temp_block->orig_to_copy.insert(make_pair(&insn, copy));
        temp_block->copy_to_orig.insert(make_pair(copy, &insn));

        block->getInstList().push_back(copy);
    }

    // update operands
    for (Instruction &insn : *original)
    {
        vector<Value *> new_operands;
        for (Value *operand : insn.operands())
        {
            if (Instruction *op_insn = dyn_cast<Instruction>(operand))
            {
                if (temp_block->orig_to_copy.find(op_insn) != temp_block->orig_to_copy.end())
                {
                    new_operands.push_back(temp_block->orig_to_copy[op_insn]);
                }
                else
                {
                    new_operands.push_back(op_insn);
                }
            }
            else
            {
                new_operands.push_back(operand); // Non-instruction values (constants, etc.)
            }
        }

        Instruction *copy = temp_block->orig_to_copy[&insn];

        for (size_t i = 0; i < new_operands.size(); ++i)
        {
            copy->setOperand(i, new_operands[i]);
        }
    }
    return temp_block;
}

bool has_dependency_ending_in_vector(Instruction *inst, set<Instruction *> inst_set)
{
    stack<Instruction *> worklist;
    set<Instruction *> visited;

    worklist.push(inst);

    while (!worklist.empty())
    {
        Instruction *current = worklist.top();
        worklist.pop();

        if (visited.count(current))
            continue;
        visited.insert(current);

        // If current is within the vector, dependency ends within the vector
        if (inst_set.count(current))
        {
            return true;
        }

        // Continue exploring operands of current instruction
        for (Use &U : current->operands())
        {
            if (Instruction *operand_inst = dyn_cast<Instruction>(U.get()))
            {
                if (!visited.count(operand_inst))
                {
                    worklist.push(operand_inst);
                }
            }
        }
    }

    return false;
}

bool is_later_in_block(Instruction *A, Instruction *B)
{
    if (!B)
        return true;

    // Both instructions must be in the same basic block.
    if (A->getParent() != B->getParent())
    {
        return false;
    }

    // Iterate through the instructions in the block until you find B.
    for (auto &I : *A->getParent())
    {
        if (&I == A)
        {
            return false; // A appears first or they are the same instruction.
        }
        if (&I == B)
        {
            return true; // B appears first, so A is later.
        }
    }
    return false; // A was not found, which should not happen if both are in the same block.
}

void move_instruction_after(Instruction *insn, Instruction *target_insn)
{
    // errs()<<"move from"<<*I<<" to"<<*TargetInst<<"\n";
    // Remove the instruction from its current position
    insn->removeFromParent();

    // Insert the instruction after the target instruction
    insn->insertAfter(target_insn);

    for (auto UI = insn->use_begin(), UE = insn->use_end(); UI != UE;)
    {
        Use &U = *UI++; // Increment iterator before using, to safely remove/replace
        Instruction *user = dyn_cast<Instruction>(U.getUser());

        // Check if the user is outside the new function
        if (user)
        {
            if (user->getParent()->getName() == insn->getParent()->getName() && !isa<PHINode>(user))
            {
                if (!is_later_in_block(user, insn))
                {
                    // errs() << "       User=" << *User << "\n";
                    move_instruction_after(user, insn);
                }
            }
        }
    }
}

// return true if non-convex
bool dfs_check_convex(set<Instruction *> insn_set, Instruction *insn)
{
    bool ret = false;
    for (Use &U : insn->operands())
    {
        if (Instruction *operand_inst = dyn_cast<Instruction>(U.get()))
        {
            if (!insn_set.count(operand_inst))
            {
                // If operand is outside the vector, check if the dependency chain ends in the vector
                if (!isa<PHINode>(operand_inst) && has_dependency_ending_in_vector(operand_inst, insn_set))
                {
                    return true; // Found an external dependency that ends in the vector
                }
            }
            else
            {
                ret |= dfs_check_convex(insn_set, operand_inst);
            }
        }
    }

    return ret; // No problematic dependencies found
}
bool is_in_another_pattern(Value *op, vector<Instruction *> another, string bbname)
{

    if (Instruction *dep_insn = dyn_cast<Instruction>(op))
    {
        if (bbname == dep_insn->getParent()->getName())
        {
            if (find(another.begin(), another.end(), dep_insn) != another.end())
            {
                return true;
            }
        }
    }

    return false;
}

bool check_cycle(vector<Instruction *> v1, vector<Instruction *> v2)
{
    bool b1 = false;
    bool b2 = false;

    for (auto I : v1)
    {
        for (unsigned i = 0; i < I->getNumOperands(); ++i)
        {
            Value *op = I->getOperand(i);
            if (isa<Constant>(op))
                continue;

            if (is_in_another_pattern(op, v2, I->getParent()->getName().str()))
            {
                b1 = true;
                break;
            }
        }
    }

    for (auto I : v2)
    {
        for (unsigned i = 0; i < I->getNumOperands(); ++i)
        {
            Value *op = I->getOperand(i);
            if (isa<Constant>(op))
                continue;
            if (is_in_another_pattern(op, v1, I->getParent()->getName().str()))
            {
                b2 = true;
                break;
            }
        }
    }
    return b1 & b2;
}

// avoid enca function cycle
vector<vector<Instruction *>> remove_cycle(vector<vector<Instruction *>> candidate,
                                           vector<double> &delays,
                                           vector<double> cc_delay,
                                           vector<int> &stage_cycles,
                                           vector<int> cc_stage_cycles)
{
    vector<vector<Instruction *>> ret;
    int size = candidate.size();
    vector<vector<Instruction *>> prev_insns;

    for (int i = 0; i < size; i++)
    {
        auto component = candidate[i];
        vector<Instruction *> tmp_insns;
        for (auto insn : component)
        {
            tmp_insns.push_back(insn);
        }

        bool cycle = false;
        for (int j = 0; j < prev_insns.size(); j++)
        {
            if (check_cycle(prev_insns[j], tmp_insns))
            {
                cycle = true;
                break;
            }
        }

        if (!cycle)
        {
            prev_insns.push_back(tmp_insns);
            ret.push_back(component);

            delays.push_back(cc_delay[i]);
            stage_cycles.push_back(cc_stage_cycles[i]);
        }
    }

    return ret;
}

void customize(
    vector<vector<Instruction *>> candidate,
    Module &M,
    LLVMContext &context,
    int threshold, vector<double> cc_delay, vector<int> cc_stage_cycles)
{
    DataLayout DL(&M); // Data layout to compute sizes
    // Declare malloc function in the module
    FunctionType *malloc_func_type = FunctionType::get(Type::getInt8PtrTy(context), {Type::getInt64Ty(context)}, false);
    FunctionCallee malloc_func = M.getOrInsertFunction("malloc", malloc_func_type);

    vector<double> delays;
    vector<int> stage_cycles;
    auto ret = remove_cycle(candidate, delays, cc_delay, stage_cycles, cc_stage_cycles);

    int i = 0;
    for (auto component : ret)
    {
        set<Value *> outputs_in_new_function;
        OrderedInsnSet output_insns;

        for (auto insn : component)
        {
            outputs_in_new_function.insert(insn); // Track outputs within the new function
        }

        BasicBlock *BB = component[0]->getParent();

        set<Instruction *> insn_set(component.begin(), component.end());

        bool nonConvex = false;
        for (auto I : component)
            nonConvex |= dfs_check_convex(insn_set, I);
        // errs() << "convex=" << !nonConvex << "\n";
        if (nonConvex)
            continue;

        // Identify output instructions (those not used within InstructionsToMove)
        for (Instruction *I : component)
        {
            // cout << "move :" << insn_to_string(I) << "\n";
            bool is_output = false; // Assume it's not an output until proven otherwise
            int n = 0;

            for (auto &Use : I->uses())
            {
                n++;
                if (Instruction *User = dyn_cast<Instruction>(Use.getUser()))
                {
                    // Check if the user is not in InstructionsToMove
                    if (find(component.begin(), component.end(), User) == component.end())
                    {
                        is_output = true;
                        break;
                    }
                }
            }

            // Add to OutputInstructions if it's used outside of InstructionsToMove and has uses
            if (is_output && n != 0)
            {
                // errs() << "output " << *I << "\n";
                output_insns.insert(I);
            }
        }

        // Determine the types of inputs and outputs
        vector<Type *> input_types;
        vector<Value *> args;
        set<Value *> args_set; // To ensure we don't add the same argument multiple times

        Instruction *last_dep = nullptr;
        // Add function arguments (inputs) and map them to the new function
        for (Instruction *I : component)
        {
            for (unsigned i = 0; i < I->getNumOperands(); ++i)
            {
                Value *Op = I->getOperand(i);
                // If the operand is not produced by another instruction that will be moved,
                // and it is not already in ArgsSet, add it as an input argument
                if (outputs_in_new_function.find(Op) == outputs_in_new_function.end() &&
                    args_set.find(Op) == args_set.end())
                {
                    if (isa<Constant>(Op))
                        continue;

                    input_types.push_back(Op->getType());
                    args.push_back(Op);
                    args_set.insert(Op); // Track added arguments

                    if (Instruction *dep_insn = dyn_cast<Instruction>(Op))
                    {
                        if (I->getParent()->getName() == dep_insn->getParent()->getName())
                        {
                            if (is_later_in_block(dep_insn, last_dep))
                            {
                                last_dep = dep_insn;
                            }
                        }
                    }
                }
            }
        }

        // Split the inputs into multiple structs based on the threshold
        unsigned num_structs = (args.size() + threshold - 1) / threshold;
        vector<StructType *> struct_types;

        for (unsigned i = 0; i < num_structs; ++i)
        {
            unsigned start_idx = i * threshold;
            unsigned end_idx = min(start_idx + threshold, (unsigned)args.size());

            vector<Type *> sub_input_types(input_types.begin() + start_idx, input_types.begin() + end_idx);
            StructType *input_struct_type = StructType::create(context, sub_input_types, "InputStruct_" + to_string(i));
            struct_types.push_back(input_struct_type);
        }

        // Create the return type as a structure that includes all outputs
        vector<Type *> output_types;
        for (Instruction *I : output_insns.order)
        {
            output_types.push_back(I->getType());
        }

        bool no_packing = false;
        if (args.size() + output_types.size() <= 4)
        {
            // llvm::errs() << "no packing\n";
            no_packing = true;
        }

        bool single_ret = false;
        if (output_types.size() == 1)
        {
            // llvm::errs() << "single ret\n";
            single_ret = true;
        }

        // Define the new encapsulated function type (this will use multiple structs as arguments)
        vector<Type *> encaps_func_args;

        if (no_packing)
        {
            encaps_func_args = input_types;
        }
        else
        {
            for (auto *struct_type : struct_types)
            {
                encaps_func_args.push_back(struct_type->getPointerTo());
            }
        }
        FunctionType *encaps_func_type = nullptr;
        Type *ret_type = nullptr;

        if (single_ret)
        {
            encaps_func_type = FunctionType::get(output_types[0], encaps_func_args, false);
        }
        else
        {
            ret_type = StructType::get(context, output_types);
            encaps_func_type = FunctionType::get(ret_type, encaps_func_args, false);
        }

        // Create the new encapsulated function
        Function *encaps_func = Function::Create(encaps_func_type, Function::InternalLinkage, "encapsulated_function_" + BB->getName(), &M);
        BasicBlock *EncapsBB = BasicBlock::Create(context, "entry", encaps_func);
        IRBuilder<> encaps_builder(EncapsBB);

        // Map function arguments to the original operands in the encapsulated instructions
        auto ArgIt = encaps_func->arg_begin();
        map<Value *, Value *> arg_to_val_map;
        vector<Value *> StructPointers;

        if (no_packing)
        {
            for (Value *Arg : args)
            {
                ArgIt->setName(Arg->getName());
                arg_to_val_map[Arg] = &*ArgIt; // Map the argument in the new function to the original operand
                ++ArgIt;
            }
        }
        else
        {
            for (unsigned i = 0; i < num_structs; ++i)
            {
                StructPointers.push_back(&*ArgIt++);
            }

            for (unsigned i = 0; i < StructPointers.size(); ++i)
            {
                Value *load_struct = encaps_builder.CreateLoad(struct_types[i], StructPointers[i], "loaded_struct_" + to_string(i));

                unsigned start_idx = i * threshold;
                unsigned end_idx = min(start_idx + threshold, (unsigned)args.size());

                for (unsigned j = start_idx; j < end_idx; ++j)
                {
                    Value *extracted_value = encaps_builder.CreateExtractValue(load_struct, {j - start_idx}, "extracted_" + to_string(j));
                    arg_to_val_map[args[j]] = extracted_value;
                }
            }
        }

        // Update the operands of instructions to point to the extracted values from the structs
        for (Instruction *I : component)
        {
            for (unsigned i = 0; i < I->getNumOperands(); ++i)
            {
                Value *Op = I->getOperand(i);
                if (arg_to_val_map.find(Op) != arg_to_val_map.end())
                {
                    I->setOperand(i, arg_to_val_map[Op]); // Replace operand with argument in new function
                }
            }
        }

        vector<Function *> pack_funcs;
        if (!no_packing)
        {
            // Create multiple packing functions, one for each struct
            for (unsigned i = 0; i < struct_types.size(); ++i)
            {
                unsigned start_idx = i * threshold;
                unsigned end_idx = min(start_idx + threshold, (unsigned)args.size());

                vector<Type *> pack_func_input_types(input_types.begin() + start_idx, input_types.begin() + end_idx);
                if (i > 0)
                {
                    pack_func_input_types.push_back(struct_types[i - 1]->getPointerTo());
                }
                FunctionType *pack_func_type = FunctionType::get(struct_types[i]->getPointerTo(), pack_func_input_types, false);

                // Create the packing function
                Function *pack_func = Function::Create(pack_func_type, Function::InternalLinkage, "packing_function_" + to_string(i), &M);
                BasicBlock *pack_bb = BasicBlock::Create(context, "entry", pack_func);
                IRBuilder<> pack_builder(pack_bb);

                MDNode *pack_metadata = MDNode::get(context, MDString::get(context, "PackingFunction"));
                pack_func->setMetadata("packing_func", pack_metadata);

                // Allocate memory for the struct using malloc
                auto IntPtrTy = Type::getInt64Ty(context); // 64-bit pointer size
                Value *SizeOfStruct = ConstantInt::get(IntPtrTy, DL.getTypeAllocSize(struct_types[i]));
                Value *input_struct_pack_func = pack_builder.CreateCall(malloc_func, {SizeOfStruct});
                input_struct_pack_func = pack_builder.CreateBitCast(input_struct_pack_func, struct_types[i]->getPointerTo(), "input_struct_ptr");

                Value *InputStruct = UndefValue::get(struct_types[i]);

                int TypeSize = pack_func_input_types.size();
                if (i > 0)
                {
                    TypeSize -= 1;
                }
                for (unsigned j = 0; j < TypeSize; ++j)
                {
                    InputStruct = pack_builder.CreateInsertValue(InputStruct, pack_func->getArg(j), j);
                }

                pack_builder.CreateStore(InputStruct, input_struct_pack_func);
                pack_builder.CreateRet(input_struct_pack_func);

                pack_funcs.push_back(pack_func);
            }
        }

        // Replace the original instructions with calls to packing functions and encapsulated function
        IRBuilder<> Builder(context);

        if (last_dep)
        {
            auto iter = last_dep->getIterator();
            while (isa<PHINode>(iter))
            {
                iter++;
            }
            Builder.SetInsertPoint(last_dep->getParent(), ++iter);
        }
        else
        {
            auto inst = component.back();
            Builder.SetInsertPoint(inst->getParent(), inst->getIterator());
        }

        CallInst *call = nullptr;
        if (!no_packing)
        {
            vector<Value *> packed_structs;
            for (unsigned i = 0; i < pack_funcs.size(); ++i)
            {
                unsigned start_idx = i * threshold;
                unsigned end_idx = min(start_idx + threshold, (unsigned)args.size());

                vector<Value *> pack_func_args(args.begin() + start_idx, args.begin() + end_idx);
                if (i > 0)
                {
                    pack_func_args.push_back(packed_structs[i - 1]);
                }
                packed_structs.push_back(Builder.CreateCall(pack_funcs[i], pack_func_args));
            }
            call = Builder.CreateCall(encaps_func, packed_structs, "call_func");
        }
        else
        {
            call = Builder.CreateCall(encaps_func, args, "call_func");
        }

        // MDNode *encap_metadata = MDNode::get(context, MDString::get(context, to_string(delays[i])));
        // encaps_func->setMetadata("encaps_func", encap_metadata);

        std::vector<Metadata *> values;
        values.push_back(MDString::get(context, to_string(delays[i])));
        values.push_back(MDString::get(context, to_string(stage_cycles[i])));
        MDNode *metanode = MDNode::get(context, values);
        encaps_func->setMetadata("encaps_func", metanode);

        // Create a call to the encapsulated function with the struct pointers

        vector<Instruction *> suffixInsns;
        map<Instruction *, Instruction *> target_map;

        unsigned index = 0;
        for (Instruction *I : output_insns.order)
        {
            Value *extracted_value = nullptr;
            if (!single_ret)
                extracted_value = Builder.CreateExtractValue(call, index++, I->getName() + ".extracted");
            // Iterate over the uses of the instruction
            for (auto UI = I->use_begin(), UE = I->use_end(); UI != UE;)
            {
                Use &U = *UI++; // Increment iterator before using, to safely remove/replace
                Instruction *User = dyn_cast<Instruction>(U.getUser());

                // Check if the user is outside the new function
                if (User && User->getFunction() != encaps_func && User->getParent()->getName() == I->getParent()->getName())
                {
                    if (find(component.begin(), component.end(), User) == component.end())
                    {
                        // llvm::errs() << *User << "\n";
                        // llvm::errs() << "User->get=" << User->getFunction()->getName().str() << " " << EncapsFunc->getName().str() << "\n";
                        // llvm::errs() << "Replacing use in: " << *User << "\n";
                        if (!single_ret)
                        {
                            U.set(extracted_value); // Replace the use with the extracted value
                            Instruction *tmp = dyn_cast<Instruction>(extracted_value);
                            if (!is_later_in_block(User, tmp))
                            {
                                // suffixInsns.push_back(User);
                                // if (targetMap.find(User) != targetMap.end())
                                // {
                                //     assert(targetMap[User] != tmp);
                                // }
                                // targetMap.insert(make_pair(User, tmp));
                                if (!isa<PHINode>(User) && User->getParent()->getName() == tmp->getParent()->getName())
                                    move_instruction_after(User, tmp);
                            }
                        }
                        else
                        {
                            U.set(call); // Replace the use with the extracted value
                            if (!is_later_in_block(User, call))
                            {
                                // suffixInsns.push_back(User);
                                // if (targetMap.find(User) != targetMap.end())
                                // {
                                //     assert(targetMap[User] != Call);
                                // }
                                // targetMap.insert(make_pair(User, Call));
                                if (!isa<PHINode>(User) && User->getParent()->getName() == call->getParent()->getName())
                                    move_instruction_after(User, call);
                            }
                        }
                    }
                }
                else
                {
                    // llvm::errs() << "Use inside new function, not replacing: " << *User << "\n";
                }
            }
        }

        // for (auto I : suffixInsns)
        // {
        //     // errs() << "suffix insn=" << *I << "\n";
        //     if (!isa<PHINode>(I) && I->getParent()->getName() == targetMap[I]->getParent()->getName())
        //         moveInstructionAfter(I, targetMap[I]);
        // }

        for (Instruction *I : component)
        {
            I->removeFromParent();    // Remove the instruction from its original location
            encaps_builder.Insert(I); // Insert the instruction into the new function
        }

        // Create a struct to hold the return values
        if (!single_ret)
        {
            Value *ret_struct = UndefValue::get(ret_type);
            index = 0;
            for (Instruction *I : output_insns.order)
            {
                ret_struct = encaps_builder.CreateInsertValue(ret_struct, I, index++);
            }
            encaps_builder.CreateRet(ret_struct);
        }
        else
        {
            encaps_builder.CreateRet(*(output_insns.order.begin()));
        }

        i++;
    }
}

DependencyGraph build_data_dependency_graph(map<Instruction *, int> insn_uid_map, const std::vector<Instruction *> &instructions)
{
    DependencyGraph dep_graph;

    // Create a set of instructions for fast lookup
    std::set<Instruction *> instruction_set(instructions.begin(), instructions.end());

    // Iterate over each instruction in the input vector
    for (auto *insn : instructions)
    {
        // Ensure the instruction has an entry in the graph
        dep_graph[insn] = vector<Instruction *>();

        // Iterate over each operand of the current instruction
        for (auto &operand : insn->operands())
        {
            // Check if the operand is also an instruction and in the input set
            if (auto *operand_inst = dyn_cast<Instruction>(operand))
            {
                if (instruction_set.find(operand_inst) != instruction_set.end())
                {
                    // Add an edge from operandInst (producer) to insn (consumer)
                    // porducer uid < consumer uid to avoid cycle
                    if (insn_uid_map[operand_inst] < insn_uid_map[insn])
                    {
                        dep_graph[operand_inst].push_back(insn);
                    }
                }
            }
        }
    }

    return dep_graph;
}