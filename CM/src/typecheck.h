#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <CM.h>
#include <behavior.h>


namespace CM {

  class TypeCheckBehavior : public Behavior {
  public:
    TypeCheckBehavior(const std::string &file);

    void act(Program& p) override;
    void act(Function& f) override;

    void act(Number& e) override;
    void act(Variable& e) override;
    void act(Binary& e) override;
    void act(Unary& e) override;
    void act(Index& e) override;
    void act(Call& e) override;
    void act(NewArray& e) override;
    void act(Conditional& e) override;

    void act(Block& s) override;
    void act(Declaration& s) override;
    void act(Assign& s) override;
    void act(ExpressionStatement& s) override;
    void act(If& s) override;
    void act(While& s) override;
    void act(For& s) override;
    void act(Break& s) override;
    void act(Continue& s) override;
    void act(DoWhile& s) override;
    void act(Return& s) override;

    std::vector<std::string> errors;
    std::vector<std::string> warnings;

  private:
    Type check(Expression* e);
    void expect_int(Expression* e, const std::string &what);
    void declare(const std::string &name, Type type, Position pos);
    bool lookup(const std::string &name, Type &type) const;
    void error(Position pos, const std::string &message);
    void warning(Position pos, const std::string &message);

    std::string file;
    Program* program = nullptr;
    Function* cur_function = nullptr;
    std::vector<std::unordered_map<std::string, Type>> scopes;
    int loop_depth = 0;
  };

  bool check_program(Program& p, const std::string &file);

}
