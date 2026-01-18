#include <sstream> 

#include <L2.h>
#include <liveness_analysis.h> 
#include <helper.h> 

namespace L2 {


Register::Register (RegisterID r)
  : ID {r}{
  return ;
}

Number::Number (int64_t n)
  : number {n}{
    return ; 
  }
  
int64_t Number::value() const {
  return number; 
}

Label::Label (const std::string &s)
  : label {s} {
    return; 
  }

Func::Func (const std::string &s)
  : function_label {s} {
    return; 
  }

Variable::Variable(const std::string &s)
  : var (s) {
    return; 
  }

StackArg::StackArg(Number* n)
  : offset (n) {
    return; 
  }

Memory::Memory (Item *v, Number *n)
  : var {r}, offset {n} {
    return; 
  }

ItemType Register::kind () const {
  return ItemType::RegisterItem; 
}

ItemType Number::kind() const {
  return ItemType::NumberItem; 
}

ItemType Label::kind() const {
  return ItemType::LabelItem; 
}

ItemType Func::kind() const {
  return ItemType::FuncItem; 
}

ItemType Variable::kind() const {
  return ItemType::VariableItem; 
}

ItemType StackArg::kind() const {
  return ItemType::StackArgItem; 
}

ItemType Memory::kind() const {
  return ItemType::MemoryItem; 
}

std::string Register::emit (const EmitOptions& options) const {
  std::ostringstream s; 
  std::string reg = options.eightBitRegister ? eightBitReg_assembly_from_register(ID) : options.indirectRegCall ? indirect_call_reg_assembly_from_register(ID) : options.livenessAnalysis ? string_from_register(ID) : assembly_from_register(ID); 
  s << reg; 
  return s.str(); 
}

std::string Number::emit(const EmitOptions& options) const {
  std::string nString = std::to_string(number); 
  std::ostringstream s; 
  s << "$" << nString;
  return s.str(); 
}

std::string Label::emit(const EmitOptions& options) const {
  std::string lname = label.substr(1); 
  std::ostringstream s; 
  std::string prefix = options.memoryStoredLabel ? "$_" : "_"; 
  s << prefix << lname;
  return s.str(); 
}

std::string Func::emit(const EmitOptions& options) const {
  std::string fname = function_label.substr(1); 
  std::ostringstream s; 
  if (options.functionCall) {
    s << "_" << fname; 
  } else {
    s << "$_" << fname; 
  }
  return s.str(); 
}

std::string Variable::emit(const EmitOptions& options) const {
  return var; 
}

std::string StackArg::emit(const EmitOptions& options) const {
  return "";
}

std::string Memory::emit(const EmitOptions& options) const {
  if (options.livenessAnalysis) {
    return reg->emit(options); 
  }
  
  std::string offsetString = offset->emit().substr(1);
  std::string regString = reg->emit();
  
  std::ostringstream s; 
  s << offsetString << "(" << regString << ")"; 
  return s.str(); 
}



Instruction_assignment::Instruction_assignment (Item *dst, Item *src)
  : src_ { src },
    dst_ { dst } {
  return ;
}

Instruction_stack_arg_assignment::Instruction_stack_arg_assignment (Item *dst, StackArg* src)
  : dst_ {dst}, 
    src_ {src} {
      return;
    }

Instruction_aop::Instruction_aop (Item *dst, AOP a, Item *rhs)
  : dst_ {dst}, 
    aop_ {a},
    rhs_ {rhs} {
      return ;
    }

Instruction_sop::Instruction_sop (Item *dst, SOP s, Item *src)
  : dst_ {dst}, 
    sop_ {s},
    src_ {src} {
      return ;
    }


Instruction_mem_aop::Instruction_mem_aop(Item *lhs, AOP a, Item *rhs)
  : lhs_{lhs},
    aop_{a},
    rhs_{rhs} {
  return;
}

Instruction_cmp_assignment::Instruction_cmp_assignment(Item *dst, Item *lhs, CMP c, Item *rhs)
  : dst_{dst},
    lhs_{lhs},
    cmp_{c},
    rhs_{rhs} {
  return;
}

Instruction_cjump::Instruction_cjump(Item *lhs, CMP c, Item *rhs, Label *l)
  : lhs_{lhs},
    cmp_{c},
    rhs_{rhs},
    label_{l} {
  return;
}

Instruction_label::Instruction_label(Label *l)
  : label_{l} {
  return;
}

Instruction_goto::Instruction_goto(Label *l)
  : label_{l} {
  return;
}

Instruction_call::Instruction_call(CallType ct, Item *callee, Number *n)
  : callType_{ct},
    callee_{callee},
    nArgs_{n} {
  return;
}

Instruction_reg_inc_dec::Instruction_reg_inc_dec(Item *r, IncDec i)
  : dst_{r},
    op_{i} {
  return;
}

Instruction_lea::Instruction_lea(Item *dst, Item *lhs, Item *rhs, Number *scale)
  : dst_{dst},
    lhs_{lhs},
    rhs_{rhs},
    scale_{scale} {
  return;
}

void Program::accept(Behavior& b) {
  b.act(*this);
}

void Function::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_assignment::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_stack_arg_assignment::accept(Behavior& b) {
  b.act(*this); 
}

void Instruction_aop::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_sop::accept(Behavior& b) {
  b.act(*this);
}


void Instruction_mem_aop::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_cmp_assignment::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_cjump::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_label::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_goto::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_ret::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_call::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_reg_inc_dec::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_lea::accept(Behavior& b) {
  b.act(*this);
}

const Item* Instruction_assignment::src() const { return src_; }
const Item* Instruction_assignment::dst() const { return dst_; }

const Item* Instruction_stack_arg_assignment::dst() const { return dst_; }
const StackArg* Instruction_stack_arg_assignment::src() const { return src_; }

const Item* Instruction_aop::dst() const { return dst_; }
AOP Instruction_aop::aop() const { return aop_; }
const Item* Instruction_aop::rhs() const { return rhs_; }

const Item* Instruction_sop::dst() const { return dst_; }
SOP Instruction_sop::sop() const { return sop_; }
const Item* Instruction_sop::src() const { return src_; }

const Item* Instruction_mem_aop::lhs() const { return lhs_; }
AOP Instruction_mem_aop::aop() const { return aop_; }
const Item* Instruction_mem_aop::rhs() const { return rhs_; }

const Item* Instruction_cmp_assignment::dst() const { return dst_; }
const Item* Instruction_cmp_assignment::lhs() const { return lhs_; }
CMP Instruction_cmp_assignment::cmp() const { return cmp_; }
const Item* Instruction_cmp_assignment::rhs() const { return rhs_; }

const Item* Instruction_cjump::lhs() const { return lhs_; }
CMP Instruction_cjump::cmp() const { return cmp_; }
const Item* Instruction_cjump::rhs() const { return rhs_; }
const Label* Instruction_cjump::label() const { return label_; }

const Label* Instruction_label::label() const { return label_; }

const Label* Instruction_goto::label() const { return label_; }

CallType Instruction_call::callType() const { return callType_; }
const Item* Instruction_call::callee() const { return callee_; }
const Number* Instruction_call::nArgs() const { return nArgs_; }

const Item* Instruction_reg_inc_dec::dst() const { return dst_; }
IncDec Instruction_reg_inc_dec::op() const { return op_; }

const Item* Instruction_lea::dst() const { return dst_; }
const Item* Instruction_lea::lhs() const { return lhs_; }
const Item* Instruction_lea::rhs() const { return rhs_; }
const Number* Instruction_lea::scale() const { return scale_; }

}

