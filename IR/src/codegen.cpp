#include <codegen.h>

namespace IR {

  static std::string emit_callee(IR::CallType c, IR::Item* callee) {
    switch (c) {
      case IR::print:        return "print";
      case IR::input:        return "input";
      case IR::tuple_error:  return "tuple-error";
      case IR::tensor_error: return "tensor-error";
      case IR::ir:
      default:
        return callee->emit();   
    }
  }

  static const char* op_to_str(IR::OP op) {
    switch (op) {
      case IR::plus:                 return "+";
      case IR::minus:                return "-";
      case IR::times:                return "*";
      case IR::at:                   return "&";
      case IR::left_shift:           return "<<";
      case IR::right_shift:          return ">>";
      case IR::less_than:            return "<";
      case IR::less_than_equal:      return "<=";
      case IR::equal:                return "=";
      case IR::greater_than_equal:   return ">=";
      case IR::greater_than:         return ">";
    }
    return "<?>";
  }

  static bool is_tuple_var(IR::Function* f, const std::string& v) {
    if (!f) return false;

    auto check = [&](const std::string& key) {
      auto it = f->variable_types.find(key);
      return it != f->variable_types.end() && it->second.first == IR::tuple;
    };

    if (check(v)) return true;
    if (!v.empty() && v[0] == '%' && check(v.substr(1))) return true;
    return false;
  }

  CodeGenBehavior::CodeGenBehavior(std::ofstream &o) 
    : out (o) {
      return;
    }

  void CodeGenBehavior::act(Program& p) {
    for (auto* f : p.functions) {
      f->accept(*this);
    }
  }

  void CodeGenBehavior::act(Function& f) {
    cur_function = &f;

    out << "define " << f.name << " (";
    for (size_t k = 0; k < f.var_arguments.size(); k++) {
      if (k) out << ", ";
      out << f.var_arguments[k]->emit();
    }
    out << ") {\n";

    for (size_t bi = 0; bi < f.basic_blocks.size(); bi++) {
      cur_bb  = f.basic_blocks[bi];
      next_bb = (bi + 1 < f.basic_blocks.size()) ? f.basic_blocks[bi + 1] : nullptr;
      out << cur_bb->label_->emit() << "\n";

      for (auto* inst : cur_bb->instructions) {
        inst->accept(*this);
      }
    }

    out << "}\n";

    cur_function = nullptr;
    cur_bb = nullptr;
    next_bb = nullptr;
  }

  void CodeGenBehavior::act(Instruction_initialize& i) {
    return;
  }

  void CodeGenBehavior::act(Instruction_assignment& i) {
    out << i.dst_->emit() << " <- " << i.src_->emit() << "\n";
  }

  void CodeGenBehavior::act(Instruction_op& i) {
    out << i.dst_->emit()
        << " <- " << i.lhs_->emit()
        << " " << op_to_str(i.op_)
        << " " << i.rhs_->emit()
        << "\n";
  }

  std::string CodeGenBehavior::element_address(const std::string& base, const std::vector<Item*>& indexes, bool tuple) {
    const size_t dims = indexes.size();
    const int64_t header = tuple ? 8 : 8 * static_cast<int64_t>(dims + 1);

    std::string index;
    int64_t constant_index = 0;
    bool all_constant = true;
    for (size_t d = 0; d < dims; d++) {
      auto* n = dynamic_cast<Number*>(indexes[d]);
      if (d > 0) {
        std::string len_enc = temp();
        std::string len = temp();
        std::string addr = temp();
        out << addr << " <- " << base << " + " << (8 * (d + 1)) << "\n";
        out << len_enc << " <- load " << addr << "\n";
        out << len << " <- " << len_enc << " >> 1\n";
        if (all_constant) {
          index = temp();
          out << index << " <- " << constant_index << " * " << len << "\n";
        } else {
          out << index << " <- " << index << " * " << len << "\n";
        }
        all_constant = false;
        if (n) {
          out << index << " <- " << index << " + " << n->number_ << "\n";
        } else {
          out << index << " <- " << index << " + " << indexes[d]->emit() << "\n";
        }
        continue;
      }
      if (n) {
        constant_index = n->number_;
      } else {
        all_constant = false;
        index = temp();
        out << index << " <- " << indexes[d]->emit() << "\n";
      }
    }

    std::string addr = temp();
    if (all_constant) {
      out << addr << " <- " << base << " + " << (header + 8 * constant_index) << "\n";
      return addr;
    }
    std::string offset = temp();
    out << offset << " <- " << index << " << 3\n";
    out << offset << " <- " << offset << " + " << base << "\n";
    out << addr << " <- " << offset << " + " << header << "\n";
    return addr;
  }

  void CodeGenBehavior::act(Instruction_index_load& i) {
    std::string base = i.src_->emit();
    std::string addr = element_address(base, i.indexes_, is_tuple_var(cur_function, base));
    out << i.dst_->emit() << " <- load " << addr << "\n";
  }

