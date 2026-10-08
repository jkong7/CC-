#include <codegen.h>
#include <helper.h>
#include <scopes.h>

namespace LB {

  static std::string type_to_str(VarType t) {
    switch (t.type) {
      case Type::void_: return "void";
      case Type::tuple: return "tuple";
      case Type::code:  return "code";
      case Type::int64: {
        std::string s = "int64";
        for (int64_t d = 0; d < t.dims; d++) s += "[]";
        return s;
      }
    }
    return "<?>";
  }

  static std::string callee_to_str(CallType c, const Item* callee) {
    switch (c) {
      case CallType::print:        return "print";
      case CallType::input:        return "input";
      case CallType::tuple_error:  return "tuple-error";
      case CallType::tensor_error: return "tensor-error";
      case CallType::lb:
      default:
        return callee->emit();
    }
  }

  static std::string join(const std::vector<Item*> &items) {
    std::string s;
    for (size_t k = 0; k < items.size(); k++) {
      if (k) s += ", ";
      s += items[k]->emit();
    }
    return s;
  }

  static std::string indexes(const std::vector<Item*> &items) {
    std::string s;
    for (auto* item : items) s += "[" + item->emit() + "]";
    return s;
  }

  static size_t leading_underscores(const std::string &s) {
    size_t n = 0;
    while (n < s.size() && s[n] == '_') n++;
    return n;
  }

  static void scan_names(const Instruction_scope* scope, size_t &longest);

  static void scan_item(const Item* item, size_t &longest) {
    if (auto* n = dynamic_cast<const Name*>(item)) {
      longest = std::max(longest, leading_underscores(n->name_));
    } else if (auto* l = dynamic_cast<const Label*>(item)) {
      longest = std::max(longest, leading_underscores(l->label_.substr(1)));
    }
  }

  static void scan_names(const Instruction_scope* scope, size_t &longest) {
    for (auto* i : scope->instructions_) {
      if (auto* s = dynamic_cast<const Instruction_scope*>(i)) {
        scan_names(s, longest);
      } else if (auto* d = dynamic_cast<const Instruction_declare*>(i)) {
        for (auto* v : d->vars_) scan_item(v, longest);
      } else if (auto* l = dynamic_cast<const Instruction_label*>(i)) {
        scan_item(l->label_, longest);
      }
    }
  }

  static std::string compute_prefix(const Program &p) {
    size_t longest = 0;
    for (auto* f : p.functions) {
      longest = std::max(longest, leading_underscores(f->name));
      for (auto* param : f->params) scan_item(param, longest);
      scan_names(f->body, longest);
    }
    return std::string(longest + 1, '_');
  }

  CodeGenBehavior::CodeGenBehavior(std::ofstream &o, const std::string &prefix) 
    : prefix (prefix),
      out (o) {
      return;
    }

  void CodeGenBehavior::act(Program& p) {
    for (auto* f : p.functions) {
      f->accept(*this);
    }
  }

  void CodeGenBehavior::act(Function& f) {
    body.clear();
    temps.clear();

    out << type_to_str(f.return_type) << " " << f.name << " (";
    for (size_t k = 0; k < f.params.size(); k++) {
      if (k) out << ", ";
      out << type_to_str(f.param_types[k]) << " " << f.params[k]->emit();
    }
    out << ") {\n";

    int64_t last_line = 0;
    for (auto* i : f.instructions) {
      if (i->line_ != last_line) {
        line("//#line " + std::to_string(i->line_));
        last_line = i->line_;
      }
      i->accept(*this);
    }

    for (auto &t : temps) {
      out << "  int64 " << t << "\n";
    }
    for (auto &l : body) {
      out << (l[0] == ':' || l[0] == '/' ? "" : "  ") << l << "\n";
    }
    out << "}\n\n";
  }

  void CodeGenBehavior::act(Instruction_declare& i) {
    for (auto* v : i.vars_) {
      line(type_to_str(i.type_) + " " + v->emit());
    }
  }

  void CodeGenBehavior::act(Instruction_assignment& i) {
    line(i.dst_->emit() + " <- " + i.src_->emit());
  }

  void CodeGenBehavior::act(Instruction_op& i) {
    line(i.dst_->emit() + " <- " + i.lhs_->emit() + " " + op_to_str(i.op_) + " " + i.rhs_->emit());
  }

  void CodeGenBehavior::act(Instruction_index_load& i) {
    line(i.dst_->emit() + " <- " + i.src_->emit() + indexes(i.indexes_));
  }

  void CodeGenBehavior::act(Instruction_index_store& i) {
    line(i.dst_->emit() + indexes(i.indexes_) + " <- " + i.src_->emit());
  }

  void CodeGenBehavior::act(Instruction_length& i) {
    line(i.dst_->emit() + " <- length " + i.src_->emit());
  }

  void CodeGenBehavior::act(Instruction_length_t& i) {
    line(i.dst_->emit() + " <- length " + i.src_->emit() + " " + i.t_->emit());
  }

  void CodeGenBehavior::act(Instruction_call& i) {
    line(callee_to_str(i.c_, i.callee_) + "(" + join(i.args_) + ")");
  }

  void CodeGenBehavior::act(Instruction_call_assignment& i) {
    line(i.dst_->emit() + " <- " + callee_to_str(i.c_, i.callee_) + "(" + join(i.args_) + ")");
  }

  void CodeGenBehavior::act(Instruction_new_array& i) {
    line(i.dst_->emit() + " <- new Array(" + join(i.args_) + ")");
  }

  void CodeGenBehavior::act(Instruction_new_tuple& i) {
    line(i.dst_->emit() + " <- new Tuple(" + i.t_->emit() + ")");
  }

  void CodeGenBehavior::act(Instruction_label& i) {
    line(i.label_->emit());
  }

  void CodeGenBehavior::act(Instruction_goto& i) {
    line("br " + i.label_->emit());
  }

  void CodeGenBehavior::act(Instruction_if& i) {
    branch(i.lhs_, i.op_, i.rhs_, i.label1_, i.label2_);
  }

  void CodeGenBehavior::act(Instruction_while& i) {
    line(i.cond_label_->emit());
    branch(i.lhs_, i.op_, i.rhs_, i.label1_, i.label2_);
  }

  void CodeGenBehavior::act(Instruction_continue& i) {
    line("br " + i.loop_->cond_label_->emit());
  }

  void CodeGenBehavior::act(Instruction_break& i) {
    line("br " + i.loop_->label2_->emit());
  }

  void CodeGenBehavior::act(Instruction_scope& i) {
    for (auto* inner : i.instructions_) {
      inner->accept(*this);
    }
  }

  void CodeGenBehavior::act(Instruction_return& i) {
    line("return");
  }

  void CodeGenBehavior::act(Instruction_return_t& i) {
    line("return " + i.t_->emit());
  }

  void CodeGenBehavior::branch(Item* lhs, OP op, Item* rhs, Label* l1, Label* l2) {
    std::string c = condition_var();
    line(c + " <- " + lhs->emit() + " " + op_to_str(op) + " " + rhs->emit());
    line("br " + c + " " + l1->emit() + " " + l2->emit());
  }

  std::string CodeGenBehavior::condition_var() {
    std::string t = prefix + "c" + std::to_string(temp_counter++);
    temps.push_back(t);
    return t;
  }

  void CodeGenBehavior::line(const std::string &s) {
    body.push_back(s);
  }

  void generate_code(Program& p) {
    std::string prefix = compute_prefix(p);
    resolve_scopes(p, prefix);

    std::ofstream outputFile("prog.a");
    CodeGenBehavior b(outputFile, prefix);
    p.accept(b);
  }

}
