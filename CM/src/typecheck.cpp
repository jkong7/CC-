#include <iostream>
#include <unordered_set>

#include <typecheck.h>

namespace CM {

  static const std::unordered_set<std::string> RESERVED = {
    "print", "input", "length", "int64", "tuple", "code", "goto", "br", "Array", "Tuple"
  };

  static bool always_returns(const Statement* s) {
    if (!s) return false;
    if (dynamic_cast<const Return*>(s)) return true;
    if (auto* b = dynamic_cast<const Block*>(s)) {
      for (auto* inner : b->statements_) {
        if (always_returns(inner)) return true;
      }
      return false;
    }
    if (auto* i = dynamic_cast<const If*>(s)) {
      return always_returns(i->then_) && always_returns(i->else_);
    }
    return false;
  }

  TypeCheckBehavior::TypeCheckBehavior(const std::string &file)
    : file (file) {
    return;
  }

  void TypeCheckBehavior::error(Position pos, const std::string &message) {
    errors.push_back(file + ":" + std::to_string(pos.line) + ":" + std::to_string(pos.column) + ": error: " + message);
  }

  void TypeCheckBehavior::warning(Position pos, const std::string &message) {
    warnings.push_back(file + ":" + std::to_string(pos.line) + ":" + std::to_string(pos.column) + ": warning: " + message);
  }

  Type TypeCheckBehavior::check(Expression* e) {
    e->accept(*this);
    return e->type;
  }

  void TypeCheckBehavior::expect_int(Expression* e, const std::string &what) {
    Type t = check(e);
    if (!t.is_int()) {
      error(e->pos, what + " must be int, found " + t.str());
    }
  }

  void TypeCheckBehavior::declare(const std::string &name, Type type, Position pos) {
    if (RESERVED.count(name)) {
      error(pos, "'" + name + "' is a reserved name");
    } else if (program->find_function(name)) {
      error(pos, "'" + name + "' shadows a function");
    } else if (scopes.back().count(name)) {
      error(pos, "redeclaration of '" + name + "'");
    }
    if (type.is_void) {
      error(pos, "variable '" + name + "' declared void");
    }
    scopes.back()[name] = type;
  }

