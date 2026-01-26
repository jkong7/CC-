#include <string>
#include <iostream>
#include <fstream>

#include <liveness_analysis.h>


namespace L2{

    LivenessAnalysisBehavior::LivenessAnalysisBehavior(std::ostream &out)
    : out (out) {
      return; 
    }

    void LivenessAnalysisBehavior::act(Program& p) { 
        initialize_containers(p.functions.size()); 
        for (int i = 0; i < p.functions.size(); i++) {
            while (true) {
                clear_function_containers();
                p.functions[i]->accept(*this);
                generate_in_out_sets(p);
                generate_interference_graph(p); // add all keys (a key might not have any interference neighbors??)
                if (color_graph()) break; // so now we spilloutputs and coloroutputs for each function 
                spill(p, cur_f);  // do this with another concrete visitor, output of it creates new instruction list, set f -> inst to new list
            }
            cur_f=i; 
        }

        //print_liveness_tests();
        print_interference_tests();
        
    }

    void LivenessAnalysisBehavior::act(Function& f) {
        cur_i = 0; 
        livenessData[cur_f].resize(f.instructions.size());
        for (Instruction *i: f.instructions) {
            i->accept(*this); 
            cur_i++; 
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_assignment& i) {
        auto &ls = livenessData[cur_f][cur_i]; 
        const Item* dst = i.dst(); 
        const Item* src = i.src(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(src)) {
            ls.gen.insert(src->emit(options));
        }
        if (isLivenessContributor(dst)) {
            if (dst->kind() == ItemType::MemoryItem) {
                ls.gen.insert(dst->emit(options)); 
            } else {
                ls.kill.insert(dst->emit(options));
            } 
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_stack_arg_assignment& i) {
        auto &ls = livenessData[cur_f][cur_i]; 
        const Item* dst = i.dst(); 

        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(dst)) {
            ls.kill.insert(dst->emit(options));
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_aop& i) {
        auto &ls = livenessData[cur_f][cur_i]; 
        const Item* dst = i.dst(); 
        const Item* src = i.rhs(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 

        if (isLivenessContributor(src)) {
            ls.gen.insert(src->emit(options));
        }
        if (isLivenessContributor(dst)) {
            ls.gen.insert(dst->emit(options)); 
            ls.kill.insert(dst->emit(options)); 
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_sop& i) {
        auto &ls = livenessData[cur_f][cur_i]; 
        const Item* dst = i.dst(); 
        const Item* src = i.src(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 

        if (isLivenessContributor(src)) {
            ls.gen.insert(src->emit(options));
        }
        if (isLivenessContributor(dst)) {
            ls.gen.insert(dst->emit(options)); 
            ls.kill.insert(dst->emit(options));
        } 
    }
    
    void LivenessAnalysisBehavior::act(Instruction_mem_aop& i) {
        auto &ls = livenessData[cur_f][cur_i]; 
        const Item* lhs = i.lhs(); 
        const Item* rhs = i.rhs(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(lhs)) {
            ls.gen.insert(lhs->emit(options));
            if (lhs->kind() != ItemType::MemoryItem) {
                ls.kill.insert(lhs->emit(options)); 
            }
        }
        if (isLivenessContributor(rhs)) {
            ls.gen.insert(rhs->emit(options));
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_cmp_assignment& i) {
        auto &ls = livenessData[cur_f][cur_i]; 
        const Item* dst = i.dst(); 
        const Item* lhs = i.lhs(); 
        const Item* rhs = i.rhs(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(dst)) {
            ls.kill.insert(dst->emit(options));
        } 
        if (isLivenessContributor(lhs)) {
            ls.gen.insert(lhs->emit(options)); 
        }
        if (isLivenessContributor(rhs)) {
            ls.gen.insert(rhs->emit(options));
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_cjump& i) {
        auto &ls = livenessData[cur_f][cur_i]; 
        const Item* lhs = i.lhs(); 
        const Item* rhs = i.rhs(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(lhs)) {
            ls.gen.insert(lhs->emit(options)); 
        }
        if (isLivenessContributor(rhs)) {
            ls.gen.insert(rhs->emit(options));
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_label& i) {
        // empty gen + kill

        // Store instruction # -> label
        auto &lm = labelMap[cur_f]; 
        lm[i.label()->emit()] = cur_i;
    }

    void LivenessAnalysisBehavior::act(Instruction_goto& i) {
        // empty gen + kill
    }

    void LivenessAnalysisBehavior::act(Instruction_ret& i) {
        auto &ls = livenessData[cur_f][cur_i];
        std::unordered_set<std::string> callee_save_registers = {"r12", "r13", "r14", "r15", "rbp", "rbx"}; 
        ls.gen.insert("rax"); 
        ls.gen.insert(callee_save_registers.begin(), callee_save_registers.end()); 
    }

    void LivenessAnalysisBehavior::act(Instruction_call& i) {
        auto &ls = livenessData[cur_f][cur_i];
        std::unordered_set<std::string> caller_save_registers = {"r10", "r11", "r8", "r9", "rax", "rcx", "rdi", "rdx", "rsi"}; 
        ls.kill.insert(caller_save_registers.begin(), caller_save_registers.end());

        std::vector<std::string> argument_registers = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        if (i.callType() == CallType::l1) {
            const Item* callee = i.callee();
            if (isLivenessContributor(callee)) {
                EmitOptions options; 
                options.livenessAnalysis = true; 
                ls.gen.insert(callee->emit(options));
            }
        }

        int64_t num_args = i.nArgs()->value(); 
        for (int argIndex = 0; argIndex < std::min(num_args, static_cast<int64_t>(6)); argIndex++) {
            ls.gen.insert(argument_registers[argIndex]);
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_reg_inc_dec& i) {
        auto &ls = livenessData[cur_f][cur_i];
        const Item* dst = i.dst(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(dst)) {
            ls.gen.insert(dst->emit(options)); 
            ls.kill.insert(dst->emit(options));
        } 
    }

    void LivenessAnalysisBehavior::act(Instruction_lea& i) {
        auto &ls = livenessData[cur_f][cur_i]; 
        const Item* dst = i.dst();
        const Item* lhs = i.lhs();
        const Item* rhs = i.rhs();
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(lhs)) {
            ls.gen.insert(lhs->emit(options));
        }
        if (isLivenessContributor(rhs)) {
            ls.gen.insert(rhs->emit(options));
        }
        if (isLivenessContributor(dst)) {
            ls.kill.insert(dst->emit(options));
        }         
    }

    void LivenessAnalysisBehavior::initialize_containers(size_t n) {
        livenessData.resize(n); 
        labelMap.resize(n); 
        interferenceGraph.resize(n);
        nodeDegrees.resize(n); 
        removed_nodes.resize(n); 
        node_stack.resize(n); 
        spillOutputs.resize(n); 
        colorOutputs.resize(n);
    }

    void LivenessAnalysisBehavior::clear_function_containers() {
        livenessData[cur_f].clear();
        labelMap[cur_f].clear(); 
        interferenceGraph[cur_f].clear(); 
        nodeDegrees[cur_f].clear(); 
        removed_nodes[cur_f].clear(); 
        node_stack[cur_f].clear(); 
        spillOutputs[cur_f].clear(); 
        colorOutputs[cur_f].clear(); 
    }


    bool LivenessAnalysisBehavior::isLivenessContributor(const Item* var) {
        EmitOptions options; 
        options.livenessAnalysis = true; 
        return (var->kind() == ItemType::RegisterItem && var->emit() != "%rsp") || var->kind() == ItemType::VariableItem || (var->kind() == ItemType::MemoryItem && var->emit(options) != "rsp");
    }

    bool LivenessAnalysisBehavior::isNoSuccessorInstruction(const Instruction* i) {
        if (auto *inst = dynamic_cast<const Instruction_call*>(i)) {
            if (inst->callType() == CallType::tuple_error || inst->callType() == CallType::tensor_error) {
                return true;
            }
        }
        return dynamic_cast<const Instruction_ret*>(i);
    }


    void LivenessAnalysisBehavior::print_instruction_gen_kill(size_t cur_i, const livenessSets& ls) {
        std::cout << cur_i << " gen set: ";
        bool first = true;
        for (const auto &s : ls.gen) {
            if (!first) std::cout << ", ";
            std::cout << s;
            first = false;
        }
        std::cout << "\n";

        std::cout << cur_i << " kill set: ";
        first = true;
        for (const auto &s : ls.kill) {
            if (!first) std::cout << ", ";
            std::cout << s;
            first = false;
        }
        std::cout << "\n\n";  
    }

    void LivenessAnalysisBehavior::generate_in_out_sets(const Program &p) {
        bool change = true; 
        auto& functionInstructions = p.functions[cur_f]->instructions; 
        auto& functionLivenessData = livenessData[cur_f]; 
        auto& functionLabelMap = labelMap[cur_f]; 
        while (change) {
            change = false; 
            for (int j = (int)functionLivenessData.size()-1; j>=0; j--) {
                livenessSets& ls = functionLivenessData[j];
                std::unordered_set<std::string> original_in = ls.in; 
                std::unordered_set<std::string> original_out = ls.out; 
                Instruction* cur_instruction = functionInstructions[j];
                if (isNoSuccessorInstruction(cur_instruction)) {
                    // no successors, out is empty 
                } else if (auto *gt = dynamic_cast<const Instruction_goto*>(cur_instruction)) {
                    const std::string label = gt->label()->emit();
                    size_t label_instruction_index = functionLabelMap[label]; 
                    livenessSets& ls_label_instruction = functionLivenessData[label_instruction_index]; 
                    ls.out = ls_label_instruction.in;
                } else if (auto *cj = dynamic_cast<const Instruction_cjump*>(cur_instruction)) {
                    const std::string label = cj->label()->emit();
                    size_t label_instruction_index = functionLabelMap[label]; 
                    livenessSets& ls_label_instruction = functionLivenessData[label_instruction_index]; 
                    ls.out = ls_label_instruction.in;

                    livenessSets& ls_next_inst = functionLivenessData[j+1];
                    ls.out.insert(ls_next_inst.in.begin(), ls_next_inst.in.end()); 
                } else {
                    livenessSets& ls_next_inst = functionLivenessData[j+1]; 
                    ls.out = ls_next_inst.in; 
                }
                std::unordered_set out_kill_diff_set = set_difference(ls.out, ls.kill);
                ls.in = set_union(ls.gen, out_kill_diff_set);
                if (ls.in != original_in || ls.out != original_out) {
                    change = true;
                }
            }
        }
    }




    void LivenessAnalysisBehavior::generate_interference_graph(const Program &p) {
        auto& functionInterferenceGraph = interferenceGraph[cur_f];  
        auto& functionLivenessData = livenessData[cur_f]; 
        auto& functionInstructions = p.functions[cur_f]->instructions; 
        for (int j = 0; j<(int)functionLivenessData.size(); j++) {
            livenessSets& ls = functionLivenessData[j];
            Instruction* cur_instruction = functionInstructions[j]; 
            add_edges_to_graph(functionInterferenceGraph, ls.in, ls.in);
            add_edges_to_graph(functionInterferenceGraph, ls.out, ls.out);
            add_edges_to_graph(functionInterferenceGraph, ls.kill, ls.out); 
            add_edges_to_graph(functionInterferenceGraph, GPregisters, GPregisters);
            if (auto *shift = dynamic_cast<const Instruction_sop*>(cur_instruction)) {
                if (auto* n = dynamic_cast<const Number*>(shift->src())) {
                    continue;
                }
                EmitOptions options; 
                options.livenessAnalysis = true; 
                std::unordered_set<std::string> rcxVar = {shift->src()->emit(options)};
                add_edges_to_graph(functionInterferenceGraph, rcxVar, GPregisters_without_rcx); 
            }
        }
        for (const auto& [key, val] : functionInterferenceGraph) {
            nodeDegrees[cur_f][key] = val.size(); 
        }
    }
        

    std::string LivenessAnalysisBehavior::pick_low_node() {
        size_t best = 0; 
        std::string bestNode; 
        bool found = false; 
        for (const auto& [key, val] : nodeDegrees[cur_f]) {
            if (!removed_nodes[cur_f].count(key) && val < 15) {
                if (val > best || !found) {
                    found = true; 
                    best = val; 
                    bestNode = key; 
                }
            }
        }
        return bestNode; 
    }

    std::string LivenessAnalysisBehavior::pick_high_node() {
        size_t best = 0; 
        std::string bestNode; 
        bool found = false; 
        for (const auto& [key, val] : nodeDegrees[cur_f]) {
            if (!removed_nodes[cur_f].count(key)) {
                if (val > best || !found) {
                    found = true; 
                    best = val; 
                    bestNode = key; 
                }
            }
        }
        return bestNode; 
    }

    void LivenessAnalysisBehavior::update_graph(const std::string &selected) {
        removed_nodes[cur_f].insert(selected); 
        auto it = interferenceGraph[cur_f].find(selected); 
        if (it == interferenceGraph[cur_f].end()) return; 
        for (const auto& neigh : it -> second) {
            if (removed_nodes[cur_f].count(neigh)) { continue ;} 
            auto& d = nodeDegrees[cur_f][neigh]; 
            if (d > 0) d--; 
        }
    }

    void LivenessAnalysisBehavior::select_nodes() {
        auto& functionNodeDegrees = nodeDegrees[cur_f]; 
        bool hasPick = true; 
        while (hasPick) {
            std::string selected; 
            std::string low_node = pick_low_node();
            if (low_node.empty()) {
                std::string high_node = pick_high_node();
                if (high_node.empty()) {
                    hasPick = false; 
                } else {
                    selected = high_node; 
                    node_stack[cur_f].push_back(high_node); 
                }
            } else {
                selected = low_node; 
                node_stack[cur_f].push_back(low_node); 
            }
            if (!selected.empty()) {
                update_graph(selected); 
            }
        }
    } 

    bool LivenessAnalysisBehavior::color_or_spill_node(const std::string &cur_node, const std::unordered_set<std::string> &neighbors) {
        for (const auto& color : colorOrder) {
            bool found = true; 
            for (const auto& neigh : neighbors) {
                if (color == neigh || (colorOutputs[cur_f].count(neigh) && color == colorOutputs[cur_f].at(neigh))) {
                    found = false; 
                    break; 
                }
            }
            if (found) {
                colorOutputs[cur_f][cur_node] = color;
                return false; 
            }
        }
        bool isSpillTemp = cur_node.size() >= 2 && cur_node[1] == 'S'; 
        if (!isSpillTemp) {
            spillOutputs[cur_f].insert(cur_node);
        } 
        return true; 
    }

    bool LivenessAnalysisBehavior::color_graph() {
        select_nodes(); 
        auto& functionNodeStack = node_stack[cur_f]; 
        auto& functionInterferenceGraph = interferenceGraph[cur_f]; 
        bool spill = false; 
        while (!functionNodeStack.empty()) {
            std::string cur_node = functionNodeStack.back(); 
            functionNodeStack.pop_back(); 
            if (color_or_spill_node(cur_node, functionInterferenceGraph[cur_node])) {
                spill = true;
            } 
        }
        if (spill) {
            return false; 
        }
        return true; 
    }


    void LivenessAnalysisBehavior::print_in_out_sets() {
        for (size_t f = 0; f < livenessData.size(); ++f) {
            std::cout << "Function " << f << ":\n";

            for (size_t i = 0; i < livenessData[f].size(); ++i) {
            const auto& ls = livenessData[f][i];

            auto printSet = [](const std::unordered_set<std::string>& s) {
                bool first = true;
                for (const auto& x : s) {
                if (!first) std::cout << ", ";
                std::cout << x;
                first = false;
                }
            };

            std::cout << "  Instr " << i << "\n";

            std::cout << "    IN  : { ";
            printSet(ls.in);
            std::cout << " }\n";

            std::cout << "    OUT : { ";
            printSet(ls.out);
            std::cout << " }\n";
            }
        }
    }

    void LivenessAnalysisBehavior::print_paren_set(const std::unordered_set<std::string>& s) {
        if (s.empty()) {
            out << "()\n";
            return;
        }

        // alphabetical order
        std::vector<std::string> v(s.begin(), s.end());
        std::sort(v.begin(), v.end());

        out << "(";
        for (size_t i = 0; i < v.size(); ++i) {
            if (i) out << " ";
            out << v[i];
        }
        out << ")\n";
    }

    void LivenessAnalysisBehavior::print_liveness_tests() {
        const size_t f = 0;

        out << "(\n";

        out << "(in\n";
        for (size_t i = 0; i < livenessData[f].size(); ++i) {
            print_paren_set(livenessData[f][i].in);
        }

        out << ")\n\n";

        out << "(out\n";
        for (size_t i = 0; i < livenessData[f].size(); ++i) {
            print_paren_set(livenessData[f][i].out);
        }

        out << ")\n\n";

        out << ")\n";
    }

    void LivenessAnalysisBehavior::print_interference_tests() {
        const size_t f = 0; 
        std::vector<std::string> keys; 
        std::transform(interferenceGraph[f].begin(), interferenceGraph[f].end(), std::back_inserter(keys), [](const auto& m) {return m.first;});
        std::sort(keys.begin(), keys.end()); 
        for (auto& key: keys) {
            std::vector<std::string> keyConnects; 
            auto& neighSet = interferenceGraph[f].at(key); 
            std::transform(neighSet.begin(), neighSet.end(), std::back_inserter(keyConnects), [](const auto& s) {return s;});
            std::sort(keyConnects.begin(), keyConnects.end()); 
            out << key;
            for (const auto& neigh : keyConnects) {
                if (neigh == key) continue;      
                    out << " " << neigh;
                }
            out << "\n";                    
        }
    }

    void analyze_liveness(Program p) {

        LivenessAnalysisBehavior b(std::cout);
        p.accept(b); 

        return;
    }
}
