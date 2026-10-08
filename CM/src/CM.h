#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <utility>



namespace CM {
  struct Behavior;

  // Types

  struct Type {
    bool is_void = false;
    int64_t dims = 0;

    bool operator==(const Type &o) const { return is_void == o.is_void && dims == o.dims; }
    bool operator!=(const Type &o) const { return !(*this == o); }
    bool is_int() const { return !is_void && dims == 0; }
    std::string str() const;
  };

  Type int_type();
  Type void_type();
  Type array_type(int64_t dims);


  // Enums

  enum BinOp {add, sub, mul, band, shl, shr, lt, le, gt, ge, eq, ne, land, lor};

  enum UnOp {neg, lnot};


  struct Position {
    int64_t line = 0;
    int64_t column = 0;
  };


  /*
   * Expressions.
   */
  class Expression {
    public:
      virtual ~Expression() = default;
      virtual void accept(Behavior& b) = 0;

      Position pos;
      Type type;
  };

  class Number : public Expression {
    public:
      Number(int64_t n);
      void accept(Behavior& b) override;

      int64_t value_;
  };

  class Variable : public Expression {
    public:
      Variable(const std::string &name);
      void accept(Behavior& b) override;

      std::string name_;
  };

  class Binary : public Expression {
    public:
      Binary(BinOp op, Expression* lhs, Expression* rhs);
      void accept(Behavior& b) override;

      BinOp op_;
      Expression* lhs_;
      Expression* rhs_;
  };

  class Unary : public Expression {
    public:
      Unary(UnOp op, Expression* operand);
      void accept(Behavior& b) override;

      UnOp op_;
      Expression* operand_;
  };

  class Index : public Expression {
    public:
      Index(Expression* base, std::vector<Expression*> indexes);
      void accept(Behavior& b) override;

      Expression* base_;
      std::vector<Expression*> indexes_;
  };

  class Call : public Expression {
    public:
      Call(const std::string &callee, std::vector<Expression*> args);
      void accept(Behavior& b) override;

      std::string callee_;
      std::vector<Expression*> args_;
  };

  class NewArray : public Expression {
    public:
      NewArray(std::vector<Expression*> dims);
      void accept(Behavior& b) override;

      std::vector<Expression*> dims_;
  };


  /*
   * Statements.
   */
  class Statement {
    public:
      virtual ~Statement() = default;
      virtual void accept(Behavior& b) = 0;

      Position pos;
  };

  class Block : public Statement {
    public:
      Block(std::vector<Statement*> statements);
      void accept(Behavior& b) override;

      std::vector<Statement*> statements_;
  };

  class Declaration : public Statement {
    public:
      Declaration(Type type, std::vector<std::pair<std::string, Expression*>> declarators);
      void accept(Behavior& b) override;

      Type type_;
      std::vector<std::pair<std::string, Expression*>> declarators_;
  };

  class Assign : public Statement {
    public:
      Assign(Variable* target, std::vector<Expression*> indexes, bool compound, BinOp op, Expression* value);
      void accept(Behavior& b) override;

      Variable* target_;
      std::vector<Expression*> indexes_;
      bool compound_;
      BinOp op_;
      Expression* value_;
  };

  class ExpressionStatement : public Statement {
    public:
      ExpressionStatement(Expression* e);
      void accept(Behavior& b) override;

      Expression* expression_;
  };

  class If : public Statement {
    public:
      If(Expression* cond, Statement* then_branch, Statement* else_branch);
      void accept(Behavior& b) override;

      Expression* cond_;
      Statement* then_;
      Statement* else_;
  };

  class While : public Statement {
    public:
      While(Expression* cond, Statement* body);
      void accept(Behavior& b) override;

      Expression* cond_;
      Statement* body_;
  };

  class For : public Statement {
    public:
      For(Statement* init, Expression* cond, Statement* step, Statement* body);
      void accept(Behavior& b) override;

      Statement* init_;
      Expression* cond_;
      Statement* step_;
      Statement* body_;
  };

  class Break : public Statement {
    public:
      Break();
      void accept(Behavior& b) override;
  };

  class Continue : public Statement {
    public:
      Continue();
      void accept(Behavior& b) override;
  };

  class Return : public Statement {
    public:
      Return(Expression* value);
      void accept(Behavior& b) override;

      Expression* value_;
  };


  class Function {
    public:
      Type return_type;
      std::string name;
      std::vector<std::pair<Type, std::string>> params;
      Block* body = nullptr;
      Position pos;

      void accept(Behavior& b);
  };

  class Program {
    public:
      std::vector<Function*> functions;

      void accept(Behavior& b);
      Function* find_function(const std::string &name) const;
  };

  bool is_builtin(const std::string &name);
  bool is_comparison(BinOp op);
  const char* op_to_str(BinOp op);

}
