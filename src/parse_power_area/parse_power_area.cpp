#include <unordered_map>
#include <set>
#include <algorithm>
#include <assert.h>
#include <list>
#include <vector>
#include <limits>
#include <memory>
#include <time.h>

#include "../mining/utils.h"
#include "../mining/graph.h"
#include "../mining/PatternManager.h"
#include "../cost_model/cost_model.h"

#include "maps.h"

using namespace std;

std::vector<int> splitAndConvertToInt(std::string input)
{
    std::vector<int> numbers;
    std::istringstream ss(input);
    std::string token;

    while (std::getline(ss, token, ','))
    {
        try
        {
            numbers.push_back(std::stoi(token));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error converting string to int: " << e.what() << '\n';
            // Handle the error, e.g., ignore the conversion or break the loop
        }
    }

    return numbers;
}

int main(int argc, char **argv)
{

    std::string ir_file = std::string(argv[1]);
    std::string config_file = std::string(argv[2]);

    LLVMContext context;
    SMDiagnostic error;
    std::unique_ptr<Module> m;
    m = parseIRFile(ir_file, error, context);
    PatternManager *pm = new PatternManager(ir_file, *m);

    CostModel *cost_model = new CostModel(config_file);
    pm->opcode_delay = cost_model->opcode_delay;

    set<int> visit;

    double total_area = 0.;

    for (auto &func : *m)
    {
        string funcName = func.getName().str();
        if (funcName.find("encapsulated_function") != std::string::npos)
        {
            int opcode = -1;
            vector<int> fu_ops;
            if (llvm::MDNode *CustomMD = func.getMetadata(funcName))
            {
                int i = 0;
                for (const auto &op : CustomMD->operands())
                {
                    auto *meta = llvm::dyn_cast<llvm::MDString>(op.get());
                    if (i == 0)
                    {
                        opcode = std::stoi(meta->getString().str());
                    }
                    else if (i == 2)
                    {
                        fu_ops = splitAndConvertToInt(meta->getString().str());
                    }
                    i++;
                }
            }

            double op_energy = 0.;
            double op_area = 0.;
            for (auto op : fu_ops)
            {
                op_area += getAladdinArea(llvm::Instruction::getOpcodeName(op));
                op_energy += getAladdinEnergyDyn(llvm::Instruction::getOpcodeName(op));
            }

            if (!visit.count(opcode))
            {
                total_area += op_area;
                cout << "opcode=" << opcode << " energy=" << op_energy << "\n";
                visit.insert(opcode);
            }
            // cout << "fuops: \n";

            // cout << "\n";
        }
    }
    total_area += 5.981433e+00 * 32 * 32;
    cout << "total_area=" << total_area * 1e-6<< "\n";

    return 0;
}