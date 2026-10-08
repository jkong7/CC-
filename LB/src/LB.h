#pragma once

#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <iostream>



namespace LB {
  struct Behavior;

  // Enums

  enum OP {plus, minus, times, at, left_shift, right_shift, less_than, less_than_equal, equal, greater_than_equal, greater_than};

  enum CallType {lb, print, input, tuple_error, tensor_error};

  enum Type {void_, tuple, code, int64};


  // Items

  enum ItemType { NumberItem, LabelItem, NameItem };

  class Item {
    public:
      virtual ~Item() = default;
      virtual std::string emit() const = 0;
      virtual ItemType kind() const = 0;
  };


  class Number : public Item {
    public:
      Number (int64_t n);
      std::string emit() const override;
      ItemType kind() const override;

      int64_t number_;
  };

  class Label : public Item {
    public:
      Label (const std::string &s);
      std::string emit() const override;
      ItemType kind() const override;

      std::string label_;
  };

  class Name : public Item {
    public:
      Name (const std::string &s);
      std::string emit() const override;
      ItemType kind() const override;

      std::string name_;
  };


  struct VarType {
    Type type;
    int64_t dims;
  };


  /*
   * Instruction interface.
   */
  class Instruction{
    public:
      virtual ~Instruction() = default;
      void virtual accept(Behavior& b) = 0;

      int64_t line_ = 0;
  };

  /*
   * Instructions.
   */

  class Instruction_declare : public Instruction {
  public:
    Instruction_declare(VarType type, std::vector<Name*> vars);
    void accept(Behavior& b) override;

    VarType type_;
    std::vector<Name*> vars_;
  };

  class Instruction_assignment : public Instruction {
    public:
      Instruction_assignment(Name* dst, Item* src);
      void accept(Behavior& b) override;

      Name* dst_;
      Item* src_;
  };

  class Instruction_op : public Instruction {
  public:
    Instruction_op(Name* dst, Item* lhs, OP op, Item* rhs);
    void accept(Behavior& b) override;

    Name* dst_;
    Item* lhs_;
    OP op_;
    Item* rhs_;
  };

  class Instruction_index_load : public Instruction {
  public:
    Instruction_index_load(Name* dst, Name* src, std::vector<Item*> indexes);
    void accept(Behavior& b) override;

    Name* dst_;
    Name* src_;
    std::vector<Item*> indexes_;
  };

  class Instruction_index_store : public Instruction {
  public:
    Instruction_index_store(Name* dst, std::vector<Item*> indexes, Item* src);
    void accept(Behavior& b) override;

    Name* dst_;
    std::vector<Item*> indexes_;
    Item* src_;
  };

  class Instruction_length_t : public Instruction {
  public:
    Instruction_length_t(Name* dst, Name* src, Item* t);
    void accept(Behavior& b) override;

    Name* dst_;
    Name* src_;
    Item* t_;
  };

  class Instruction_length: public Instruction {
  public:
    Instruction_length(Name* dst, Name* src);
    void accept(Behavior& b) override;

    Name* dst_;
    Name* src_;
  };

  class Instruction_call : public Instruction {
  public:
    Instruction_call(CallType c, Item* callee, std::vector<Item*> args);
    void accept(Behavior& b) override;

    CallType c_;
    Item* callee_;
    std::vector<Item*> args_;
  };

  class Instruction_call_assignment : public Instruction {
  public:
    Instruction_call_assignment(Name* dst, CallType c, Item* callee, std::vector<Item*> args);
    void accept(Behavior& b) override;

    Name* dst_;
    CallType c_;
    Item* callee_;
    std::vector<Item*> args_;
  };

  class Instruction_new_array : public Instruction {
  public:
    Instruction_new_array(Name* dst, std::vector<Item*> args);
    void accept(Behavior& b) override;

    Name* dst_;
    std::vector<Item*> args_;
  };

  class Instruction_new_tuple : public Instruction {
  public:
    Instruction_new_tuple(Name* dst, Item* t);
    void accept(Behavior& b) override;

    Name* dst_;
    Item* t_;
  };

  class Instruction_label : public Instruction {
  public:
    Instruction_label(Label* label);
    void accept(Behavior& b) override;

    Label* label_;
  };

  class Instruction_goto : public Instruction {
  public:
    Instruction_goto(Label* label);
    void accept(Behavior& b) override;

    Label* label_;
  };

  class Instruction_if : public Instruction {
  public:
    Instruction_if(Item* lhs, OP op, Item* rhs, Label* label1, Label* label2);
    void accept(Behavior& b) override;

    Item* lhs_;
    OP op_;
    Item* rhs_;
    Label* label1_;
    Label* label2_;
  };

  class Instruction_while : public Instruction {
  public:
    Instruction_while(Item* lhs, OP op, Item* rhs, Label* label1, Label* label2);
    void accept(Behavior& b) override;

    Item* lhs_;
    OP op_;
    Item* rhs_;
    Label* label1_;
    Label* label2_;
    Label* cond_label_ = nullptr;
  };

  class Instruction_continue : public Instruction {
  public:
    Instruction_continue();
    void accept(Behavior& b) override;

    Instruction_while* loop_ = nullptr;
  };

  class Instruction_break : public Instruction {
  public:
    Instruction_break();
    void accept(Behavior& b) override;

    Instruction_while* loop_ = nullptr;
  };

  class Instruction_scope : public Instruction {
  public:
    Instruction_scope(std::vector<Instruction*> instructions);
    void accept(Behavior& b) override;

    std::vector<Instruction*> instructions_;
  };

  class Instruction_return : public Instruction {
  public:
    Instruction_return();
    void accept(Behavior& b) override;
  };

  class Instruction_return_t : public Instruction {
  public:
    Instruction_return_t(Item* t);
    void accept(Behavior& b) override;

    Item* t_;
  };


  class Function{
    public:
      VarType return_type;
      std::string name;
      std::vector<Name*> params;
      std::vector<VarType> param_types;
      Instruction_scope* body = nullptr;
      std::vector<Instruction*> instructions;
      std::unordered_map<std::string, VarType> variable_types;

      void accept(Behavior& b);
  };

  class Program{
    public:
      std::vector<Function *> functions;

      void accept(Behavior& b);
      bool has_function(const std::string &name) const;
  };

}
