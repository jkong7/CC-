#pragma once

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <fstream>
#include <LA.h>
#include <behavior.h>


namespace LA{

  class CodeGenBehavior : public Behavior {
  public:
    CodeGenBehavior(std::ofstream &o, const Program &p);

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
    void act(Instruction_break_uncond& i) override;
    void act(Instruction_break_cond& i) override;

    void act(Instruction_return& i) override;
    void act(Instruction_return_t& i) override;

  private:
    std::string temp();
    std::string fresh_label();
    std::string var(const Name* n) const;
    std::string value(const Item* item, bool allow_function = false);
    std::string decoded(const Item* item);
    std::string callee(CallType c, const Item* item);
    std::string args(const std::vector<Item*> &items);
    VarType type_of(const Name* n) const;

    void line(const std::string &s);
    void cold_block(const std::string &label, const std::string &call);
    void check_allocated(const Name* base, int64_t source_line);
    void check_bounds(const Name* base, const std::vector<Item*> &indexes, int64_t source_line);
    std::vector<std::string> indexes(const std::vector<Item*> &items);
    std::vector<std::string> form_basic_blocks(const std::vector<std::string> &lines);

    const Program &program;
    Function* cur_function = nullptr;
    std::string prefix;
    std::vector<std::string> body;
    std::vector<std::string> cold;
    std::vector<std::string> temps;
    std::string last_compare_dst;
    std::string last_compare_raw;
    int temp_counter = 0;
    int label_counter = 0;
    std::ofstream &out;
  };

  void generate_code(Program& p);

}
