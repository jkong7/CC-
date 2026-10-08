#pragma once

namespace CM {
  class Program;
  class Function;

  class Number;
  class Variable;
  class Binary;
  class Unary;
  class Index;
  class Call;
  class NewArray;

  class Block;
  class Declaration;
  class Assign;
  class ExpressionStatement;
  class If;
  class While;
  class For;
  class Break;
  class Continue;
  class Return;

  class Behavior {
    public:
      virtual ~Behavior() = default;

      virtual void act(Program &p) = 0;
      virtual void act(Function &f) = 0;

      virtual void act(Number &e) = 0;
      virtual void act(Variable &e) = 0;
      virtual void act(Binary &e) = 0;
      virtual void act(Unary &e) = 0;
      virtual void act(Index &e) = 0;
      virtual void act(Call &e) = 0;
      virtual void act(NewArray &e) = 0;

      virtual void act(Block &s) = 0;
      virtual void act(Declaration &s) = 0;
      virtual void act(Assign &s) = 0;
      virtual void act(ExpressionStatement &s) = 0;
      virtual void act(If &s) = 0;
      virtual void act(While &s) = 0;
      virtual void act(For &s) = 0;
      virtual void act(Break &s) = 0;
      virtual void act(Continue &s) = 0;
      virtual void act(Return &s) = 0;
  };

}
