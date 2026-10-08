#include "tiler.h"

#include <cassert>
#include <type_traits>
#include <variant>

namespace L3 {

  static bool is_leaf(const Tree& t) {
    return t.kind == TreeType::Leaf && t.leaf.has_value(); 
  }

  static const Tree* ptr(const std::unique_ptr<Tree>& p) {
    return p ? p.get() : nullptr; 
  }

  static std::string leaf_to_str(const Leaf& leaf) {
    return std::visit([](auto&& x) -> std::string {
      using T = std::decay_t<decltype(x)>;
      if constexpr (std::is_same_v<T, NumberLeaf>) return std::to_string(x.n);
      if constexpr (std::is_same_v<T, VarLeaf>)    return x.var;
      if constexpr (std::is_same_v<T, LabelLeaf>)  return x.label;
      if constexpr (std::is_same_v<T, FuncLeaf>)   return x.name;
      return "?leaf";
    }, leaf);
  }

  static std::string leaf_node_to_str(const Tree* t) {
    assert(t && is_leaf(*t));
    return leaf_to_str(*t->leaf);
  }

  static const char* op_to_str(OP op) {
    switch (op) {
      case plus:        return "+=";
      case minus:       return "-=";
      case times:       return "*=";
      case at:          return "&=";
      case left_shift:  return "<<=";
      case right_shift: return ">>=";
    }
    return "?op";
  }

  static const char* cmp_to_str(CMP c) {
    switch (c) {
      case less_than:          return "<";
      case less_than_equal:    return "<=";
      case equal:              return "=";
      case greater_than_equal: return ">=";
      case greater_than:       return ">";
    }
    return "?cmp";
  }

  static bool is_commutative(OP op) {
    return op == plus || op == times || op == at;
  }

  static bool is_number_leaf(const Tree* t, int64_t& n) {
    if (!t || !is_leaf(*t)) return false;
    if (auto* x = std::get_if<NumberLeaf>(&*t->leaf)) {
      n = x->n;
      return true;
    }
    return false;
  }

  static bool is_var_leaf(const Tree* t) {
    return t && is_leaf(*t) && std::holds_alternative<VarLeaf>(*t->leaf);
  }

  static std::string compute_prefix_from_program(const Program& p) {
    std::string longest = "L";
    for (auto* f : p.functions) {
      if (!f) continue;
      for (auto* inst : f->instructions) {
        if (auto* lab = dynamic_cast<Instruction_label*>(inst)) {
          std::string s = lab->label_->emit();
          if (!s.empty() && s[0] == ':') s = s.substr(1);
          if (s.size() > longest.size()) longest = s;
        }
      }
    }
    return ":" + longest + "_global_";
  }



  Emitter::Emitter(std::ostream& out) : out_(out) {}

  void Emitter::line(const std::string& s) {
    out_ << "  " << s << "\n";
  }

  std::string Emitter::fresh_tmp() {
    return "%__tmp" + std::to_string(tmp_next_++);
  }



  void GlobalLabel::enter_function(const std::string& fn) { cur_fn = fn; }

  std::string GlobalLabel::make_label(const std::string& l3_label) {
    const std::string key = cur_fn + "|" + l3_label;

    auto it = labelMap.find(key);
    if (it != labelMap.end()) {
      return prefix + std::to_string(it->second);
    }

    int64_t id = next++;
    labelMap.emplace(key, id);
    return prefix + std::to_string(id);
  }

  std::string GlobalLabel::make_fresh_label() {
    return prefix + std::to_string(next++);
  }






