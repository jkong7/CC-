#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "L3.h"
#include "tree.h"

namespace L3 {

  class Emitter {
  public:
    explicit Emitter(std::ostream& out);

    void line(const std::string& s);
    std::string fresh_tmp(); 

  private:
    std::ostream& out_;
    int64_t tmp_next_ = 0; 
  };

  struct Match {
    const Tree* node = nullptr;

    const Tree* dst = nullptr;
    const Tree* lhs = nullptr;
    const Tree* rhs = nullptr;

    std::optional<OP>  op;
    std::optional<CMP> cmp;
  };

  struct GlobalLabel {
    void enter_function(const std::string& fn);
    std::string make_label(const std::string& l3_label); 
    std::string make_fresh_label(); 

    std::string prefix = ":global_";
    int64_t next = 0;
    std::string cur_fn;
    std::unordered_map<std::string, int64_t> labelMap;
};



  class TilingEngine {
  public:
    explicit TilingEngine(std::ostream& out, GlobalLabel& labeler, bool shuffle_callee_saves = false);
    void tile(Program& p);

  private:

    void tile_function(Function& f);

    void codegen(const Node& item);
    void initialize_function_args(const std::vector<Variable*> var_arguments);
    template <class CallT>
    void handle_call(const CallT* call);

    void tile_tree(const Tree& t);


    std::string lower_expr(const Tree* t);
    bool lower_condition(const Tree* t, std::string& condition);
    std::string lower_address(const Tree* t);
    void lower_assign(const std::string& dst, const Tree* rhs);

    Emitter emitter_;
    GlobalLabel labeler_; 
    bool shuffle_callee_saves_ = false;
    bool saving_ = false;
  };

  void tile_program(Program& p, std::ostream& out, int32_t optLevel = 0);

} 
