#pragma once 

#include <algorithm> 
#include <iterator> 
#include <unordered_map> 
#include <unordered_set> 
#include <vector> 
#include <sstream> 
#include <tuple> 
#include <liveness_analysis.h>
#include <L2.h>


namespace L2{


    class SpillBehavior: public Behavior {
        public: 
            explicit SpillBehavior(const std::unordered_set<std::string> &spillInputs, size_t functionIndex, size_t tempCounter, size_t spillCounter); 
            void act(Program& p) override; 
            void act(Function &f) override; 
            void act(Instruction_assignment &i) override; 
            virtual void act(Instruction_stack_arg_assignment &i) override; 
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

            Item* newTemp();
            Item* read(Item* src);
            void write(Item* dst, Item* toWrite); 

            size_t spillCounter; 
            size_t tempCounter; 
        private:  
            std::unordered_set<std::string> spillInputs; 
            std::unordered_map<std::string, size_t> varOffsets; 
            size_t functionIndex; 
            
            std::vector<Instruction*> newInstructions;
    };

    std::tuple<size_t, size_t> spill(Program &p, const std::unordered_set<std::string> &spillInputs, size_t functionIndex, size_t tempCounter, size_t spillCounter); 
}