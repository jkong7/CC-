#include <sstream> 

#include <IR.h>
#include <helper.h> 
#include <behavior.h> 

namespace IR {


// Items 

Number::Number (int64_t n)
  : number_ {n}{
    return ; 
  }


Label::Label (const std::string &s)
  : label_ {s} {
    return; 
  }

Func::Func (const std::string &s)
  : function_label_ {s} {
    return; 
  }

Variable::Variable(const std::string &s)
  : var_ {s} {
    return; 
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


std::string Number::emit() const {
  return std::to_string(number_); 
}

std::string Label::emit() const {
  return ":" + label_.substr(1); 
}

std::string Func::emit() const {
  return "@" + function_label_.substr(1);
}

std::string Variable::emit() const {
  return var_; 
}



// Instruction constructors 
Instruction_initialize::Instruction_initialize(Variable* var)
  : var_{var} {
  return;
}

Instruction_assignment::Instruction_assignment(Variable* dst, Item* src)
  : dst_{dst},
    src_{src} {
  return;
}

Instruction_op::Instruction_op(Variable* dst, Item* lhs, OP op, Item* rhs)
  : dst_{dst},
    lhs_{lhs},
    op_{op},
    rhs_{rhs} {
  return;
}

Instruction_index_load::Instruction_index_load(Variable* dst, Variable* src, std::vector<Item*> indexes)
  : dst_{dst},
    src_{src},
    indexes_{std::move(indexes)} {
  return;
}

Instruction_index_store::Instruction_index_store(Variable* dst, std::vector<Item*> indexes, Item* src)
  : dst_{dst},
    indexes_{std::move(indexes)},
    src_{src} {
  return;
}

Instruction_length_t::Instruction_length_t(Variable* dst, Variable* src, Item* t) 
  : dst_{dst},
    src_{src},
    t_{t} {
  return;
}

Instruction_length::Instruction_length(Variable* dst, Variable* src)
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

Instruction_call_assignment::Instruction_call_assignment(Variable* dst, CallType c, Item* callee, std::vector<Item*> args)
  : dst_{dst},
    c_{c},
    callee_{callee},
    args_{std::move(args)} {
  return;
}

Instruction_new_array::Instruction_new_array(Variable* dst, std::vector<Item*> args)
  : dst_{dst},
    args_{std::move(args)} {
  return;
}

Instruction_new_tuple::Instruction_new_tuple(Variable* dst, Item* t)
  : dst_{dst},
    t_{t} {
  return;
}

Instruction_break_uncond::Instruction_break_uncond(Label* label)
  : label_{label} {
  return;
}

Instruction_break_cond::Instruction_break_cond(Item* t, Label* label1, Label* label2)
  : t_{t},
    label1_{label1},
    label2_{label2} {
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

void Instruction_initialize::accept(Behavior& b) {
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

void Instruction_break_uncond::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_break_cond::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_return::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_return_t::accept(Behavior& b) {
  b.act(*this);
}

void Function::fill_succs() {
  for (auto* bb : basic_blocks) {
    label_to_bb[bb->label_->label_] = bb;
  }
  for (auto* bb : basic_blocks) {
    bb->succs.clear();
    for (auto& label : bb->succ_labels) {
      bb->succs.push_back(label_to_bb.at(label));
    }
  }
}

void Program::linearize_bb() {
  for (auto* f : functions) {
    f->fill_succs(); 
    std::unordered_set<BasicBlock*> unmarked(f->basic_blocks.begin(), f->basic_blocks.end()); // blocks initially unmarked 
    std::vector<BasicBlock*> linearized; 
    for (auto* start : f->basic_blocks) {
      if (!unmarked.count(start)) continue; 
      BasicBlock* bb = start; 

      while (bb && unmarked.count(bb)) {
        unmarked.erase(bb); 
        linearized.push_back(bb);

        BasicBlock* next = nullptr; 
        for (auto* s : bb->succs) {
          if (unmarked.count(s)) {
            next = s; 
            break; // Simple heuristic: select first succ
          }
        }
        bb = next; 
      }
    }
    f->basic_blocks = std::move(linearized);
  }
}
}

