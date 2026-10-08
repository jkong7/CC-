#include <stdexcept>

#include <codegen.h>
#include <helper.h>

namespace LA {

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

  static size_t leading_underscores(const std::string &s) {
    size_t n = 0;
    while (n < s.size() && s[n] == '_') n++;
    return n;
  }

  static std::string compute_prefix(const Program &p) {
    size_t longest = 0;
    for (auto* f : p.functions) {
      longest = std::max(longest, leading_underscores(f->name));
      for (auto &[name, type] : f->variable_types) {
        longest = std::max(longest, leading_underscores(name));
      }
      for (auto* i : f->instructions) {
        if (auto* l = dynamic_cast<Instruction_label*>(i)) {
          longest = std::max(longest, leading_underscores(l->label_->label_.substr(1)));
        }
      }
    }
    return std::string(longest + 1, '_');
  }

  static bool is_label(const std::string &s) {
    return !s.empty() && s[0] == ':';
  }

  static bool is_terminator(const std::string &s) {
    return s.rfind("br ", 0) == 0 || s == "return" || s.rfind("return ", 0) == 0;
  }

  CodeGenBehavior::CodeGenBehavior(std::ofstream &o, const Program &p)
    : program (p),
      prefix (compute_prefix(p)),
      out (o) {
      return;
    }

  void CodeGenBehavior::act(Program& p) {
    for (auto* f : p.functions) {
      f->accept(*this);
    }
  }

  void CodeGenBehavior::act(Function& f) {
    cur_function = &f;
    body.clear();
    cold.clear();
    temps.clear();

    out << "define " << type_to_str(f.return_type) << " @" << f.name << " (";
    for (size_t k = 0; k < f.params.size(); k++) {
      if (k) out << ", ";
      out << type_to_str(type_of(f.params[k])) << " " << var(f.params[k]);
    }
    out << ") {\n";

    for (auto* i : f.instructions) {
      i->accept(*this);
    }

    std::vector<std::string> lines = body;
    if (lines.empty() || !is_terminator(lines.back())) {
      lines.push_back("return");
    }
    lines.insert(lines.end(), cold.begin(), cold.end());
    lines = form_basic_blocks(lines);

    out << lines[0] << "\n";
    for (auto &t : temps) {
      out << "  int64 " << t << "\n";
    }
    for (size_t k = 1; k < lines.size(); k++) {
      out << (is_label(lines[k]) ? "" : "  ") << lines[k] << "\n";
    }
    out << "}\n\n";

    cur_function = nullptr;
  }

  std::vector<std::string> CodeGenBehavior::form_basic_blocks(const std::vector<std::string> &lines) {
    std::vector<std::string> blocks;
    bool start_block = true;
    for (auto &l : lines) {
      if (start_block) {
        if (!is_label(l)) {
          blocks.push_back(fresh_label());
        }
        start_block = false;
      } else if (is_label(l)) {
        blocks.push_back("br " + l);
      }
      blocks.push_back(l);
      if (is_terminator(l)) {
        start_block = true;
      }
    }
    return blocks;
  }

  void CodeGenBehavior::act(Instruction_declare& i) {
    line(type_to_str(i.type_) + " " + var(i.var_));
    bool number = i.type_.type == Type::int64 && i.type_.dims == 0;
    line(var(i.var_) + " <- " + (number ? "1" : "0"));
  }

  void CodeGenBehavior::act(Instruction_assignment& i) {
    line(var(i.dst_) + " <- " + value(i.src_, true));
  }

