#include <codegen.h>
#include <parser.h>
#include <typecheck.h>
#include <sstream>

namespace CM {

  static std::string lb_type(Type t) {
    if (t.is_void) return "void";
    std::string s = "int64";
    for (int64_t d = 0; d < t.dims; d++) s += "[]";
    return s;
  }

  static const char* lb_op(BinOp op) {
    switch (op) {
      case add:  return "+";
      case sub:  return "-";
      case mul:  return "*";
      case band: return "&";
      case shl:  return "<<";
      case shr:  return ">>";
      case lt:   return "<";
      case le:   return "<=";
      case gt:   return ">";
      case ge:   return ">=";
      case eq:   return "=";
      default:   return "<?>";
    }
  }

  static bool constant(Expression* e, int64_t &value) {
    if (auto* n = dynamic_cast<Number*>(e)) {
      value = n->value_;
      return true;
    }
    return false;
  }

  bool CodeGenBehavior::needs_division() const {
    return uses_division;
  }

  CodeGenBehavior::CodeGenBehavior(std::ostream &o)
    : out (o) {
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
    loops.clear();
    depth = 1;
    last_line = 0;

    out << lb_type(f.return_type) << " " << f.name << " (";
    for (size_t k = 0; k < f.params.size(); k++) {
      if (k) out << ", ";
      out << lb_type(f.params[k].first) << " " << f.params[k].second;
    }
    out << ") {\n";

    for (auto* s : f.body->statements_) s->accept(*this);

    for (auto &[type, name] : temps) {
      out << "  " << lb_type(type) << " " << name << "\n";
    }
    for (auto &l : body) {
      out << l << "\n";
    }
    out << "}\n\n";
  }

  std::string CodeGenBehavior::gen(Expression* e) {
    e->accept(*this);
    return result;
  }

  void CodeGenBehavior::act(Number& e) {
    result = std::to_string(e.value_);
  }

  void CodeGenBehavior::act(Variable& e) {
    result = e.name_;
  }

  void CodeGenBehavior::act(Binary& e) {
    if (e.op_ == land || e.op_ == lor) {
      std::string t = temp(int_type());
      std::string on_true = fresh_label();
      std::string on_false = fresh_label();
      std::string done = fresh_label();
      branch(&e, on_true, on_false);
      label(on_true);
      line(t + " <- 1");
      jump(done);
      label(on_false);
      line(t + " <- 0");
      label(done);
      result = t;
      return;
    }

    std::string a = gen(e.lhs_);
    std::string b = gen(e.rhs_);
    result = arithmetic(e.op_, a, b);
  }

  std::string CodeGenBehavior::arithmetic(BinOp op, const std::string &a, const std::string &b) {
    std::string t = temp(int_type());
    switch (op) {
      case ne:
        line(t + " <- " + a + " = " + b);
        line(t + " <- 1 - " + t);
        break;
      case div:
      case mod:
        uses_division = true;
        line(t + " <- " + std::string(op == div ? "_div" : "_mod") + "(" + a + ", " + b + ")");
        break;
      case bor: {
        std::string both = temp(int_type());
        line(both + " <- " + a + " & " + b);
        line(t + " <- " + a + " + " + b);
        line(t + " <- " + t + " - " + both);
        break;
      }
      case bxor: {
        std::string both = temp(int_type());
        line(both + " <- " + a + " & " + b);
        line(both + " <- " + both + " << 1");
        line(t + " <- " + a + " + " + b);
        line(t + " <- " + t + " - " + both);
        break;
      }
      default:
        line(t + " <- " + a + " " + lb_op(op) + " " + b);
        break;
    }
    return t;
  }

  void CodeGenBehavior::act(Unary& e) {
    int64_t value;
    if (e.op_ == neg && constant(e.operand_, value)) {
      result = std::to_string(-value);
      return;
    }
    std::string a = gen(e.operand_);
    std::string t = temp(int_type());
    if (e.op_ == neg) {
      line(t + " <- 0 - " + a);
    } else if (e.op_ == bnot) {
      line(t + " <- -1 - " + a);
    } else {
      line(t + " <- " + a + " = 0");
    }
    result = t;
  }

  void CodeGenBehavior::act(Index& e) {
    std::string base = gen(e.base_);
    std::string access = base;
    for (auto* idx : e.indexes_) access += "[" + gen(idx) + "]";
    std::string t = temp(e.type);
    line(t + " <- " + access);
    result = t;
  }

