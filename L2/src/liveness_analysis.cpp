#include <string>
#include <iostream>
#include <fstream>

#include <liveness_analysis.h>
#include <helper.h> 

using namespace std;

namespace L2{

    bool LivenessAnalysisBehavior::isLivenessContributor(const Item* var) {
        return var->kind() == ItemType::RegisterItem || var->kind() == ItemType::VariableItem || var->kind() == ItemType::MemoryItem;
    }

    void LivenessAnalysisBehavior::print_instruction_gen_kill(
        size_t cur_i, const livenessSets& ls
    ) {
        std::cout << cur_i << " gen set: ";
        bool first = true;
        for (const auto &s : ls.gen) {
            if (!first) std::cout << ", ";
            std::cout << s;
            first = false;
        }
        std::cout << "\n";

        std::cout << cur_i << " kill set: ";
        first = true;
        for (const auto &s : ls.kill) {
            if (!first) std::cout << ", ";
            std::cout << s;
            first = false;
        }
        std::cout << "\n\n";  
    }

    void LivenessAnalysisBehavior::generate_in_out_sets(Program p) {
        for (int i = 0; i < (int)livenessData.size(); i++) {
            auto& functionInstructions = p.functions[i]->instructions; 
            auto& functionLivenessData = livenessData[i]; 
            auto& functionLabelMap = labelMap[i]; 
            for (int j = (int)functionLivenessData.size()-1; j>=0; j--) {
                livenessSets& ls = functionLivenessData[j];
                Instruction* cur_instruction = functionInstructions[j];
                if (auto *ret = dynamic_cast<const Instruction_ret*>(cur_instruction)) {
                    // no successors, out is empty 
                } else if (auto *gt = dynamic_cast<const Instruction_goto*>(cur_instruction)) {
                    const std::string label = gt->label()->emit();
                    size_t label_instruction_index = functionLabelMap[label]; 
                    livenessSets& ls_label_instruction = functionLivenessData[label_instruction_index]; 
                    ls.out = ls_label_instruction.in;
                } else if (auto *cj = dynamic_cast<const Instruction_cjump*>(cur_instruction)) {
                    const std::string label = cj->label()->emit();
                    size_t label_instruction_index = functionLabelMap[label]; 
                    livenessSets& ls_label_instruction = functionLivenessData[label_instruction_index]; 
                    ls.out = ls_label_instruction.in;

                    livenessSets& ls_next_inst = functionLivenessData[j+1];
                    ls.out.insert(ls_next_inst.in.begin(), ls_next_inst.in.end()); 
                } else {
                    livenessSets& ls_next_inst = functionLivenessData[j+1]; 
                    ls.out = ls_next_inst.in; 
                }
                std::unordered_set out_kill_diff_set = set_difference(ls.out, ls.kill);
                ls.in = set_union(ls.gen, out_kill_diff_set);
            }
        }
    }

    void LivenessAnalysisBehavior::print_in_out_sets() {
    for (size_t f = 0; f < livenessData.size(); ++f) {
        std::cout << "Function " << f << ":\n";

        for (size_t i = 0; i < livenessData[f].size(); ++i) {
        const auto& ls = livenessData[f][i];

        auto printSet = [](const std::unordered_set<std::string>& s) {
            bool first = true;
            for (const auto& x : s) {
            if (!first) std::cout << ", ";
            std::cout << x;
            first = false;
            }
        };

        std::cout << "  Instr " << i << "\n";

        std::cout << "    IN  : { ";
        printSet(ls.in);
        std::cout << " }\n";

        std::cout << "    OUT : { ";
        printSet(ls.out);
        std::cout << " }\n";
        }
    }
    }


    void LivenessAnalysisBehavior::act(Program& p) {
        for (Function *f: p.functions) {
            f->accept(*this); 
        }
        generate_in_out_sets(p);
        print_in_out_sets();
    }