  void CodeGenBehavior::act(Instruction_op& i) {
    std::string dst = var(i.dst_);
    auto* lnum = dynamic_cast<Number*>(i.lhs_);
    auto* rnum = dynamic_cast<Number*>(i.rhs_);

    if (lnum && rnum) {
      line(dst + " <- " + std::to_string(encode(fold(i.op_, lnum->number_, rnum->number_))));
      return;
    }

    std::string a = value(i.lhs_);
    std::string b = value(i.rhs_);
    std::string op = op_to_str(i.op_);

    switch (i.op_) {
      case OP::plus:
        line(dst + " <- " + a + " + " + b);
        line(dst + " <- " + dst + " - 1");
        return;

      case OP::minus:
        line(dst + " <- " + a + " - " + b);
        line(dst + " <- " + dst + " + 1");
        return;

      case OP::at:
        line(dst + " <- " + a + " & " + b);
        return;

      case OP::times: {
        Item* var_side = lnum ? i.rhs_ : i.lhs_;
        Item* other = lnum ? i.lhs_ : i.rhs_;
        std::string x = decoded(var_side);
        std::string y;
        if (auto* n = dynamic_cast<Number*>(other)) {
          y = std::to_string(static_cast<int64_t>(static_cast<uint64_t>(n->number_) << 1));
        } else {
          y = temp();
          line(y + " <- " + value(other) + " - 1");
        }
        line(dst + " <- " + x + " * " + y);
        line(dst + " <- " + dst + " + 1");
        return;
      }

      case OP::left_shift:
      case OP::right_shift: {
        std::string x = decoded(i.lhs_);
        std::string y = decoded(i.rhs_);
        line(dst + " <- " + x + " " + op + " " + y);
        line(dst + " <- " + dst + " << 1");
        line(dst + " <- " + dst + " + 1");
        return;
      }

      default:
        line(dst + " <- " + a + " " + op + " " + b);
        line(dst + " <- " + dst + " << 1");
        line(dst + " <- " + dst + " + 1");
        return;
    }
  }

  void CodeGenBehavior::act(Instruction_index_load& i) {
    check_allocated(i.src_, i.line_);
    if (type_of(i.src_).type != Type::tuple) {
      check_bounds(i.src_, i.indexes_, i.line_);
    }
    std::string access = var(i.src_);
    for (auto &idx : indexes(i.indexes_)) access += "[" + idx + "]";
    line(var(i.dst_) + " <- " + access);
  }

  void CodeGenBehavior::act(Instruction_index_store& i) {
    check_allocated(i.dst_, i.line_);
    if (type_of(i.dst_).type != Type::tuple) {
      check_bounds(i.dst_, i.indexes_, i.line_);
    }
    std::string access = var(i.dst_);
    for (auto &idx : indexes(i.indexes_)) access += "[" + idx + "]";
    line(access + " <- " + value(i.src_));
  }

  void CodeGenBehavior::act(Instruction_length& i) {
    check_allocated(i.src_, i.line_);
    line(var(i.dst_) + " <- length " + var(i.src_));
  }

  void CodeGenBehavior::act(Instruction_length_t& i) {
    check_allocated(i.src_, i.line_);
    line(var(i.dst_) + " <- length " + var(i.src_) + " " + decoded(i.t_));
  }

  void CodeGenBehavior::act(Instruction_call& i) {
    line("call " + callee(i.c_, i.callee_) + "(" + args(i.args_) + ")");
  }

  void CodeGenBehavior::act(Instruction_call_assignment& i) {
    line(var(i.dst_) + " <- call " + callee(i.c_, i.callee_) + "(" + args(i.args_) + ")");
  }

  void CodeGenBehavior::act(Instruction_new_array& i) {
    line(var(i.dst_) + " <- new Array(" + args(i.args_) + ")");
  }

  void CodeGenBehavior::act(Instruction_new_tuple& i) {
    line(var(i.dst_) + " <- new Tuple(" + value(i.t_) + ")");
  }

  void CodeGenBehavior::act(Instruction_label& i) {
    line(i.label_->emit());
  }

  void CodeGenBehavior::act(Instruction_break_uncond& i) {
    line("br " + i.label_->emit());
  }

  void CodeGenBehavior::act(Instruction_break_cond& i) {
    if (auto* n = dynamic_cast<Number*>(i.t_)) {
      line("br " + (n->number_ != 0 ? i.label1_->emit() : i.label2_->emit()));
      return;
    }
    std::string zero = temp();
    line(zero + " <- " + value(i.t_) + " = 1");
    line("br " + zero + " " + i.label2_->emit() + " " + i.label1_->emit());
  }

  void CodeGenBehavior::act(Instruction_return& i) {
    line("return");
  }

  void CodeGenBehavior::act(Instruction_return_t& i) {
    line("return " + value(i.t_));
  }

