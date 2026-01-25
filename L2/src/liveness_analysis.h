#pragma once

#include <algorithm> 
#include <iterator> 
#include <unordered_map> 
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
      explicit LivenessAnalysisBehavior(std::ostream &out);
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

      void print_instruction_gen_kill(size_t cur_i, const livenessSets& ls);
      void print_in_out_sets();
      void print_paren_set(const std::unordered_set<std::string>& s);
      void print_liveness_tests();
      void print_interference_tests();

      bool isLivenessContributor(const Item* var); 
      bool isNoSuccessorInstruction(const Instruction* i);

      void generate_in_out_sets(const Program &p); 
      void generate_interference_graph(const Program &p); 

      std::string pick_low_node(size_t functionIndex); 
      std::string pick_high_node(size_t functionIndex); 
      void update_graph(const std::string &selected, size_t functionIndex); 
      void select_nodes(); 
      void color_graph(); 

 
    private: 
      std::vector<std::vector<livenessSets>> livenessData; 
      std::vector<std::unordered_map<std::string, size_t>> labelMap; 
      std::vector<std::unordered_map<std::string, std::unordered_set<std::string>>> interferenceGraph; 
      size_t cur_i = 0; 

      std::vector<std::unordered_map<std::string, size_t>> nodeDegrees; 
      std::vector<std::unordered_set<std::string>> removed_nodes; 
      std::vector<std::vector<std::string>> node_stack; 

      std::ostream &out; 
  }; 


    void analyze_liveness(Program p); 

}
