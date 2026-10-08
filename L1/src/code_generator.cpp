#include <string>
#include <iostream>
#include <fstream>

#include <code_generator.h>
#include <helper.h> 
#include <peephole.h>
#include <sstream>
#include <vector>

using namespace std;

namespace L1{
  CodeGenBehavior::CodeGenBehavior(std::ostream &out)
    : out (out) {
      return; 
    }
 
  void CodeGenBehavior::act(Program &p) {
    out << ".text\n";
    out << "  .globl go\n";
    out << "go:\n";
    out << "  pushq %rbx\n";
    out << "  pushq %rbp\n";
    out << "  pushq %r12\n";
    out << "  pushq %r13\n";
    out << "  pushq %r14\n";
    out << "  pushq %r15\n";
    out << "  call _" << p.entryPointLabel.substr(1) << "\n";
    out << "  popq %r15\n";
    out << "  popq %r14\n";
    out << "  popq %r13\n";
    out << "  popq %r12\n";
    out << "  popq %rbp\n";
    out << "  popq %rbx\n";
    out << "  retq\n"; 
    for (Function* f: p.functions) {
      f->accept(*this);
    }
  }

  void CodeGenBehavior::act(Function& f) {
    out << "_" << f.name.substr(1) << ":" << "\n"; 
    int64_t localsSpace = f.locals * 8; 
    int64_t stackArgsSpace = std::max<int64_t>(0, f.arguments - 6) * 8; 
    if (localsSpace != 0) {
      out << "  subq " << "$" << localsSpace << ", " << "%rsp\n";
    }
    this -> cur_frame_size = localsSpace + stackArgsSpace; 
    for (Instruction* i: f.instructions) {
      i -> accept(*this); 
    }
  }

  void CodeGenBehavior::act(Instruction_assignment &i) {
    EmitOptions options; 
    options.memoryStoredLabel = true; 
    out << "  movq " << i.src()->emit(options) << ", " << i.dst()->emit() << "\n";
  }

  void CodeGenBehavior::act(Instruction_aop &i) {
    out << "  " << assembly_from_aop(i.aop()) << " " << i.rhs()->emit() << ", " << i.dst()->emit() << "\n";
  } 

  void CodeGenBehavior::act(Instruction_sop &i) {
    EmitOptions options; 
    options.eightBitRegister = true; 
    out << "  " << assembly_from_sop(i.sop()) << " "  << i.src()->emit(options) << ", " << i.dst()->emit() << "\n";
  } 

  void CodeGenBehavior::act(Instruction_mem_aop &i) {
    out << "  " << assembly_from_aop(i.aop()) << " " << i.rhs()->emit() << ", " << i.lhs()->emit() << "\n";
  } 

  void CodeGenBehavior::act(Instruction_cmp_assignment &i) {
    auto *lhs = dynamic_cast<const Number*>(i.lhs());
    auto *rhs = dynamic_cast<const Number*>(i.rhs()); 
    bool compileTimeCalculate = lhs != nullptr && rhs != nullptr; 
    if (compileTimeCalculate) {
      out << "  movq " << "$" << comp(lhs->value(), rhs->value(), i.cmp()) << ", " << i.dst()->emit() << "\n"; 
      return; 
    }

    bool flip = lhs != nullptr && rhs == nullptr; 
    std::string left = flip ? i.lhs()->emit() : i.rhs() -> emit(); 
    std::string right = flip ? i.rhs()->emit() : i.lhs()->emit();

    EmitOptions options; 
    options.eightBitRegister = true; 
    out << "  " << "cmpq " << left << ", " << right << "\n"; 
    out << "  " << assembly_from_cmp(i.cmp(), flip) << " " << i.dst()->emit(options) << "\n"; 
    out << "  " << "movzbq " << i.dst()->emit(options) << ", " << i.dst()->emit() << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_cjump &i) {
    auto *lhs = dynamic_cast<const Number*>(i.lhs());
    auto *rhs = dynamic_cast<const Number*>(i.rhs()); 
    bool compileTimeCalculate = lhs != nullptr && rhs != nullptr; 
    if (compileTimeCalculate) {
      if (comp(lhs->value(), rhs->value(), i.cmp())) {
        out << "  " << "jmp " << i.label()->emit() << "\n";
      }
      return; 
    }

    bool flip = lhs != nullptr && rhs == nullptr; 
    std::string left = flip ? i.lhs()->emit() : i.rhs() -> emit(); 
    std::string right = flip ? i.rhs()->emit() : i.lhs()->emit();

    out << "  " << "cmpq " << left << ", " << right << "\n"; 
    out << "  " << jump_assembly_from_cmp(i.cmp(), flip) << " " << i.label()->emit() << "\n";
  } 

  void CodeGenBehavior::act(Instruction_label &i) {
    out << "  " << i.label()->emit() << ":\n"; 
  } 

  void CodeGenBehavior::act(Instruction_goto &i) {
    out << "  jmp " << i.label()->emit() << "\n";
  } 

  void CodeGenBehavior::act(Instruction_ret &i) {
    if (cur_frame_size != 0) {
      out << "  addq " << "$" << cur_frame_size << ", " << "%rsp\n"; 
    }
    out << "  retq\n";
  } 

  void CodeGenBehavior::act(Instruction_call &i) {
    if (i.callType() == l1) {
      int64_t space = i.nArgs()->value() >= 6 ? (i.nArgs()->value() - 6) * 8 + 8 : 8; 
      if (space != 0) {
        out << "  subq " << "$" << space << ", " << "%rsp\n";
      }
      EmitOptions options; 
      options.functionCall = true;
      options.indirectRegCall = true; 
      out << "  jmp " << i.callee()->emit(options) << "\n"; 
    } else if (i.callType() == print) {
      out << "  call print\n"; 
    } else if (i.callType() == allocate) {
      out << "  call allocate\n"; 
    } else if (i.callType() == input) {
      out << "  call input\n"; 
    } else if (i.callType() == tuple_error) {
      out << "  call tuple_error\n"; 
    } else if (i.callType() == tensor_error) {
      if (i.nArgs()->value() == 1) {
        out << "  call array_tensor_error_null\n"; 
      } else if (i.nArgs()->value() == 3) {
        out << "  call array_error\n"; 
      } else if (i.nArgs()->value() == 4) {
        out << "  call tensor_error\n";
      }
    }
  } 

  void CodeGenBehavior::act(Instruction_reg_inc_dec &i) {
    out << "  " << assembly_from_inc_dec(i.op()) << " " << i.dst()->emit() << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_lea &i) {
    out << "  lea " << "(" << i.lhs()->emit() << ", " << i.rhs()->emit() << ", " << i.scale()->emit().substr(1) << "), " << i.dst()->emit() << "\n";
  } 


  void generate_code(Program p, int32_t optLevel){

    std::stringstream assembly;
    CodeGenBehavior b(assembly);
    p.accept(b); 

    std::vector<std::string> lines;
    std::string l;
    while (std::getline(assembly, l)) {
      lines.push_back(l);
    }
    if (optLevel > 0) {
      lines = peephole(lines);
    }

    std::ofstream outputFile;
    outputFile.open("prog.S");
    for (auto &line : lines) {
      outputFile << line << "\n";
    }
    outputFile.close();
   
    return ;
  }
}
