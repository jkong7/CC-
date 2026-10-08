#pragma once 

#include <unordered_map> 
#include <unordered_set> 
#include <vector> 
#include <string> 
#include <LB.h>
#include <behavior.h> 


namespace LB{

  class ScopeBehavior : public Behavior {
  public:
    ScopeBehavior(const std::string &prefix); 

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
    void declare(Name* n, VarType type); 
    void rename(Item* item); 
    void rename_all(const std::vector<Item*> &items); 
    void collect_loops(Instruction_scope* scope); 
    void emit(Instruction* i); 

    std::string prefix; 
    Function* cur_function = nullptr;
    std::vector<std::unordered_map<std::string, std::string>> scopes; 
    std::unordered_set<std::string> used; 
    std::unordered_map<std::string, Instruction_while*> loop_bodies; 
    std::vector<Instruction_while*> loops; 
    int rename_counter = 0; 
    int label_counter = 0; 
  };

  void resolve_scopes(Program& p, const std::string &prefix);

}