  TilingEngine::TilingEngine(std::ostream& out, GlobalLabel& labeler)
    : emitter_(out), labeler_(labeler) {
  }

std::string TilingEngine::lower_expr(const Tree* t) {

  switch (t->kind) {
    case TreeType::Leaf: {
      return leaf_node_to_str(t);
    }

    case TreeType::BinOp: {
      assert(t->binOp.has_value());
      const Tree* lhs = ptr(t->lhs);
      const Tree* rhs = ptr(t->rhs);
      assert(lhs && rhs);

      std::string l = lower_expr(lhs);
      std::string r = lower_expr(rhs);

      std::string tmp = emitter_.fresh_tmp();
      emitter_.line(tmp + " <- " + l);
      emitter_.line(tmp + " " + op_to_str(*t->binOp) + " " + r);
      return tmp;
    }

    case TreeType::Cmp: {
      assert(t->cmp.has_value());
      const Tree* lhs = ptr(t->lhs);
      const Tree* rhs = ptr(t->rhs);
      assert(lhs && rhs);

      std::string l = lower_expr(lhs);
      std::string r = lower_expr(rhs);

      std::string tmp = emitter_.fresh_tmp();
      CMP c = *t->cmp;

      std::string L = l;
      std::string R = r;
      const char* op_str = nullptr;

      switch (c) {
        case less_than:
        case less_than_equal:
        case equal:
          op_str = cmp_to_str(c);  
          break;

        case greater_than:
          op_str = "<";       
          std::swap(L, R);
          break;

        case greater_than_equal:
          op_str = "<=";           
          std::swap(L, R);
          break;
      }

      emitter_.line(tmp + " <- " + L + " " + op_str + " " + R);
      return tmp;
    }

    case TreeType::Load: {
      const Tree* dst = ptr(t->lhs);
      const Tree* src = ptr(t->rhs);
      assert(src && "Load must have address (rhs)");

      std::string addr = lower_address(src);

      std::string tmp;
      if (dst && is_leaf(*dst)) {
        tmp = leaf_node_to_str(dst);
      } else {
        tmp = emitter_.fresh_tmp();
      }

      emitter_.line(tmp + " <- mem " + addr);
      return tmp;
    }

    case TreeType::Assign:
    case TreeType::Store:
    case TreeType::Return:
    case TreeType::Break:
      return "";
  }

  return "";
}



void TilingEngine::tile_tree(const Tree& t) {
  switch (t.kind) {
    case TreeType::Assign: {
      const Tree* dstNode = ptr(t.lhs);
      const Tree* rhsNode = ptr(t.rhs);
      assert(dstNode && rhsNode);
      assert(is_leaf(*dstNode) && "Assign lhs should be a leaf variable");

      lower_assign(leaf_node_to_str(dstNode), rhsNode);
      break;
    }

    case TreeType::Load: {
      const Tree* dstNode = ptr(t.lhs);
      const Tree* srcNode = ptr(t.rhs);
      assert(dstNode && srcNode);
      assert(is_leaf(*dstNode) && "Load lhs should be a leaf variable");

      std::string dst  = leaf_node_to_str(dstNode);
      std::string addr = lower_address(srcNode);
      emitter_.line(dst + " <- mem " + addr);
      break;
    }

    case TreeType::Store: {
      const Tree* addrNode = ptr(t.lhs);
      const Tree* valNode  = ptr(t.rhs);
      assert(addrNode && valNode);

      std::string addr = lower_address(addrNode);
      std::string val  = lower_expr(valNode);
      emitter_.line("mem " + addr + " <- " + val);
      break;
    }

    case TreeType::Return: {
      if (t.lhs) {
        std::string val = lower_expr(ptr(t.lhs));
        emitter_.line("rax <- " + val);
      }
      emitter_.line("return");
      break;
    }

    case TreeType::Break: {
      const Tree* labelNode = ptr(t.lhs);
      assert(labelNode && is_leaf(*labelNode));
      std::string labelName   = leaf_node_to_str(labelNode);
      std::string globalLabel = labeler_.make_label(labelName);

      if (t.rhs) {
        std::string condition;
        if (lower_condition(ptr(t.rhs), condition)) {
          emitter_.line("cjump " + condition + " " + globalLabel);
        } else {
          std::string cond = lower_expr(ptr(t.rhs));
          emitter_.line("cjump " + cond + " = 1 " + globalLabel);
        }
      } else {
        emitter_.line("goto " + globalLabel);
      }
      break;
    }

    case TreeType::Leaf:
    case TreeType::BinOp:
    case TreeType::Cmp: {
      (void) lower_expr(&t);
      break;
    }
  }
}


  bool TilingEngine::lower_condition(const Tree* t, std::string& condition) {
    if (!t || t->kind != TreeType::Cmp) return false;
    CMP c = *t->cmp;
    const Tree* lhs = ptr(t->lhs);
    const Tree* rhs = ptr(t->rhs);
    bool negate = false;

    int64_t zero;
    if (c == equal && is_number_leaf(rhs, zero) && zero == 0 && lhs->kind == TreeType::Cmp && *lhs->cmp != equal) {
      negate = true;
      c = *lhs->cmp;
      rhs = ptr(lhs->rhs);
      lhs = ptr(lhs->lhs);
    }

    std::string L = lower_expr(lhs);
    std::string R = lower_expr(rhs);

    if (negate) {
      switch (c) {
        case less_than:          c = greater_than_equal; break;
        case less_than_equal:    c = greater_than;       break;
        case greater_than:       c = less_than_equal;    break;
        case greater_than_equal: c = less_than;          break;
        case equal:              break;
      }
    }

    switch (c) {
      case less_than:
      case less_than_equal:
      case equal:
        condition = L + " " + cmp_to_str(c) + " " + R;
        break;
      case greater_than:
        condition = R + " < " + L;
        break;
      case greater_than_equal:
        condition = R + " <= " + L;
        break;
    }
    return true;
  }

  std::string TilingEngine::lower_address(const Tree* t) {
    int64_t offset;
    if (t->kind == TreeType::BinOp && *t->binOp == plus) {
      const Tree* lhs = ptr(t->lhs);
      const Tree* rhs = ptr(t->rhs);
      if (is_number_leaf(rhs, offset) && offset % 8 == 0 && !is_number_leaf(lhs, offset)) {
        is_number_leaf(rhs, offset);
        return lower_expr(lhs) + " " + std::to_string(offset);
      }
      if (is_number_leaf(lhs, offset) && offset % 8 == 0 && !is_number_leaf(rhs, offset)) {
        is_number_leaf(lhs, offset);
        return lower_expr(rhs) + " " + std::to_string(offset);
      }
    }
    std::string base = lower_expr(t);
    if (!is_var_leaf(t) && t->kind == TreeType::Leaf) {
      std::string tmp = emitter_.fresh_tmp();
      emitter_.line(tmp + " <- " + base);
      base = tmp;
    }
    return base + " 0";
  }

