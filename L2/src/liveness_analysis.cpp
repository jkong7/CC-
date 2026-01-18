#include <string>
#include <iostream>
#include <fstream>

#include <liveness_analysis.h>
#include <helper.h> 

using namespace std;

namespace L2{

    bool LivenessAnalysisBehavior::isLivenessContributor(const Item* var) {
        return var->kind() == ItemType::RegisterItem || var->kind() == ItemType::VariableItem;
    }

    void LivenessAnalysisBehavior::act(Program& p) {
        for (Function *f: p.functions) {
            f->accept(*this); 
        }
    }

    void LivenessAnalysisBehavior::act(Function& f) {
        livenessData.emplace_back(f.instructions.size()); 
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
            std::cout << cur_i << ": " << "gen set: " << src->emit(options) << std::endl; 
        }
        if (isLivenessContributor(dst)) {
            ls.kill.insert(dst->emit(options)); 
            std::cout << cur_i << ": " << "kill set: " << dst->emit(options) << std::endl; 
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_stack_arg_assignment& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_aop& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_sop& i) {
        
    }
    
    void LivenessAnalysisBehavior::act(Instruction_mem_aop& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_cmp_assignment& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_cjump& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_label& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_goto& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_ret& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_call& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_reg_inc_dec& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_lea& i) {
        
    }

    void analyze_liveness(Program p) {
        LivenessAnalysisBehavior b;
        p.accept(b); 
    }
}