  void CodeGenBehavior::check_allocated(const Name* base, int64_t source_line) {
    std::string is_null = temp();
    std::string ok = fresh_label();
    std::string err = fresh_label();
    line(is_null + " <- " + var(base) + " = 0");
    line("br " + is_null + " " + err + " " + ok);
    line(ok);
    cold_block(err, "call tensor-error(" + std::to_string(encode(source_line)) + ")");
  }

  void CodeGenBehavior::check_bounds(const Name* base, const std::vector<Item*> &idxs, int64_t source_line) {
    for (size_t d = 0; d < idxs.size(); d++) {
      std::string len = temp();
      std::string in_range = temp();
      std::string index = value(idxs[d]);
      std::string ok = fresh_label();
      std::string err = fresh_label();

      line(len + " <- length " + var(base) + " " + std::to_string(d));
      line(in_range + " <- " + index + " < " + len);
      auto* n = dynamic_cast<Number*>(idxs[d]);
      if (!n || n->number_ < 0) {
        std::string non_negative = temp();
        line(non_negative + " <- " + index + " >= 1");
        line(in_range + " <- " + in_range + " & " + non_negative);
      }
      line("br " + in_range + " " + ok + " " + err);
      line(ok);

      std::string error_args = std::to_string(encode(source_line));
      if (idxs.size() > 1) {
        error_args += ", " + std::to_string(encode(static_cast<int64_t>(d)));
      }
      error_args += ", " + len + ", " + index;
      cold_block(err, "call tensor-error(" + error_args + ")");
    }
  }

  std::vector<std::string> CodeGenBehavior::indexes(const std::vector<Item*> &items) {
    std::vector<std::string> result;
    for (auto* item : items) result.push_back(decoded(item));
    return result;
  }

  void CodeGenBehavior::cold_block(const std::string &label, const std::string &call) {
    cold.push_back(label);
    cold.push_back(call);
    cold.push_back("return");
  }

  void CodeGenBehavior::line(const std::string &s) {
    body.push_back(s);
  }

  VarType CodeGenBehavior::type_of(const Name* n) const {
    auto it = cur_function->variable_types.find(n->name_);
    if (it == cur_function->variable_types.end()) {
      throw std::runtime_error("undeclared variable " + n->name_ + " in function " + cur_function->name);
    }
    return it->second;
  }

  std::string CodeGenBehavior::var(const Name* n) const {
    type_of(n);
    return "%" + n->name_;
  }

  std::string CodeGenBehavior::value(const Item* item, bool allow_function) {
    if (auto* n = dynamic_cast<const Number*>(item)) {
      return std::to_string(encode(n->number_));
    }
    auto* name = static_cast<const Name*>(item);
    if (cur_function->variable_types.count(name->name_)) {
      return "%" + name->name_;
    }
    if (program.has_function(name->name_)) {
      if (allow_function) {
        return "@" + name->name_;
      }
      std::string t = temp();
      line(t + " <- @" + name->name_);
      return t;
    }
    throw std::runtime_error("unknown name " + name->name_ + " in function " + cur_function->name);
  }

  std::string CodeGenBehavior::decoded(const Item* item) {
    if (auto* n = dynamic_cast<const Number*>(item)) {
      return std::to_string(n->number_);
    }
    std::string t = temp();
    line(t + " <- " + value(item) + " >> 1");
    return t;
  }

  std::string CodeGenBehavior::callee(CallType c, const Item* item) {
    switch (c) {
      case CallType::print:        return "print";
      case CallType::input:        return "input";
      case CallType::tuple_error:  return "tuple-error";
      case CallType::tensor_error: return "tensor-error";
      case CallType::la:
      default:
        return value(item, true);
    }
  }

  std::string CodeGenBehavior::args(const std::vector<Item*> &items) {
    std::string s;
    for (size_t k = 0; k < items.size(); k++) {
      if (k) s += ", ";
      s += value(items[k]);
    }
    return s;
  }

  std::string CodeGenBehavior::temp() {
    std::string t = "%" + prefix + "t" + std::to_string(temp_counter++);
    temps.push_back(t);
    return t;
  }

  std::string CodeGenBehavior::fresh_label() {
    return ":" + prefix + "l" + std::to_string(label_counter++);
  }

  void generate_code(Program& p) {
    std::ofstream outputFile("prog.IR");
    CodeGenBehavior b(outputFile, p);
    p.accept(b);
  }

}