  void CodeGenBehavior::act(Instruction_index_store& i) {
    std::string base = i.dst_->emit();
    std::string addr = element_address(base, i.indexes_, is_tuple_var(cur_function, base));
    out << "store " << addr << " <- " << i.src_->emit() << "\n";
  }

  void CodeGenBehavior::act(Instruction_length& i) {

    std::string encoded = temp();

    out << encoded << " <- load " << i.src_->emit() << "\n";
    out << i.dst_->emit() << " <- " << encoded << " << 1\n";
    out << i.dst_->emit() << " <- " << i.dst_->emit() << " + 1\n";
  }

  void CodeGenBehavior::act(Instruction_length_t& i) {

    std::string addr = temp();

    if (auto* n = dynamic_cast<Number*>(i.t_)) {
      out << addr << " <- " << i.src_->emit() << " + " << (8 * (n->number_ + 1)) << "\n";
    } else {
      std::string dim_offset = temp();
      out << dim_offset << " <- " << i.t_->emit() << " << 3\n";
      out << dim_offset << " <- " << dim_offset << " + " << i.src_->emit() << "\n";
      out << addr << " <- " << dim_offset << " + 8\n";
    }

    out << i.dst_->emit() << " <- load " << addr << "\n";
  }

  void CodeGenBehavior::act(Instruction_call& i) {

    out << "call " << emit_callee(i.c_, i.callee_) << "(";

    for (size_t k = 0; k < i.args_.size(); k++) {
      if (k) out << ", ";
      out << i.args_[k]->emit();
    }

    out << ")\n";
  }

  void CodeGenBehavior::act(Instruction_call_assignment& i) {

    out << i.dst_->emit()
        << " <- call "
        << emit_callee(i.c_, i.callee_)
        << "(";

    for (size_t k = 0; k < i.args_.size(); k++) {
      if (k) out << ", ";
      out << i.args_[k]->emit();
    }

    out << ")\n";
  }


  void CodeGenBehavior::act(Instruction_new_array& i) {

    std::vector<std::string> decoded_dims;

    for (auto item : i.args_) {
      std::string t = temp();
      out << t << " <- " << item->emit() << " >> 1\n";
      decoded_dims.push_back(t);
    }

    std::string size_temp = temp();

    if (decoded_dims.size() == 1) {
      out << size_temp << " <- " << decoded_dims[0] << "\n";
    } else {
      out << size_temp << " <- " 
          << decoded_dims[0] << " * " 
          << decoded_dims[1] << "\n";

      for (size_t k = 2; k < decoded_dims.size(); k++) {
        std::string next = temp();
        out << next << " <- " 
            << size_temp << " * " 
            << decoded_dims[k] << "\n";
        size_temp = next;
      }
    }

    out << size_temp << " <- " 
        << size_temp << " + " 
        << decoded_dims.size() << "\n";

    out << size_temp << " <- " << size_temp << " << 1\n";
    out << size_temp << " <- " << size_temp << " + 1\n";

    out << i.dst_->emit()
        << " <- call allocate(" 
        << size_temp << ", 1)\n";

    for (size_t k = 0; k < i.args_.size(); k++) {
      std::string addr = temp();
      out << addr << " <- " 
          << i.dst_->emit() 
          << " + " << 8 * (k + 1) << "\n";

      out << "store " << addr 
          << " <- " 
          << i.args_[k]->emit() 
          << "\n";
    }
  }

  void CodeGenBehavior::act(Instruction_new_tuple& i) {
    out << i.dst_->emit()
        << " <- call allocate(" << i.t_->emit() << ", 1)\n";
  }

  void CodeGenBehavior::act(Instruction_break_uncond& i) {
    if (next_bb && i.label_->emit() == next_bb->label_->emit()) {
      return;
    }
    out << "br " << i.label_->emit() << "\n";
  }

  void CodeGenBehavior::act(Instruction_break_cond& i) {
    const std::string L1 = i.label1_->emit();
    const std::string L2 = i.label2_->emit();
    const std::string next = next_bb ? next_bb->label_->emit() : "";

    if (next_bb && next == L2) {
      out << "br " << i.t_->emit() << " " << L1 << "\n";
      return;
    }

    if (next_bb && next == L1) {
      std::string neg = temp();
      out << neg << " <- " << i.t_->emit() << " = 0\n";
      out << "br " << neg << " " << L2 << "\n";
      return;
    }

    out << "br " << i.t_->emit() << " " << L1 << "\n";
    out << "br " << L2 << "\n";
  }

  void CodeGenBehavior::act(Instruction_return& i) {
    out << "return\n";
  }

  void CodeGenBehavior::act(Instruction_return_t& i) {
    out << "return " << i.t_->emit() << "\n";
  }

  std::string CodeGenBehavior::temp() {
    return "%v" + std::to_string(temp_counter++); 
  }


  void generate_code(Program& p) {
    std::ofstream outputFile("prog.L3");
    CodeGenBehavior b(outputFile);
    p.accept(b);
  }

}