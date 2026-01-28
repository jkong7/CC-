#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>




namespace L2 {
  struct Behavior; 

  // Enums 

  // Consider making enum classes
  enum RegisterID {rdi, rsi, rdx, rcx, r8, r9, rax, rbx, rbp, r10, r11, r12, r13, r14, r15, rsp};

  enum AOP {plus_equal, minus_equal, times_equal, and_equal}; 

  enum SOP {left_shift, right_shift}; 

  enum CMP {less_than, less_than_equal, equal}; 

  enum IncDec {increment, decrement}; 

  enum CallType {l1, print, input, allocate, tuple_error, tensor_error}; 


  // Items 

  enum ItemType { RegisterItem, NumberItem, LabelItem, FuncItem, VariableItem, StackArgItem, MemoryItem }; 

  struct EmitOptions {
    bool eightBitRegister = false; 
    bool memoryStoredLabel = false; 
    bool functionCall = false; 
    bool indirectRegCall = false; 
    bool livenessAnalysis = false; 
  }; 

  class Item {
    public: 
      virtual ~Item() = default; 
      virtual std::string emit(const EmitOptions& options = EmitOptions{}) const = 0; 
      virtual ItemType kind() const = 0; 
  };

  class Register : public Item {
    public:
      Register (RegisterID r);
      std::string emit(const EmitOptions& options = EmitOptions{}) const override;
      ItemType kind() const override; 

    private:
      RegisterID ID;
  };

  class Number : public Item {
    public:
      Number (int64_t n); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override;
      int64_t value() const; 
      ItemType kind() const override; 

    private: 
      int64_t number; 

  }; 

  class Label : public Item {
    public: 
      Label (const std::string &s); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override;
      ItemType kind() const override; 

    private: 
      std::string label; 
  }; 

  class Func : public Item {
    public: 
      Func (const std::string &s); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override;
      ItemType kind() const override; 

    private: 
      std::string function_label; 
  }; 

  class Variable : public Item {
    public: 
      Variable (const std::string &s); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override; 
      ItemType kind() const override; 

    private: 
      std::string var; 
  }; 

  class StackArg : public Item {
    public: 
      StackArg (Number* n); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override; 
      ItemType kind() const override; 

    private: 
      Number* offset; 
  };

  class Memory : public Item {
    public: 
      Memory (Item *v, Number *n); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override;
      ItemType kind() const override; 

      Item* getVar(); 
      Number* getOffset(); 

    private: 
      Item *var; 
      Number *offset; 
  };





  /*
   * Instruction interface.
   */
  class Instruction{
    public: 
      virtual ~Instruction() = default; 
      void virtual accept(Behavior& b) = 0; 
  };

  /*
   * Instructions.
   */

  class Instruction_assignment : public Instruction {
  public:
    Instruction_assignment(Item* dst, Item* src);
    void accept(Behavior& b) override;

    const Item* src() const;
    const Item* dst() const;

  private:
    Item* src_;
    Item* dst_;
  };

  class Instruction_stack_arg_assignment : public Instruction {
    public:
      Instruction_stack_arg_assignment(Item* dst, StackArg* src);
      void accept(Behavior& b) override; 

      const Item* dst() const; 
      const StackArg* src() const; 

    private: 
      Item* dst_; 
      StackArg* src_; 
  }; 


  class Instruction_aop : public Instruction {
  public:
    Instruction_aop(Item* dst, AOP a, Item* rhs);
    void accept(Behavior& b) override;

    const Item* dst() const;
    AOP aop() const;
    const Item* rhs() const;

  private:
    Item* dst_;
    AOP aop_;
    Item* rhs_;
  };


  class Instruction_sop : public Instruction {
  public:
    Instruction_sop(Item* dst, SOP s, Item* src);
    void accept(Behavior& b) override;

    const Item* dst() const;
    SOP sop() const;
    const Item* src() const;

  private:
    Item* dst_;
    SOP sop_;
    Item* src_;
  };


  class Instruction_mem_aop : public Instruction {
  public:
    Instruction_mem_aop(Item* lhs, AOP a, Item* rhs);
    void accept(Behavior& b) override;

    const Item* lhs() const;
    AOP aop() const;
    const Item* rhs() const;

  private:
    Item* lhs_;
    AOP aop_;
    Item* rhs_;
  };


  class Instruction_cmp_assignment : public Instruction {
  public:
    Instruction_cmp_assignment(Item* dst, Item* lhs, CMP c, Item* rhs);
    void accept(Behavior& b) override;

    const Item* dst() const;
    const Item* lhs() const;
    CMP cmp() const;
    const Item* rhs() const;

  private:
    Item* dst_;
    Item* lhs_;
    CMP cmp_;
    Item* rhs_;
  };


  class Instruction_cjump : public Instruction {
  public:
    Instruction_cjump(Item* lhs, CMP c, Item* rhs, Label* l);
    void accept(Behavior& b) override;

    const Item* lhs() const;
    CMP cmp() const;
    const Item* rhs() const;
    const Label* label() const;

  private:
    Item* lhs_;
    CMP cmp_;
    Item* rhs_;
    Label* label_;
  };


  class Instruction_label : public Instruction {
  public:
    Instruction_label(Label* l);
    void accept(Behavior& b) override;

    const Label* label() const;

  private:
    Label* label_;
  };


  class Instruction_goto : public Instruction {
  public:
    Instruction_goto(Label* l);
    void accept(Behavior& b) override;

    const Label* label() const;

  private:
    Label* label_;
  };


  class Instruction_ret : public Instruction {
  public:
    void accept(Behavior& b) override;
  };


  class Instruction_call : public Instruction {
  public:
    Instruction_call(CallType c, Item* callee, Number* n);
    void accept(Behavior& b) override;

    CallType callType() const;
    const Item* callee() const;
    const Number* nArgs() const;

  private:
    CallType callType_;
    Item* callee_;
    Number* nArgs_;
  };


  class Instruction_reg_inc_dec : public Instruction {
  public:
    Instruction_reg_inc_dec(Item* r, IncDec i);
    void accept(Behavior& b) override;

    const Item* dst() const;
    IncDec op() const;

  private:
    Item* dst_;
    IncDec op_;
  };


  class Instruction_lea : public Instruction {
  public:
    Instruction_lea(Item* dst, Item* lhs, Item* rhs, Number* n);
    void accept(Behavior& b) override;

    const Item* dst() const;
    const Item* lhs() const;
    const Item* rhs() const;
    const Number* scale() const;

  private:
    Item* dst_;
    Item* lhs_;
    Item* rhs_;
    Number* scale_;
  };


    /*
    * Function.
    */
    class Function{
      public:
        std::string name;
        int64_t arguments;
        std::vector<Instruction *> instructions;

        void accept(Behavior& b);

    };

    class Program{
      public:
        std::string entryPointLabel;
        std::vector<Function *> functions;
        
        void accept(Behavior& b); 
    };

}
