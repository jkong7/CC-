#include <spill.h> 

namespace L2 {
    SpillBehavior::SpillBehavior(const std::unordered_set<std::string> &spillInputs, size_t functionIndex, size_t tempCounter, size_t spillCounter) 
        : spillInputs(spillInputs), functionIndex(functionIndex), tempCounter(tempCounter), spillCounter(spillCounter) {
            for (const auto& v : spillInputs) {
                varOffsets[v] = spillCounter * 8; 
                spillCounter++; 
            }
            return;
        }

    void SpillBehavior::act(Program& p) {
        p.functions[functionIndex]->accept(*this); 
    }

    void SpillBehavior::act(Function& f) {
        for (const auto& i: f.instructions) {
            i->accept(*this);
        }
        f.instructions = newInstructions;
    }

    void SpillBehavior::act(Instruction_assignment& i) {
        Item* dst = i.dst(); 
        Item* src = i.src(); 
        if (dst->kind() == ItemType::MemoryItem) {
            auto *m = dynamic_cast<Memory*>(dst);
            Item* temp1 = read(m->getVar());
            Item* temp2 = read(src);

            Number* num = m->getOffset();
            auto mem = new Memory(temp1, num);

            auto ni = new Instruction_assignment(mem, temp2);
            newInstructions.push_back(ni); 
        } else if (src->kind() == ItemType::MemoryItem) {
            auto *m = dynamic_cast<Memory*>(src);
            Item* temp1 = read(m->getVar());

            Number* num = m->getOffset();
            auto mem = new Memory(temp1, num); 

            auto temp2 = newTemp();

            auto ni = new Instruction_assignment(temp2, mem); 

            newInstructions.push_back(ni); 

            write(dst, temp2); 
        } else {
            Item* temp = read(src);
            write(dst, temp);
        }
    }

    void SpillBehavior::act(Instruction_stack_arg_assignment &i) {
        Item* dst = i.dst(); 
        Item* src = i.src(); 
        if (dst->kind() == ItemType::VariableItem && spillInputs.count(dst->emit())) {
            auto temp = newTemp(); 
            auto ni = new Instruction_assignment(temp, src); 
            newInstructions.push_back(ni);
            write(dst, temp);  
        } else {
            auto ni = new Instruction_assignment(dst, src); 
            newInstructions.push_back(ni); 
        }
    }

    void SpillBehavior::act(Instruction_aop &i) { 
        Item* dst = i.dst(); 
        Item* rhs = i.rhs(); 
        AOP aop = i.aop(); 

        Item* dstTemp = read(dst); 
        Item* rhsTemp = read(rhs); 
        auto ni = new Instruction_aop(dstTemp, aop, rhsTemp);
        newInstructions.push_back(ni); 

        write(dst, dstTemp); 

    }
    
    void SpillBehavior::act(Instruction_sop &i) {
        Item* dst = i.dst(); 
        Item* src = i.src(); 
        SOP sop = i.sop(); 

        Item* dstTemp = read(dst); 
        Item* srcTemp = read(src); 
        auto ni = new Instruction_sop(dstTemp, sop, srcTemp);
        newInstructions.push_back(ni); 

        write(dst, dstTemp); 
    }

    void SpillBehavior::act(Instruction_mem_aop &i) {
        Item* lhs = i.lhs(); 
        Item* rhs = i.rhs(); 
        AOP aop = i.aop(); 
        Item* lhsTemp = read(lhs);
        Item* rhsTemp = read(rhs);

        auto ni = new Instruction_mem_aop(lhsTemp, aop, rhsTemp); 
        newInstructions.push_back(ni);
        if (rhs->kind() == ItemType::MemoryItem) {
            write(lhs, lhsTemp);
        }
    }

    void SpillBehavior::act(Instruction_cmp_assignment &i) {
        Item* dst = i.dst(); 
        Item* lhs = i.lhs(); 
        Item* rhs = i.rhs(); 
        CMP cmp = i.cmp(); 

        Item* lhsTemp = read(lhs); 
        Item* rhsTemp = read(rhs); 

        auto dstTemp = newTemp(); 

        auto ni = new Instruction_cmp_assignment(dstTemp, lhsTemp, cmp, rhsTemp);
        newInstructions.push_back(ni); 
        write(dst, dstTemp);
    }
    
