#pragma once 

#include <algorithm> 
#include <iterator> 
#include <unordered_map> 
#include <unordered_set> 
#include <vector> 
#include <sstream> 
#include <tuple> 
#include <fstream>
#include <IR.h>
#include <behavior.h> 


namespace IR{

  class CodeGenBehavior : public Behavior {
  public:
    CodeGenBehavior(std::ofstream &o); 

    void act(Program& p) override;
    void act(Function& f) override;

    void act(Instruction_initialize& i) override;
    void act(Instruction_assignment& i) override;
    void act(Instruction_op& i) override;

    void act(Instruction_index_load& i) override;
    void act(Instruction_index_store& i) override;

    void act(Instruction_length& i) override;
    void act(Instruction_length_t& i) override;

    void act(Instruction_call& i) override;
    void act(Instruction_call_assignment& i) override;

    void act(Instruction_new_array& i) override;
    void act(Instruction_new_tuple& i) override;

    void act(Instruction_break_uncond& i) override;
    void act(Instruction_break_cond& i) override;

    void act(Instruction_return& i) override;
    void act(Instruction_return_t& i) override;

  private: 
    std::string temp();
    std::string element_address(const std::string& base, const std::vector<Item*>& indexes, bool tuple);

    Function* cur_function = nullptr;
    BasicBlock* cur_bb = nullptr;
    BasicBlock* next_bb = nullptr;
    int temp_counter = 0; 
    std::ofstream &out; 
  };

  void generate_code(Program& p);

}