#pragma once

#include <algorithm> 
#include <iterator> 
#include <unordered_map> 
#include <unordered_set> 
#include <vector> 
#include <behavior.h>
#include <spill.h> 
#include <helper.h> 
#include <L2.h>

namespace L2{

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

      void initialize_containers(size_t n); 
      void clear_function_containers();

      bool isLivenessContributor(const Item* var); 
      bool isNoSuccessorInstruction(const Instruction* i);

      void generate_in_out_sets(const Program &p); 
      void generate_interference_graph(const Program &p); 

      std::string pick_low_node(); 
      std::string pick_high_node(); 
      void update_graph(const std::string &selected); 
      void select_nodes(); 

      bool color_or_spill_node(const std::string &cur_node, const std::unordered_set<std::string> &neighbors); 
      bool color_graph(); 

 
    private: 
      size_t cur_f = 0; 
      size_t cur_i = 0; 

      std::vector<std::vector<livenessSets>> livenessData; 
      std::vector<std::unordered_map<std::string, size_t>> labelMap; 
      std::vector<std::unordered_map<std::string, std::unordered_set<std::string>>> interferenceGraph; 

      std::vector<std::unordered_map<std::string, size_t>> nodeDegrees; 
      std::vector<std::unordered_set<std::string>> removed_nodes; 
      std::vector<std::vector<std::string>> node_stack; 

      std::vector<std::unordered_set<std::string>> spillOutputs; 
      std::vector<std::unordered_map<std::string, std::string>> colorOutputs; 

      std::ostream &out; 
  }; 


    void analyze_liveness(Program p); 

}