    void LivenessAnalysisBehavior::act(Function& f) {
        cur_i = 0; 
        livenessData.emplace_back(f.instructions.size()); 
        labelMap.emplace_back(); 
        for (Instruction *i: f.instructions) {
            i->accept(*this); 
            cur_i++; 
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_assignment& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* dst = i.dst(); 
        const Item* src = i.src(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(src)) {
            ls.gen.insert(src->emit(options));
        }
        if (isLivenessContributor(dst)) {
            if (dst->kind() == ItemType::MemoryItem) {
                ls.gen.insert(dst->emit(options)); 
            } else {
                ls.kill.insert(dst->emit(options));
            } 
        }
        print_instruction_gen_kill(cur_i, ls); 
    }

    void LivenessAnalysisBehavior::act(Instruction_stack_arg_assignment& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* dst = i.dst(); 

        EmitOptions options; 
        options.livenessAnalysis = true; 
        ls.kill.insert(dst->emit(options));
    }

    void LivenessAnalysisBehavior::act(Instruction_aop& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* dst = i.dst(); 
        const Item* src = i.rhs(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 

        if (isLivenessContributor(src)) {
            ls.gen.insert(src->emit(options));
        }
        ls.gen.insert(dst->emit(options)); 
        ls.kill.insert(dst->emit(options)); 
        print_instruction_gen_kill(cur_i, ls); 
    }

    void LivenessAnalysisBehavior::act(Instruction_sop& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* dst = i.dst(); 
        const Item* src = i.src(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 

        if (isLivenessContributor(src)) {
            ls.gen.insert(src->emit(options));
        }
        ls.gen.insert(dst->emit(options)); 
        ls.kill.insert(dst->emit(options)); 
        print_instruction_gen_kill(cur_i, ls); 
    }
    
    void LivenessAnalysisBehavior::act(Instruction_mem_aop& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* lhs = i.lhs(); 
        const Item* rhs = i.rhs(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(lhs)) {
            ls.gen.insert(lhs->emit(options));
            if (lhs->kind() != ItemType::MemoryItem) {
                ls.kill.insert(lhs->emit(options)); 
            }
        }
        if (isLivenessContributor(rhs)) {
            ls.gen.insert(rhs->emit(options));
        }
        print_instruction_gen_kill(cur_i, ls); 
    }

    void LivenessAnalysisBehavior::act(Instruction_cmp_assignment& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* dst = i.dst(); 
        const Item* lhs = i.lhs(); 
        const Item* rhs = i.rhs(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 

        ls.kill.insert(dst->emit(options)); 
        if (isLivenessContributor(lhs)) {
            ls.gen.insert(lhs->emit(options)); 
        }
        if (isLivenessContributor(rhs)) {
            ls.gen.insert(rhs->emit(options));
        }
        print_instruction_gen_kill(cur_i, ls); 
    }

    void LivenessAnalysisBehavior::act(Instruction_cjump& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* lhs = i.lhs(); 
        const Item* rhs = i.rhs(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(lhs)) {
            ls.gen.insert(lhs->emit(options)); 
        }
        if (isLivenessContributor(rhs)) {
            ls.gen.insert(rhs->emit(options));
        }
        print_instruction_gen_kill(cur_i, ls); 
    }

    void LivenessAnalysisBehavior::act(Instruction_label& i) {
        // empty gen + kill

        // Store instruction # -> label
        auto &lm = labelMap.back(); 
        lm[i.label()->emit()] = cur_i;
    }

    void LivenessAnalysisBehavior::act(Instruction_goto& i) {
        // empty gen + kill
    }

    void LivenessAnalysisBehavior::act(Instruction_ret& i) {
        auto &ls = livenessData.back()[cur_i];
        std::unordered_set<string> callee_save_registers = {"r12", "r13", "r14", "r15", "rbp", "rbx"}; 
        ls.gen.insert("rax"); 
        ls.gen.insert(callee_save_registers.begin(), callee_save_registers.end()); 
        print_instruction_gen_kill(cur_i, ls); 
    }

    void LivenessAnalysisBehavior::act(Instruction_call& i) {
        auto &ls = livenessData.back()[cur_i];
        std::unordered_set<string> caller_save_registers = {"r10", "r11", "r8", "r9", "rax", "rcx", "rdi", "rdx", "rsi"}; 
        ls.kill.insert(caller_save_registers.begin(), caller_save_registers.end());

        std::vector<string> argument_registers = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        if (i.callType() == CallType::l1) {
            const Item* callee = i.callee();
            if (isLivenessContributor(callee)) {
                EmitOptions options; 
                options.livenessAnalysis = true; 
                ls.gen.insert(callee->emit(options));
            }
        }

        int64_t num_args = i.nArgs()->value(); 
        for (int argIndex = 0; argIndex < std::min(num_args, static_cast<int64_t>(6)); argIndex++) {
            ls.gen.insert(argument_registers[argIndex]);
        }
        print_instruction_gen_kill(cur_i, ls);
    }

    void LivenessAnalysisBehavior::act(Instruction_reg_inc_dec& i) {
        auto &ls = livenessData.back()[cur_i];
        const Item* dst = i.dst(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        ls.gen.insert(dst->emit(options)); 
        ls.kill.insert(dst->emit(options)); 
        print_instruction_gen_kill(cur_i, ls); 
    }

    void LivenessAnalysisBehavior::act(Instruction_lea& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* dst = i.dst();
        const Item* lhs = i.lhs();
        const Item* rhs = i.rhs();
        EmitOptions options; 
        options.livenessAnalysis = true; 
        ls.gen.insert(lhs->emit(options)); 
        ls.gen.insert(rhs->emit(options)); 
        ls.kill.insert(dst->emit(options));
        print_instruction_gen_kill(cur_i, ls); 
    }

    void analyze_liveness(Program p) {
        LivenessAnalysisBehavior b;
        p.accept(b); 
    }
}