    void SpillBehavior::act(Instruction_cjump &i) {
        Item* lhs = i.lhs(); 
        Item* rhs = i.rhs(); 
        Label* label = i.label(); 
        CMP cmp = i.cmp(); 

        Item* lhsTemp = read(lhs); 
        Item* rhsTemp = read(rhs); 

        auto ni = new Instruction_cjump(lhsTemp, cmp, rhsTemp, label); 
        newInstructions.push_back(ni); 
    }

    void SpillBehavior::act(Instruction_label &i) {
        Label* label = i.label(); 
        auto ni = new Instruction_label(label);
        newInstructions.push_back(ni); 
    }

    void SpillBehavior::act(Instruction_goto &i) {
        Label* label = i.label(); 
        auto ni = new Instruction_goto(label);
        newInstructions.push_back(ni);  
    }
    
    void SpillBehavior::act(Instruction_ret &i) {
        auto ni = new Instruction_ret(); 
        newInstructions.push_back(ni); 
    }

    void SpillBehavior::act(Instruction_call &i) {
        Item* callee = i.callee();
        Number* numArgs = i.nArgs();
        CallType ct = i.callType(); 
        if (ct == CallType::l1) { 
            Item* calleeTemp = read(callee); 
            auto ni = new Instruction_call(CallType::l1, calleeTemp, numArgs); 
            newInstructions.push_back(ni); 
        } else {
            auto ni = new Instruction_call(ct, callee, numArgs); 
            newInstructions.push_back(ni);
        }
    }

    void SpillBehavior::act(Instruction_reg_inc_dec &i) { 
        Item* dst = i.dst(); 
        IncDec op = i.op(); 
        auto dstTemp = read(dst); 
        auto ni = new Instruction_reg_inc_dec(dstTemp, op); 
        newInstructions.push_back(ni);
        write(dst, dstTemp);
    }
    
    void SpillBehavior::act(Instruction_lea &i) {
        Item* dst = i.dst(); 
        Item* lhs = i.lhs(); 
        Item* rhs = i.rhs(); 
        Number* scale = i.scale();

        auto lhsTemp = read(lhs); 
        auto rhsTemp = read(rhs); 

        auto dstTemp = newTemp(); 
        auto ni = new Instruction_lea(dstTemp, lhsTemp, rhsTemp, scale); 
        newInstructions.push_back(ni); 

        write(dst, dstTemp);
    }

    Item* SpillBehavior::newTemp() {
        std::ostringstream temp; 
        temp << "%S" << tempCounter; 
        tempCounter++; 
        std::string tempString = temp.str(); 
        Item* var = new Variable(tempString);
        return var; 
    }

    Item* SpillBehavior::read(Item* src) {
        Item* var; 
        if (src->kind() == ItemType::MemoryItem) {
            auto* m = dynamic_cast<const Memory*>(src); 
            Item* temp = read(m->getVar());
            var = new Memory(temp, m->getOffset()); 
        } else if (src->kind() == ItemType::VariableItem && spillInputs.count(src->emit())) {
            std::string v = src->emit(); 

            var = newTemp();

            auto reg = new Register(RegisterID::rsp); 
            auto num = new Number(varOffsets[v]);
            auto mem = new Memory(reg, num); 

            auto i = new Instruction_assignment(var, mem);

            newInstructions.push_back(i);

        } else {
            var = src; 
        }
        return var; 
    }

    void SpillBehavior::write(Item* dst, Item* toWrite) {
        Instruction* i; 
        if (dst->kind() == ItemType::VariableItem && spillInputs.count(dst->emit())) {
            std::string v = dst->emit(); 

            auto reg = new Register(RegisterID::rsp); 
            auto num = new Number(varOffsets[v]); 
            auto mem = new Memory(reg, num); 

            i = new Instruction_assignment(mem, toWrite);
        } else {
            i = new Instruction_assignment(dst, toWrite); 
        }
        newInstructions.push_back(i); 
    }

    std::tuple<size_t, size_t> spill(Program& p, const std::unordered_set<std::string> &spillInputs, size_t functionIndex, size_t tempCounter, size_t spillCounter) {
        SpillBehavior sb(spillInputs, functionIndex, tempCounter, spillCounter); 
        p.accept(sb); 
        return {sb.tempCounter, sb.spillCounter}; 
    }
}