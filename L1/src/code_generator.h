#pragma once

#include <L1.h>

// Base visitor class with all the visit declarations 
// Then concrete visitor class with all the visit definitions (like CodeGenVisitor ex)

// For polymorphic classes, have the base class (Item + Instruction) have a virtual accept function, with virtual destructor default

// Each specific class implements accept(Visitor* v), which calls v.visit(*this), (v can be any concrete visitor)

// main calls p.accept(v)

// virtual -> runtime dispatch and =0 means pure virtual (no implementation here and class can't be made into object, considered 
// abstract class now), every derived class HAS to implement method 

// virtual destructor guarantees derived class destructor runs even though deletion is through base ptr
// (Runs ~Derived to clean up derived resources and ~Base to clean base resources)


namespace L1{

  class Behavior {
    public: 
      virtual ~Behavior() = default; 
      virtual void act(Program &p) = 0; 
      virtual void act(Function &f) = 0;
      virtual void act(Instruction_assignment &i) = 0;
      virtual void act(Instruction_aop &i) = 0;
      virtual void act(Instruction_sop &i) = 0;
      virtual void act(Instruction_mem_aop &i) = 0;
      virtual void act(Instruction_cmp_assignment &i) = 0;
      virtual void act(Instruction_cjump &i) = 0;
      virtual void act(Instruction_label &i) = 0;
      virtual void act(Instruction_goto &i) = 0;
      virtual void act(Instruction_ret &i) = 0;
      virtual void act(Instruction_call &i) = 0;
      virtual void act(Instruction_reg_inc_dec &i) = 0;
      virtual void act(Instruction_lea &i) = 0;
  };

  class CodeGenBehavior : public Behavior {
    public:
      explicit CodeGenBehavior(std::ostream &out);
      void act(Program &p) override; 
      void act(Function &f) override; 
      void act(Instruction_assignment &i) override; 
      virtual void act(Instruction_aop &i) override; 
      virtual void act(Instruction_sop &i) override; 
      virtual void act(Instruction_mem_aop &i) override; 
      virtual void act(Instruction_cmp_assignment &i) override; 
      virtual void act(Instruction_cjump &i) override; 
      virtual void act(Instruction_label &i) override; 
      virtual void act(Instruction_goto &i) override; 
      virtual void act(Instruction_ret &i) override; 
      virtual void act(Instruction_call &i) override; 
      virtual void act(Instruction_reg_inc_dec &i) override; 
      virtual void act(Instruction_lea &i) override; 

    private:
      int64_t cur_frame_size; 
      std::ostream &out; 
  };

  void generate_code(Program p, int32_t optLevel = 0);

}
