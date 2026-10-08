#include <inline.h>


namespace IR {

  static const size_t INLINE_LIMIT = 24;

  struct Renamer {
    std::string prefix;

    Variable* var(const Variable* v) const {
      return new Variable("%" + prefix + v->var_.substr(1));
    }

    Label* label(const Label* l) const {
      return new Label(":" + prefix + l->label_.substr(1));
    }

    Item* item(Item* i) const {
      if (auto* v = dynamic_cast<Variable*>(i)) return var(v);
      if (auto* l = dynamic_cast<Label*>(i)) return label(l);
      return i;
    }

    std::vector<Item*> items(const std::vector<Item*> &list) const {
      std::vector<Item*> out;
      for (auto* i : list) out.push_back(item(i));
      return out;
    }
  };

  static size_t size_of(const Function* f) {
    size_t n = 0;
    for (auto* bb : f->basic_blocks) n += bb->instructions.size();
    return n;
  }

  static Function* direct_callee(Instruction* i, const Program& p) {
    Item* callee = nullptr;
    if (auto* c = dynamic_cast<Instruction_call*>(i)) {
      if (c->c_ == CallType::ir) callee = c->callee_;
    } else if (auto* c = dynamic_cast<Instruction_call_assignment*>(i)) {
      if (c->c_ == CallType::ir) callee = c->callee_;
    }
    auto* f = dynamic_cast<Func*>(callee);
    if (!f) return nullptr;
    for (auto* candidate : p.functions) {
      if (candidate->name == f->function_label_) return candidate;
    }
    return nullptr;
  }

  static bool calls_functions(const Function* f) {
    for (auto* bb : f->basic_blocks) {
      for (auto* i : bb->instructions) {
        auto* c = dynamic_cast<Instruction_call*>(i);
        auto* ca = dynamic_cast<Instruction_call_assignment*>(i);
        if ((c && c->c_ == CallType::ir) || (ca && ca->c_ == CallType::ir)) return true;
      }
    }
    return false;
  }

  static Instruction* clone(Instruction* i, const Renamer& r) {
    if (auto* x = dynamic_cast<Instruction_assignment*>(i))
      return new Instruction_assignment(r.var(x->dst_), r.item(x->src_));
    if (auto* x = dynamic_cast<Instruction_op*>(i))
      return new Instruction_op(r.var(x->dst_), r.item(x->lhs_), x->op_, r.item(x->rhs_));
    if (auto* x = dynamic_cast<Instruction_index_load*>(i))
      return new Instruction_index_load(r.var(x->dst_), r.var(x->src_), r.items(x->indexes_));
    if (auto* x = dynamic_cast<Instruction_index_store*>(i))
      return new Instruction_index_store(r.var(x->dst_), r.items(x->indexes_), r.item(x->src_));
    if (auto* x = dynamic_cast<Instruction_length_t*>(i))
      return new Instruction_length_t(r.var(x->dst_), r.var(x->src_), r.item(x->t_));
    if (auto* x = dynamic_cast<Instruction_length*>(i))
      return new Instruction_length(r.var(x->dst_), r.var(x->src_));
    if (auto* x = dynamic_cast<Instruction_call*>(i))
      return new Instruction_call(x->c_, x->callee_ ? r.item(x->callee_) : nullptr, r.items(x->args_));
    if (auto* x = dynamic_cast<Instruction_call_assignment*>(i))
      return new Instruction_call_assignment(r.var(x->dst_), x->c_, x->callee_ ? r.item(x->callee_) : nullptr, r.items(x->args_));
    if (auto* x = dynamic_cast<Instruction_new_array*>(i))
      return new Instruction_new_array(r.var(x->dst_), r.items(x->args_));
    if (auto* x = dynamic_cast<Instruction_new_tuple*>(i))
      return new Instruction_new_tuple(r.var(x->dst_), r.item(x->t_));
    if (auto* x = dynamic_cast<Instruction_break_uncond*>(i))
      return new Instruction_break_uncond(r.label(x->label_));
    if (auto* x = dynamic_cast<Instruction_break_cond*>(i))
      return new Instruction_break_cond(r.item(x->t_), r.label(x->label1_), r.label(x->label2_));
    return i;
  }

  static std::string fresh_prefix(const Function* caller, int64_t &counter) {
    while (true) {
      std::string prefix = "__inl" + std::to_string(counter++) + "_";
      bool clash = false;
      for (auto &[name, type] : caller->variable_types) {
        if (name.rfind("%" + prefix, 0) == 0) clash = true;
      }
      for (auto* bb : caller->basic_blocks) {
        if (bb->label_->label_.rfind(":" + prefix, 0) == 0) clash = true;
      }
      if (!clash) return prefix;
    }
  }

  static bool inline_one(Function* caller, const Program& p, int64_t &counter) {
    for (size_t b = 0; b < caller->basic_blocks.size(); b++) {
      BasicBlock* bb = caller->basic_blocks[b];
      for (size_t k = 0; k < bb->instructions.size(); k++) {
        Instruction* call = bb->instructions[k];
        Function* callee = direct_callee(call, p);
        if (!callee || callee == caller || callee->name == "@main") continue;
        if (size_of(callee) > INLINE_LIMIT || calls_functions(callee)) continue;

        Renamer r{fresh_prefix(caller, counter)};
        std::vector<Item*> args;
        Variable* dst = nullptr;
        if (auto* c = dynamic_cast<Instruction_call*>(call)) {
          args = c->args_;
        } else {
          auto* ca = static_cast<Instruction_call_assignment*>(call);
          args = ca->args_;
          dst = ca->dst_;
        }

        for (auto &[name, type] : callee->variable_types) {
          caller->variable_types["%" + r.prefix + name.substr(1)] = type;
        }

        auto* rest = new BasicBlock();
        rest->label_ = new Label(":" + r.prefix + "return");
        rest->instructions.assign(bb->instructions.begin() + k + 1, bb->instructions.end());

        bb->instructions.resize(k);
        for (size_t a = 0; a < callee->var_arguments.size() && a < args.size(); a++) {
          bb->instructions.push_back(new Instruction_assignment(r.var(callee->var_arguments[a]), args[a]));
        }
        bb->instructions.push_back(new Instruction_break_uncond(r.label(callee->basic_blocks[0]->label_)));

        std::vector<BasicBlock*> body;
        for (auto* cb : callee->basic_blocks) {
          auto* copy = new BasicBlock();
          copy->label_ = r.label(cb->label_);
          for (auto* i : cb->instructions) {
            if (auto* ret = dynamic_cast<Instruction_return_t*>(i)) {
              if (dst) copy->instructions.push_back(new Instruction_assignment(dst, r.item(ret->t_)));
              copy->instructions.push_back(new Instruction_break_uncond(rest->label_));
            } else if (dynamic_cast<Instruction_return*>(i)) {
              copy->instructions.push_back(new Instruction_break_uncond(rest->label_));
            } else {
              copy->instructions.push_back(clone(i, r));
            }
          }
          body.push_back(copy);
        }
        body.push_back(rest);
        caller->basic_blocks.insert(caller->basic_blocks.begin() + b + 1, body.begin(), body.end());
        return true;
      }
    }
    return false;
  }

  void inline_functions(Program& p) {
    int64_t counter = 0;
    for (auto* caller : p.functions) {
      size_t budget = 64;
      while (budget-- > 0 && inline_one(caller, p, counter)) {}
    }
  }

}
