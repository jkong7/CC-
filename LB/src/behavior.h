#pragma once

namespace LB {
  class Program; 
  class Function; 

  class Instruction_declare; 
  class Instruction_assignment; 
  class Instruction_op; 

  class Instruction_index_load; 
  class Instruction_index_store; 

  class Instruction_length_t; 
  class Instruction_length; 

  class Instruction_call; 
  class Instruction_call_assignment; 

  class Instruction_new_array; 
  class Instruction_new_tuple; 

  class Instruction_label; 
  class Instruction_goto; 
  class Instruction_if; 
  class Instruction_while; 
  class Instruction_continue; 
  class Instruction_break; 
  class Instruction_scope; 

  class Instruction_return_t;
  class Instruction_return; 

  class Behavior {
    public:
      virtual ~Behavior() = default;

      virtual void act(Program &p) = 0;
      virtual void act(Function &f) = 0;

      virtual void act(Instruction_declare &i) = 0;
      virtual void act(Instruction_assignment &i) = 0;
      virtual void act(Instruction_op &i) = 0;
      virtual void act(Instruction_index_load &i) = 0;
      virtual void act(Instruction_index_store &i) = 0;

      virtual void act(Instruction_length_t &i) = 0;
      virtual void act(Instruction_length &i) = 0;

      virtual void act(Instruction_call &i) = 0;
      virtual void act(Instruction_call_assignment &i) = 0;
      
      virtual void act(Instruction_new_array &i) = 0;
      virtual void act(Instruction_new_tuple &i) = 0;

      virtual void act(Instruction_label &i) = 0;
      virtual void act(Instruction_goto &i) = 0;
      virtual void act(Instruction_if &i) = 0;
      virtual void act(Instruction_while &i) = 0;
      virtual void act(Instruction_continue &i) = 0;
      virtual void act(Instruction_break &i) = 0;
      virtual void act(Instruction_scope &i) = 0;
      virtual void act(Instruction_return_t &i) = 0;
      virtual void act(Instruction_return &i) = 0;
  };

} 
