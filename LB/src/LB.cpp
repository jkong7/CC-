#include <LB.h>
#include <behavior.h> 

namespace LB {


// Items 

Number::Number (int64_t n)
  : number_ {n}{
    return ; 
  }

Label::Label (const std::string &s)
  : label_ {s} {
    return; 
  }

Name::Name (const std::string &s)
  : name_ {s} {
    return; 
  }


ItemType Number::kind() const {
  return ItemType::NumberItem; 
}

ItemType Label::kind() const {
  return ItemType::LabelItem; 
}

ItemType Name::kind() const {
  return ItemType::NameItem; 
}


std::string Number::emit() const {
  return std::to_string(number_); 
}

std::string Label::emit() const {
  return label_; 
}

std::string Name::emit() const {
  return name_; 
}



// Instruction constructors 

Instruction_declare::Instruction_declare(VarType type, std::vector<Name*> vars)
  : type_{type},
    vars_{std::move(vars)} {
  return;
}

Instruction_assignment::Instruction_assignment(Name* dst, Item* src)
  : dst_{dst},
    src_{src} {
  return;
}

Instruction_op::Instruction_op(Name* dst, Item* lhs, OP op, Item* rhs)
  : dst_{dst},
    lhs_{lhs},
    op_{op},
    rhs_{rhs} {
  return;
}

Instruction_index_load::Instruction_index_load(Name* dst, Name* src, std::vector<Item*> indexes)
  : dst_{dst},
    src_{src},
    indexes_{std::move(indexes)} {
  return;
}

Instruction_index_store::Instruction_index_store(Name* dst, std::vector<Item*> indexes, Item* src)
  : dst_{dst},
    indexes_{std::move(indexes)},
    src_{src} {
  return;
}

Instruction_length_t::Instruction_length_t(Name* dst, Name* src, Item* t) 
  : dst_{dst},
    src_{src},
    t_{t} {
  return;
}

Instruction_length::Instruction_length(Name* dst, Name* src)
  : dst_{dst},
    src_{src} {
  return;
}

Instruction_call::Instruction_call(CallType c, Item* callee, std::vector<Item*> args)
  : c_{c},
    callee_{callee},
    args_{std::move(args)} {
  return;
}

Instruction_call_assignment::Instruction_call_assignment(Name* dst, CallType c, Item* callee, std::vector<Item*> args)
  : dst_{dst},
    c_{c},
    callee_{callee},
    args_{std::move(args)} {
  return;
}

Instruction_new_array::Instruction_new_array(Name* dst, std::vector<Item*> args)
  : dst_{dst},
    args_{std::move(args)} {
  return;
}

Instruction_new_tuple::Instruction_new_tuple(Name* dst, Item* t)
  : dst_{dst},
    t_{t} {
  return;
}

Instruction_label::Instruction_label(Label* label)
  : label_{label} {
  return;
}

Instruction_goto::Instruction_goto(Label* label)
  : label_{label} {
  return;
}

Instruction_if::Instruction_if(Item* lhs, OP op, Item* rhs, Label* label1, Label* label2)
  : lhs_{lhs},
    op_{op},
    rhs_{rhs},
    label1_{label1},
    label2_{label2} {
  return;
}

Instruction_while::Instruction_while(Item* lhs, OP op, Item* rhs, Label* label1, Label* label2)
  : lhs_{lhs},
    op_{op},
    rhs_{rhs},
    label1_{label1},
    label2_{label2} {
  return;
}

Instruction_continue::Instruction_continue()
{
  return; 
}

Instruction_break::Instruction_break()
{
  return; 
}

Instruction_scope::Instruction_scope(std::vector<Instruction*> instructions)
  : instructions_{std::move(instructions)} {
  return;
}

Instruction_return::Instruction_return()
{
  return; 
}

Instruction_return_t::Instruction_return_t(Item* t)
  : t_{t} {
  return;
}



// Visitor 

void Program::accept(Behavior& b) {
  b.act(*this);
}

void Function::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_declare::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_assignment::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_op::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_index_load::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_index_store::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_length_t::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_length::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_call::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_call_assignment::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_new_array::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_new_tuple::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_label::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_goto::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_if::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_while::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_continue::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_break::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_scope::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_return::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_return_t::accept(Behavior& b) {
  b.act(*this);
}

bool Program::has_function(const std::string &name) const {
  for (auto* f : functions) {
    if (f->name == name) return true; 
  }
  return false; 
}

}
