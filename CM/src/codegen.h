#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <CM.h>
#include <behavior.h>


namespace CM {

  class CodeGenBehavior : public Behavior {
  public:
    CodeGenBehavior(std::ofstream &o);

    void act(Program& p) override;
    void act(Function& f) override;

    void act(Number& e) override;
    void act(Variable& e) override;
    void act(Binary& e) override;
    void act(Unary& e) override;
    void act(Index& e) override;
    void act(Call& e) override;
    void act(NewArray& e) override;

    void act(Block& s) override;
    void act(Declaration& s) override;
    void act(Assign& s) override;
    void act(ExpressionStatement& s) override;
    void act(If& s) override;
    void act(While& s) override;
    void act(For& s) override;
    void act(Break& s) override;
    void act(Continue& s) override;
    void act(Return& s) override;

  private:
    struct Loop {
      std::string continue_label;
      std::string break_label;
    };

    std::string gen(Expression* e);
    void branch(Expression* e, const std::string &on_true, const std::string &on_false);
    std::string temp(Type t);
    std::string fresh_label();
    std::string join(const std::vector<std::string> &items) const;
    void line(const std::string &s);
    void label(const std::string &l);
    void jump(const std::string &l);
    void source_line(const Position &pos);

    std::vector<std::string> body;
    std::vector<std::pair<Type, std::string>> temps;
    std::vector<Loop> loops;
    std::string result;
    int depth = 1;
    int64_t last_line = 0;
    int temp_counter = 0;
    int label_counter = 0;
    std::ofstream &out;
  };

  void generate_code(Program& p);

}