  void TilingEngine::lower_assign(const std::string& dst, const Tree* rhs) {
    if (rhs->kind == TreeType::BinOp) {
      std::string l = lower_expr(ptr(rhs->lhs));
      std::string r = lower_expr(ptr(rhs->rhs));
      OP op = *rhs->binOp;
      if (l == dst) {
        emitter_.line(dst + " " + op_to_str(op) + " " + r);
      } else if (r != dst) {
        emitter_.line(dst + " <- " + l);
        emitter_.line(dst + " " + op_to_str(op) + " " + r);
      } else if (is_commutative(op)) {
        emitter_.line(dst + " " + op_to_str(op) + " " + l);
      } else {
        std::string tmp = emitter_.fresh_tmp();
        emitter_.line(tmp + " <- " + l);
        emitter_.line(tmp + " " + op_to_str(op) + " " + r);
        emitter_.line(dst + " <- " + tmp);
      }
      return;
    }
    std::string condition;
    if (lower_condition(rhs, condition)) {
      emitter_.line(dst + " <- " + condition);
      return;
    }
    emitter_.line(dst + " <- " + lower_expr(rhs));
  }

  static const char* const ARG_REGISTERS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

  void TilingEngine::initialize_function_args(const std::vector<Variable*> var_arguments) {
    std::vector<Variable*> vars = var_arguments;
    emitter_.line(std::to_string(vars.size())); 
    for (size_t idx = 0; idx < vars.size(); idx++) {
      if (idx < 6) {
        emitter_.line(vars[idx]->emit() + " <- " + ARG_REGISTERS[idx]);
      } else {
        int64_t offset = 8 * static_cast<int64_t>(vars.size() - 1 - idx);
        emitter_.line(vars[idx]->emit() + " <- stack-arg " + std::to_string(offset));
      }
    }
  }

  template <class CallT>
  void TilingEngine::handle_call(const CallT* call) {
    for (size_t idx = 0; idx < call->args_.size(); ++idx) {
      std::string arg = call->args_[idx]->emit();
      if (idx < 6) {
        emitter_.line(std::string(ARG_REGISTERS[idx]) + " <- " + arg);
      } else {
        int64_t offset = -16 - 8 * static_cast<int64_t>(idx - 6);
        emitter_.line("mem rsp " + std::to_string(offset) + " <- " + arg);
      }
    }

    CallType c = call->c_;
    if (c == CallType::l3) {

      std::string ret = labeler_.make_fresh_label();
      emitter_.line("mem rsp -8 <- " + ret);
      emitter_.line("call " + call->callee_->emit() + " " +
                    std::to_string(call->args_.size()));
      emitter_.line(ret);
    } else if (c == CallType::print) {
      emitter_.line("call print " + std::to_string(call->args_.size()));
    } else if (c == CallType::input) {
      emitter_.line("call input " + std::to_string(call->args_.size()));
    } else if (c == CallType::allocate) {
      emitter_.line("call allocate " + std::to_string(call->args_.size()));
    } else if (c == CallType::tuple_error) {
      emitter_.line("call tuple-error " + std::to_string(call->args_.size()));
    } else if (c == CallType::tensor_error) {
      emitter_.line("call tensor-error " + std::to_string(call->args_.size()));
    }
  }

  void TilingEngine::codegen (const Node& item) {
    if (auto *t = std::get_if<std::unique_ptr<Tree>>(&item)) {
        tile_tree(*t->get());
      } else if (auto *i = std::get_if<Instruction_label*>(&item)) {
        emitter_.line(labeler_.make_label((*i)->label_->emit())); 
      } else if (auto *i = std::get_if<Instruction_call*>(&item)) {
        handle_call(*i);
      } else if (auto *i = std::get_if<Instruction_call_assignment*>(&item)) {
        handle_call(*i);
        emitter_.line((*i)->dst_->emit() + " <- rax");
    }
  }

  void TilingEngine::tile_function(Function& f) {
    labeler_.enter_function(f.name);
    emitter_.line("(" + f.name);
    initialize_function_args(f.var_arguments);
    for (const auto& ctx : f.contexts) {
      for (auto& nodePtr : ctx.nodes) {
        codegen(nodePtr);
      }
    }
    emitter_.line(")");
  }

  void TilingEngine::tile(Program& p) {
    emitter_.line("(@main");
    for (auto* f : p.functions) {
      tile_function(*f);
    }
    emitter_.line(")");
  }

  void tile_program(Program& p, std::ostream& out) {
    GlobalLabel labeler{}; 
    labeler.prefix = compute_prefix_from_program(p);
    TilingEngine eng(out, labeler);
    eng.tile(p);
  }
} 