  void CodeGenBehavior::act(Call& e) {
    std::vector<std::string> args;
    for (auto* a : e.args_) args.push_back(gen(a));

    if (e.callee_ == "length") {
      std::string t = temp(int_type());
      line(t + " <- length " + args[0] + " " + args[1]);
      result = t;
      return;
    }
    std::string call = e.callee_ + "(" + join(args) + ")";
    if (e.type.is_void) {
      line(call);
      result = "";
      return;
    }
    std::string t = temp(e.type);
    line(t + " <- " + call);
    result = t;
  }

  void CodeGenBehavior::act(Conditional& e) {
    std::string t = temp(e.type);
    std::string on_true = fresh_label();
    std::string on_false = fresh_label();
    std::string done = fresh_label();
    branch(e.cond_, on_true, on_false);
    label(on_true);
    line(t + " <- " + gen(e.then_));
    jump(done);
    label(on_false);
    line(t + " <- " + gen(e.else_));
    label(done);
    result = t;
  }

  void CodeGenBehavior::act(NewArray& e) {
    std::vector<std::string> dims;
    for (auto* d : e.dims_) dims.push_back(gen(d));
    std::string t = temp(e.type);
    line(t + " <- new Array(" + join(dims) + ")");
    result = t;
  }

  void CodeGenBehavior::branch(Expression* e, const std::string &on_true, const std::string &on_false) {
    source_line(e->pos);
    if (auto* b = dynamic_cast<Binary*>(e)) {
      if (b->op_ == land) {
        std::string mid = fresh_label();
        branch(b->lhs_, mid, on_false);
        label(mid);
        branch(b->rhs_, on_true, on_false);
        return;
      }
      if (b->op_ == lor) {
        std::string mid = fresh_label();
        branch(b->lhs_, on_true, mid);
        label(mid);
        branch(b->rhs_, on_true, on_false);
        return;
      }
      if (is_comparison(b->op_)) {
        std::string l = gen(b->lhs_);
        std::string r = gen(b->rhs_);
        if (b->op_ == ne) {
          line("if (" + l + " = " + r + ") " + on_false + " " + on_true);
        } else {
          line("if (" + l + " " + lb_op(b->op_) + " " + r + ") " + on_true + " " + on_false);
        }
        return;
      }
    }
    if (auto* u = dynamic_cast<Unary*>(e)) {
      if (u->op_ == lnot) {
        branch(u->operand_, on_false, on_true);
        return;
      }
    }
    int64_t value;
    if (constant(e, value)) {
      jump(value != 0 ? on_true : on_false);
      return;
    }
    std::string v = gen(e);
    line("if (" + v + " = 0) " + on_false + " " + on_true);
  }

  void CodeGenBehavior::act(Block& s) {
    line("{");
    depth++;
    for (auto* inner : s.statements_) inner->accept(*this);
    depth--;
    line("}");
  }

  void CodeGenBehavior::act(Declaration& s) {
    source_line(s.pos);
    for (auto &[name, init] : s.declarators_) {
      std::string value = init ? gen(init) : "";
      line(lb_type(s.type_) + " " + name);
      if (init) line(name + " <- " + value);
    }
  }

  void CodeGenBehavior::act(Assign& s) {
    source_line(s.pos);
    std::vector<std::string> idxs;
    for (auto* idx : s.indexes_) idxs.push_back(gen(idx));
    std::string target = s.target_->name_;
    for (auto &i : idxs) target += "[" + i + "]";

    std::string value = gen(s.value_);
    if (s.compound_) {
      std::string current = s.target_->name_;
      if (!idxs.empty()) {
        current = temp(int_type());
        line(current + " <- " + target);
      }
      bool direct = s.op_ == add || s.op_ == sub || s.op_ == mul || s.op_ == band || s.op_ == shl || s.op_ == shr;
      if (idxs.empty() && direct) {
        line(current + " <- " + current + " " + lb_op(s.op_) + " " + value);
        return;
      }
      std::string combined = arithmetic(s.op_, current, value);
      if (idxs.empty()) {
        line(s.target_->name_ + " <- " + combined);
        return;
      }
      value = combined;
    }
    line(target + " <- " + value);
  }

  void CodeGenBehavior::act(ExpressionStatement& s) {
    source_line(s.pos);
    gen(s.expression_);
  }

