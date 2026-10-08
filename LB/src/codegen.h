#pragma once 

#include <vector> 
#include <fstream>
#include <LB.h>
#include <behavior.h> 


namespace LB{

  class CodeGenBehavior : public Behavior {
  public:
    CodeGenBehavior(std::ofstream &o, const std::string &prefix); 

    void act(Program& p) override;
    void act(Function& f) override;

    void act(Instruction_declare& i) override;
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

    void act(Instruction_label& i) override;
    void act(Instruction_goto& i) override;
    void act(Instruction_if& i) override;
    void act(Instruction_while& i) override;
    void act(Instruction_continue& i) override;
    void act(Instruction_break& i) override;
    void act(Instruction_scope& i) override;

    void act(Instruction_return& i) override;
    void act(Instruction_return_t& i) override;

  private: 
    std::string condition_var(); 
    void branch(Item* lhs, OP op, Item* rhs, Label* l1, Label* l2); 
    void line(const std::string &s); 

    std::string prefix; 
    std::vector<std::string> body; 
    std::vector<std::string> temps; 
    int temp_counter = 0; 
    std::ofstream &out; 
  };

  void generate_code(Program& p);

}
