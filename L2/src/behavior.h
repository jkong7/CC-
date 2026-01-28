#pragma once

#include <L2.h>

namespace L2{

  class Behavior {
    public: 
      virtual ~Behavior() = default; 
      virtual void act(Program &p) = 0; 
      virtual void act(Function &f) = 0;
      virtual void act(Instruction_assignment &i) = 0;
      virtual void act(Instruction_stack_arg_assignment &i) = 0; 
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
}