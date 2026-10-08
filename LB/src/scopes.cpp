#include <stdexcept>

#include <scopes.h>

namespace LB {

  ScopeBehavior::ScopeBehavior(const std::string &prefix) 
    : prefix (prefix) {
      return;
    }

  void ScopeBehavior::act(Program& p) {
    for (auto* f : p.functions) {
      f->accept(*this);
    }
  }

  void ScopeBehavior::act(Function& f) {
    cur_function = &f;
    scopes.clear();
    used.clear();
    loop_bodies.clear();
    loops.clear();
    f.instructions.clear();
    f.variable_types.clear();

    collect_loops(f.body);

    scopes.emplace_back();
    for (size_t k = 0; k < f.params.size(); k++) {
      declare(f.params[k], f.param_types[k]);
    }
    f.body->accept(*this);
    scopes.pop_back();

    if (!loops.empty()) {
      throw std::runtime_error("loop " + loops.back()->label1_->label_ + " in function " + f.name + " never reaches its exit label");
    }
    cur_function = nullptr;
  }

  void ScopeBehavior::collect_loops(Instruction_scope* scope) {
    for (auto* i : scope->instructions_) {
      if (auto* w = dynamic_cast<Instruction_while*>(i)) {
        w->cond_label_ = new Label(":" + prefix + "w" + std::to_string(label_counter++));
        loop_bodies[w->label1_->label_] = w;
      } else if (auto* s = dynamic_cast<Instruction_scope*>(i)) {
        collect_loops(s);
      }
    }
  }

  void ScopeBehavior::declare(Name* n, VarType type) {
    std::string original = n->name_;
    std::string unique = original;
    if (used.count(unique)) {
      unique = prefix + "s" + std::to_string(rename_counter++) + "_" + original;
    }
    used.insert(unique);
    scopes.back()[original] = unique;
    cur_function->variable_types[unique] = type;
    n->name_ = unique;
  }

  void ScopeBehavior::rename(Item* item) {
    auto* n = dynamic_cast<Name*>(item);
    if (!n) return;
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(n->name_);
      if (found != it->end()) {
        n->name_ = found->second;
        return;
      }
    }
  }

  void ScopeBehavior::rename_all(const std::vector<Item*> &items) {
    for (auto* item : items) rename(item);
  }

  void ScopeBehavior::emit(Instruction* i) {
    cur_function->instructions.push_back(i);
  }

  void ScopeBehavior::act(Instruction_scope& i) {
    scopes.emplace_back();
    for (auto* inner : i.instructions_) {
      inner->accept(*this);
    }
    scopes.pop_back();
  }

  void ScopeBehavior::act(Instruction_declare& i) {
    for (auto* v : i.vars_) declare(v, i.type_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_assignment& i) {
    rename(i.src_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_op& i) {
    rename(i.lhs_);
    rename(i.rhs_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_index_load& i) {
    rename(i.src_);
    rename_all(i.indexes_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_index_store& i) {
    rename(i.src_);
    rename_all(i.indexes_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_length& i) {
    rename(i.src_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_length_t& i) {
    rename(i.src_);
    rename(i.t_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_call& i) {
    if (i.callee_) rename(i.callee_);
    rename_all(i.args_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_call_assignment& i) {
    if (i.callee_) rename(i.callee_);
    rename_all(i.args_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_new_array& i) {
    rename_all(i.args_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_new_tuple& i) {
    rename(i.t_);
    rename(i.dst_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_label& i) {
    const std::string &label = i.label_->label_;
    if (!loops.empty() && loops.back()->label2_->label_ == label) {
      loops.pop_back();
    }
    auto it = loop_bodies.find(label);
    if (it != loop_bodies.end()) {
      loops.push_back(it->second);
    }
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_goto& i) {
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_if& i) {
    rename(i.lhs_);
    rename(i.rhs_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_while& i) {
    rename(i.lhs_);
    rename(i.rhs_);
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_continue& i) {
    if (loops.empty()) {
      throw std::runtime_error("continue outside of a loop at line " + std::to_string(i.line_));
    }
    i.loop_ = loops.back();
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_break& i) {
    if (loops.empty()) {
      throw std::runtime_error("break outside of a loop at line " + std::to_string(i.line_));
    }
    i.loop_ = loops.back();
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_return& i) {
    emit(&i);
  }

  void ScopeBehavior::act(Instruction_return_t& i) {
    rename(i.t_);
    emit(&i);
  }

  void resolve_scopes(Program& p, const std::string &prefix) {
    ScopeBehavior b(prefix);
    p.accept(b);
  }

}
