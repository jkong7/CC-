#include <CM.h>
#include <behavior.h>

namespace CM {

  std::string Type::str() const {
    if (is_void) return "void";
    std::string s = "int";
    for (int64_t d = 0; d < dims; d++) s += "[]";
    return s;
  }

  Type int_type() {
    return Type{false, 0};
  }

  Type void_type() {
    return Type{true, 0};
  }

  Type array_type(int64_t dims) {
    return Type{false, dims};
  }


  // Expressions

  Number::Number(int64_t n)
    : value_{n} {
    return;
  }

  Variable::Variable(const std::string &name)
    : name_{name} {
    return;
  }

  Binary::Binary(BinOp op, Expression* lhs, Expression* rhs)
    : op_{op},
      lhs_{lhs},
      rhs_{rhs} {
    return;
  }

  Unary::Unary(UnOp op, Expression* operand)
    : op_{op},
      operand_{operand} {
    return;
  }

  Index::Index(Expression* base, std::vector<Expression*> indexes)
    : base_{base},
      indexes_{std::move(indexes)} {
    return;
  }

  Call::Call(const std::string &callee, std::vector<Expression*> args)
    : callee_{callee},
      args_{std::move(args)} {
    return;
  }

  NewArray::NewArray(std::vector<Expression*> dims)
    : dims_{std::move(dims)} {
    return;
  }


  // Statements

  Block::Block(std::vector<Statement*> statements)
    : statements_{std::move(statements)} {
    return;
  }

  Declaration::Declaration(Type type, std::vector<std::pair<std::string, Expression*>> declarators)
    : type_{type},
      declarators_{std::move(declarators)} {
    return;
  }

  Assign::Assign(Variable* target, std::vector<Expression*> indexes, bool compound, BinOp op, Expression* value)
    : target_{target},
      indexes_{std::move(indexes)},
      compound_{compound},
      op_{op},
      value_{value} {
    return;
  }

  ExpressionStatement::ExpressionStatement(Expression* e)
    : expression_{e} {
    return;
  }

  If::If(Expression* cond, Statement* then_branch, Statement* else_branch)
    : cond_{cond},
      then_{then_branch},
      else_{else_branch} {
    return;
  }

  While::While(Expression* cond, Statement* body)
    : cond_{cond},
      body_{body} {
    return;
  }

  For::For(Statement* init, Expression* cond, Statement* step, Statement* body)
    : init_{init},
      cond_{cond},
      step_{step},
      body_{body} {
    return;
  }

  Break::Break() {
    return;
  }

  Continue::Continue() {
    return;
  }

  Return::Return(Expression* value)
    : value_{value} {
    return;
  }


  // Visitor

  void Program::accept(Behavior& b)              { b.act(*this); }
  void Function::accept(Behavior& b)             { b.act(*this); }
  void Number::accept(Behavior& b)               { b.act(*this); }
  void Variable::accept(Behavior& b)             { b.act(*this); }
  void Binary::accept(Behavior& b)               { b.act(*this); }
  void Unary::accept(Behavior& b)                { b.act(*this); }
  void Index::accept(Behavior& b)                { b.act(*this); }
  void Call::accept(Behavior& b)                 { b.act(*this); }
  void NewArray::accept(Behavior& b)             { b.act(*this); }
  void Block::accept(Behavior& b)                { b.act(*this); }
  void Declaration::accept(Behavior& b)          { b.act(*this); }
  void Assign::accept(Behavior& b)               { b.act(*this); }
  void ExpressionStatement::accept(Behavior& b)  { b.act(*this); }
  void If::accept(Behavior& b)                   { b.act(*this); }
  void While::accept(Behavior& b)                { b.act(*this); }
  void For::accept(Behavior& b)                  { b.act(*this); }
  void Break::accept(Behavior& b)                { b.act(*this); }
  void Continue::accept(Behavior& b)             { b.act(*this); }
  void Return::accept(Behavior& b)               { b.act(*this); }


  Function* Program::find_function(const std::string &name) const {
    for (auto* f : functions) {
      if (f->name == name) return f;
    }
    return nullptr;
  }

  bool is_builtin(const std::string &name) {
    return name == "print" || name == "input" || name == "length";
  }

  bool is_comparison(BinOp op) {
    return op == lt || op == le || op == gt || op == ge || op == eq || op == ne;
  }

  const char* op_to_str(BinOp op) {
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
      case eq:   return "==";
      case ne:   return "!=";
      case land: return "&&";
      case lor:  return "||";
    }
    return "<?>";
  }

}
