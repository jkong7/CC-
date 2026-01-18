#pragma once

#include <unordered_set> 
#include <vector> 
#include <L2.h>

namespace L2{

  class Behavior {
    public: 
      virtual ~Behavior() = default; 
      virtual void act(Program &p) = 0; 
      virtual void act(Function &f) = 0;
      virtual void act(Instruction_assignment &i) = 0;
      virtual void act(Instruction_stack_arg_assignment &i) = 0; 
      virtual void act(Instruction_aop &i) = 0;
      virtual void act(Instruction_sop &i) = 0;
      virtual void act(Instruction_mem_aop &i) = 0;
      virtual void act(Instruction_cmp_assignment &i) = 0;
      virtual void act(Instruction_cjump &i) = 0;
      virtual void act(Instruction_label &i) = 0;
      virtual void act(Instruction_goto &i) = 0;
      virtual void act(Instruction_ret &i) = 0;
      virtual void act(Instruction_call &i) = 0;
      virtual void act(Instruction_reg_inc_dec &i) = 0;
      virtual void act(Instruction_lea &i) = 0;
  };

  struct livenessSets {
    std::unordered_set<std::string> gen; 
    std::unordered_set<std::string> kill; 
    std::unordered_set<std::string> in; 
    std::unordered_set<std::string> out; 
  };

  class LivenessAnalysisBehavior : public Behavior {
    public: 
      void act(Program& p) override; 
      void act(Function &f) override; 
      void act(Instruction_assignment &i) override; 
      virtual void act(Instruction_stack_arg_assignment &i) override; 
      virtual void act(Instruction_aop &i) override; 
      virtual void act(Instruction_sop &i) override; 
      virtual void act(Instruction_mem_aop &i) override; 
      virtual void act(Instruction_cmp_assignment &i) override; 
      virtual void act(Instruction_cjump &i) override; 
      virtual void act(Instruction_label &i) override; 
      virtual void act(Instruction_goto &i) override; 
      virtual void act(Instruction_ret &i) override; 
      virtual void act(Instruction_call &i) override; 
      virtual void act(Instruction_reg_inc_dec &i) override; 
      virtual void act(Instruction_lea &i) override; 

      bool isLivenessContributor(const Item* var); 
    private: 
      std::vector<std::vector<livenessSets>> livenessData; 
      size_t cur_i = 0; 
  }; 

    void analyze_liveness(Program p); 


}