  void CodeGenBehavior::act(If& s) {
    std::string then_label = fresh_label();
    std::string else_label = fresh_label();
    std::string done = fresh_label();
    branch(s.cond_, then_label, s.else_ ? else_label : done);
    label(then_label);
    s.then_->accept(*this);
    if (s.else_) {
      jump(done);
      label(else_label);
      s.else_->accept(*this);
    }
    label(done);
  }

  void CodeGenBehavior::act(While& s) {
    std::string cond = fresh_label();
    std::string loop_body = fresh_label();
    std::string done = fresh_label();
    label(cond);
    branch(s.cond_, loop_body, done);
    label(loop_body);
    loops.push_back({cond, done});
    s.body_->accept(*this);
    loops.pop_back();
    jump(cond);
    label(done);
  }

  void CodeGenBehavior::act(For& s) {
    std::string cond = fresh_label();
    std::string loop_body = fresh_label();
    std::string step = fresh_label();
    std::string done = fresh_label();
    line("{");
    depth++;
    if (s.init_) s.init_->accept(*this);
    label(cond);
    if (s.cond_) {
      branch(s.cond_, loop_body, done);
    }
    label(loop_body);
    loops.push_back({step, done});
    s.body_->accept(*this);
    loops.pop_back();
    label(step);
    if (s.step_) s.step_->accept(*this);
    jump(cond);
    label(done);
    depth--;
    line("}");
  }

  void CodeGenBehavior::act(DoWhile& s) {
    std::string loop_body = fresh_label();
    std::string cond = fresh_label();
    std::string done = fresh_label();
    label(loop_body);
    loops.push_back({cond, done});
    s.body_->accept(*this);
    loops.pop_back();
    label(cond);
    branch(s.cond_, loop_body, done);
    label(done);
  }

  void CodeGenBehavior::act(Break& s) {
    jump(loops.back().break_label);
  }

  void CodeGenBehavior::act(Continue& s) {
    jump(loops.back().continue_label);
  }

  void CodeGenBehavior::act(Return& s) {
    source_line(s.pos);
    if (s.value_) {
      line("return " + gen(s.value_));
    } else {
      line("return");
    }
  }

  std::string CodeGenBehavior::temp(Type t) {
    std::string name = "_t" + std::to_string(temp_counter++);
    temps.emplace_back(t, name);
    return name;
  }

  std::string CodeGenBehavior::fresh_label() {
    return ":_l" + std::to_string(label_counter++);
  }

  std::string CodeGenBehavior::join(const std::vector<std::string> &items) const {
    std::string s;
    for (size_t k = 0; k < items.size(); k++) {
      if (k) s += ", ";
      s += items[k];
    }
    return s;
  }

  void CodeGenBehavior::line(const std::string &s) {
    body.push_back(std::string(2 * depth, ' ') + s);
  }

  void CodeGenBehavior::label(const std::string &l) {
    body.push_back(std::string(2 * (depth - 1), ' ') + l);
  }

  void CodeGenBehavior::source_line(const Position &pos) {
    if (pos.line != last_line) {
      body.push_back("//#line " + std::to_string(pos.line));
      last_line = pos.line;
    }
  }

  void CodeGenBehavior::jump(const std::string &l) {
    line("goto " + l);
  }

  static const char* PRELUDE = R"(
int cm_udiv(int a, int b) {
  int q = 0;
  int r = 0;
  for (int i = 61; i >= 0; i--) {
    r = (r << 1) + ((a >> i) & 1);
    if (r >= b) {
      r = r - b;
      q = q + (1 << i);
    }
  }
  return q;
}

int cm_div(int a, int b) {
  if (b == 0) return 0;
  int negative = 0;
  if (a < 0) {
    a = -a;
    negative = 1 - negative;
  }
  if (b < 0) {
    b = -b;
    negative = 1 - negative;
  }
  int q = cm_udiv(a, b);
  if (negative) return -q;
  return q;
}

int cm_mod(int a, int b) {
  if (b == 0) return 0;
  return a - cm_div(a, b) * b;
}
)";

  void generate_code(Program& p) {
    std::ofstream outputFile("prog.b");
    CodeGenBehavior b(outputFile);
    p.accept(b);
    if (!b.needs_division()) return;

    Program prelude = parse_string(PRELUDE, "<prelude>");
    check_program(prelude, "<prelude>");
    std::stringstream text;
    CodeGenBehavior pb(text);
    prelude.accept(pb);
    std::string lb = text.str();
    for (size_t at = lb.find("cm_"); at != std::string::npos; at = lb.find("cm_", at)) {
      lb.replace(at, 3, "_");
    }
    outputFile << lb;
  }

}