  bool TypeCheckBehavior::lookup(const std::string &name, Type &type) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) {
        type = found->second;
        return true;
      }
    }
    return false;
  }

  void TypeCheckBehavior::act(Program& p) {
    program = &p;
    std::unordered_set<std::string> seen;
    for (auto* f : p.functions) {
      if (is_builtin(f->name) || RESERVED.count(f->name)) {
        error(f->pos, "'" + f->name + "' is a reserved name");
      }
      if (!seen.insert(f->name).second) {
        error(f->pos, "redefinition of function '" + f->name + "'");
      }
    }
    auto* main = p.find_function("main");
    if (!main) {
      error(Position{1, 1}, "no main function");
    } else if (!main->return_type.is_void || !main->params.empty()) {
      error(main->pos, "main must be declared as 'void main()'");
    }
    for (auto* f : p.functions) {
      f->accept(*this);
    }
  }

  void TypeCheckBehavior::act(Function& f) {
    cur_function = &f;
    scopes.clear();
    scopes.emplace_back();
    for (auto &[type, name] : f.params) {
      declare(name, type, f.pos);
    }
    f.body->accept(*this);
    scopes.pop_back();
    if (!f.return_type.is_void && !always_returns(f.body)) {
      warning(f.pos, "function '" + f.name + "' may finish without returning a value");
    }
    cur_function = nullptr;
  }

  void TypeCheckBehavior::act(Number& e) {
    e.type = int_type();
  }

  void TypeCheckBehavior::act(Variable& e) {
    e.type = int_type();
    if (!lookup(e.name_, e.type)) {
      if (program->find_function(e.name_)) {
        error(e.pos, "function '" + e.name_ + "' used as a value");
      } else {
        error(e.pos, "use of undeclared variable '" + e.name_ + "'");
      }
    }
  }

  void TypeCheckBehavior::act(Binary& e) {
    std::string what = std::string("operand of '") + op_to_str(e.op_) + "'";
    expect_int(e.lhs_, what);
    expect_int(e.rhs_, what);
    e.type = int_type();
  }

  void TypeCheckBehavior::act(Unary& e) {
    expect_int(e.operand_, e.op_ == neg ? "operand of '-'" : e.op_ == lnot ? "operand of '!'" : "operand of '~'");
    e.type = int_type();
  }

  void TypeCheckBehavior::act(Index& e) {
    Type base = check(e.base_);
    e.type = int_type();
    for (auto* idx : e.indexes_) expect_int(idx, "array index");
    if (base.is_void || base.dims == 0) {
      error(e.pos, "cannot index a value of type " + base.str());
    } else if (static_cast<int64_t>(e.indexes_.size()) != base.dims) {
      error(e.pos, "array of type " + base.str() + " needs " + std::to_string(base.dims) + " indexes, found " + std::to_string(e.indexes_.size()));
    }
  }

  void TypeCheckBehavior::act(Call& e) {
    if (e.callee_ == "print") {
      e.type = void_type();
      if (e.args_.size() != 1) {
        error(e.pos, "print takes 1 argument");
      }
      for (auto* a : e.args_) {
        if (check(a).is_void) error(a->pos, "cannot print a void value");
      }
      return;
    }
    if (e.callee_ == "input") {
      e.type = int_type();
      if (!e.args_.empty()) error(e.pos, "input takes no arguments");
      return;
    }
    if (e.callee_ == "length") {
      e.type = int_type();
      if (e.args_.size() != 2) {
        error(e.pos, "length takes an array and a dimension");
        return;
      }
      Type a = check(e.args_[0]);
      if (a.is_void || a.dims == 0) error(e.args_[0]->pos, "length needs an array, found " + a.str());
      expect_int(e.args_[1], "array dimension");
      return;
    }

    auto* f = program->find_function(e.callee_);
    if (!f) {
      error(e.pos, "call to undefined function '" + e.callee_ + "'");
      e.type = int_type();
      for (auto* a : e.args_) check(a);
      return;
    }
    e.type = f->return_type;
    if (f->params.size() != e.args_.size()) {
      error(e.pos, "'" + f->name + "' takes " + std::to_string(f->params.size()) + " arguments, found " + std::to_string(e.args_.size()));
    }
    for (size_t k = 0; k < e.args_.size(); k++) {
      Type t = check(e.args_[k]);
      if (k < f->params.size() && t != f->params[k].first) {
        error(e.args_[k]->pos, "argument " + std::to_string(k + 1) + " of '" + f->name + "' must be " + f->params[k].first.str() + ", found " + t.str());
      }
    }
  }

  void TypeCheckBehavior::act(Conditional& e) {
    expect_int(e.cond_, "condition");
    Type a = check(e.then_);
    Type b = check(e.else_);
    if (a != b) {
      error(e.pos, "branches of '?:' have different types " + a.str() + " and " + b.str());
    }
    e.type = a;
  }

  void TypeCheckBehavior::act(NewArray& e) {
    for (auto* d : e.dims_) expect_int(d, "array dimension");
    e.type = array_type(static_cast<int64_t>(e.dims_.size()));
  }

  void TypeCheckBehavior::act(Block& s) {
    scopes.emplace_back();
    for (auto* inner : s.statements_) inner->accept(*this);
    scopes.pop_back();
  }

  void TypeCheckBehavior::act(Declaration& s) {
    for (auto &[name, init] : s.declarators_) {
      if (init) {
        Type t = check(init);
        if (t != s.type_) {
          error(init->pos, "cannot initialize '" + name + "' of type " + s.type_.str() + " with " + t.str());
        }
      }
      declare(name, s.type_, s.pos);
    }
  }

  void TypeCheckBehavior::act(Assign& s) {
    Type target = check(s.target_);
    for (auto* idx : s.indexes_) expect_int(idx, "array index");
    if (!s.indexes_.empty()) {
      if (target.dims == 0) {
        error(s.target_->pos, "cannot index a value of type " + target.str());
      } else if (static_cast<int64_t>(s.indexes_.size()) != target.dims) {
        error(s.target_->pos, "array of type " + target.str() + " needs " + std::to_string(target.dims) + " indexes, found " + std::to_string(s.indexes_.size()));
      }
      target = int_type();
    }
    Type value = check(s.value_);
    if (s.compound_ && (!target.is_int() || !value.is_int())) {
      error(s.pos, std::string("compound '") + op_to_str(s.op_) + "=' needs int operands");
    } else if (value != target) {
      error(s.value_->pos, "cannot assign " + value.str() + " to " + target.str());
    }
  }

  void TypeCheckBehavior::act(ExpressionStatement& s) {
    check(s.expression_);
    if (!dynamic_cast<Call*>(s.expression_)) {
      warning(s.pos, "expression result unused");
    }
  }

  void TypeCheckBehavior::act(If& s) {
    expect_int(s.cond_, "condition");
    s.then_->accept(*this);
    if (s.else_) s.else_->accept(*this);
  }

  void TypeCheckBehavior::act(While& s) {
    expect_int(s.cond_, "condition");
    loop_depth++;
    s.body_->accept(*this);
    loop_depth--;
  }

  void TypeCheckBehavior::act(For& s) {
    scopes.emplace_back();
    if (s.init_) s.init_->accept(*this);
    if (s.cond_) expect_int(s.cond_, "condition");
    if (s.step_) s.step_->accept(*this);
    loop_depth++;
    s.body_->accept(*this);
    loop_depth--;
    scopes.pop_back();
  }

  void TypeCheckBehavior::act(DoWhile& s) {
    loop_depth++;
    s.body_->accept(*this);
    loop_depth--;
    expect_int(s.cond_, "condition");
  }

  void TypeCheckBehavior::act(Break& s) {
    if (loop_depth == 0) error(s.pos, "break outside of a loop");
  }

  void TypeCheckBehavior::act(Continue& s) {
    if (loop_depth == 0) error(s.pos, "continue outside of a loop");
  }

  void TypeCheckBehavior::act(Return& s) {
    Type expected = cur_function->return_type;
    if (!s.value_) {
      if (!expected.is_void) error(s.pos, "'" + cur_function->name + "' must return " + expected.str());
      return;
    }
    Type t = check(s.value_);
    if (expected.is_void) {
      error(s.value_->pos, "void function '" + cur_function->name + "' cannot return a value");
    } else if (t != expected) {
      error(s.value_->pos, "'" + cur_function->name + "' returns " + expected.str() + ", found " + t.str());
    }
  }

  bool check_program(Program& p, const std::string &file) {
    TypeCheckBehavior b(file);
    p.accept(b);
    for (auto &w : b.warnings) std::cerr << w << std::endl;
    for (auto &e : b.errors) std::cerr << e << std::endl;
    return b.errors.empty();
  }

}
